/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#if ( BORINGSSL_LIB)
int waitCounter = 0;
#elif ( OPENSSL_LIB)
int waitCounter = 0;
#elif ( LIBSODIUM_LIB)
int waitCounter = 0;
#elif ( CRYPTOPP_LIB)
int waitCounter = 0;
#endif

/* -- Begin Profiling Symbol Block for routine MPI_Wait */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Wait = PMPI_Wait
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Wait  MPI_Wait
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Wait as PMPI_Wait
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Wait(MPI_Request *request, MPI_Status *status) __attribute__((weak,alias("PMPI_Wait")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Wait
#define MPI_Wait PMPI_Wait
#undef FUNCNAME
#define FUNCNAME MPIR_Wait_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Wait_impl(MPI_Request *request, MPI_Status *status)
{
    int mpi_errno = MPI_SUCCESS;
    int active_flag;
    MPID_Request *request_ptr = NULL;

    /* If this is a null request handle, then return an empty status */
    if (*request == MPI_REQUEST_NULL)
    {
	MPIR_Status_set_empty(status);
	goto fn_exit;
    }

    MPID_Request_get_ptr(*request, request_ptr);

#if defined (_SHARP_SUPPORT_)
    if (request_ptr->sharp_req != NULL) {
        mpi_errno = MPID_SHARP_COLL_REQ_WAIT(request_ptr);
        if (mpi_errno != MPID_SHARP_COLL_SUCCESS) {
            PRINT_ERROR("SHArP non-blocking collective failed \n ");
            MPID_SHARP_COLL_REQ_FREE(request_ptr);
            mpi_errno = MPI_ERR_INTERN;
            MPIR_ERR_POP(mpi_errno);
        }
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }
#endif


    if (!MPID_Request_is_complete(request_ptr))
    {
	MPID_Progress_state progress_state;

        /* If this is an anysource request including a communicator with
         * anysource disabled, convert the call to an MPI_Test instead so we
         * don't get stuck in the progress engine. */
        if (unlikely(MPIR_CVAR_ENABLE_FT &&
                    MPID_Request_is_anysource(request_ptr) &&
                    !MPID_Comm_AS_enabled(request_ptr->comm))) {
            mpi_errno = MPIR_Test_impl(request, &active_flag, status);
            goto fn_exit;
        }

	MPID_Progress_start(&progress_state);
        while (!MPID_Request_is_complete(request_ptr))
	{
	    mpi_errno = MPIR_Grequest_progress_poke(1, &request_ptr, status);
	    if (request_ptr->kind == MPID_UREQUEST &&
                request_ptr->greq_fns->wait_fn != NULL)
	    {
		if (mpi_errno) {
		    /* --BEGIN ERROR HANDLING-- */
		    MPID_Progress_end(&progress_state);
                    MPIR_ERR_POP(mpi_errno);
                    /* --END ERROR HANDLING-- */
		}
		continue; /* treating UREQUEST like normal request means we'll
			     poll indefinitely. skip over progress_wait */
	    }

        /* In a multi-threaded scenario, with tests like
         * test/mpi/threads/comm/comm_idup, we see a corner case where the
         * previous call to MPIR_Grequest_progress_poke is completing the
         * request causing the blocking call 'MPID_Progress_wait' below to get
         * stuck. This check is to catch this scenario. */
        if (MPID_Request_is_complete(request_ptr)) {
            break;
        }

	    mpi_errno = MPID_Progress_wait(&progress_state);
	    if (mpi_errno) {
		/* --BEGIN ERROR HANDLING-- */
		MPID_Progress_end(&progress_state);
                MPIR_ERR_POP(mpi_errno);
		/* --END ERROR HANDLING-- */
	    }

            if (unlikely(
                        MPIR_CVAR_ENABLE_FT &&
                        MPID_Request_is_anysource(request_ptr) &&
                        !MPID_Request_is_complete(request_ptr) &&
                        !MPID_Comm_AS_enabled(request_ptr->comm))) {
                MPID_Progress_end(&progress_state);
                MPIR_ERR_SET(mpi_errno, MPIX_ERR_PROC_FAILED_PENDING, "**failure_pending");
                if (status != MPI_STATUS_IGNORE) status->MPI_ERROR = mpi_errno;
                goto fn_fail;
            }
	}
	MPID_Progress_end(&progress_state);
    }

    mpi_errno = MPIR_Request_complete(request, request_ptr, status, &active_flag);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Wait
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
    MPI_Wait - Waits for an MPI request to complete

Input Parameters:
. request - request (handle) 

Output Parameters:
. status - status object (Status).  May be 'MPI_STATUS_IGNORE'.

.N waitstatus

.N ThreadSafe

.N Fortran

.N FortranStatus

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_REQUEST
.N MPI_ERR_ARG
@*/
int MPI_Wait(MPI_Request *request, MPI_Status *status)
{
    MPID_Request * request_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm * comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WAIT);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_PT2PT_FUNC_ENTER(MPID_STATE_MPI_WAIT);

    /* Check the arguments */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL(request, "request", mpi_errno);
	    /* NOTE: MPI_STATUS_IGNORE != NULL */
	    MPIR_ERRTEST_ARGNULL(status, "status", mpi_errno);
	    MPIR_ERRTEST_REQUEST_OR_NULL(*request, mpi_errno);
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* If this is a null request handle, then return an empty status */
    if (*request == MPI_REQUEST_NULL)
    {
	MPIR_Status_set_empty(status);
	goto fn_exit;
    }
    
    /* Convert MPI request handle to a request object pointer */
    MPID_Request_get_ptr(*request, request_ptr);
    
    /* Validate object pointers if error checking is enabled */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPID_Request_valid_ptr( request_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */

    /* save copy of comm because request will be freed */
    if (request_ptr)
        comm_ptr = request_ptr->comm;
    mpi_errno = MPIR_Wait_impl(request, status);
    if (mpi_errno) goto fn_fail;

    /* ... end of body of routine ... */
    
  fn_exit:
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_WAIT);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;
	
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
				     FCNAME, __LINE__, MPI_ERR_OTHER,
				     "**mpi_wait", "**mpi_wait %p %p", 
				     request, status);
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* Added by Abu Naser */
#if ( BORINGSSL_LIB)

int MPI_SEC_Wait(MPI_Request *request, MPI_Status *status){
    
    int mpi_errno = MPI_SUCCESS;
    int  recv_sz=0; 
    int var;
    unsigned long count;          
       
    mpi_errno=MPI_Wait(request, status);
    MPI_Datatype datatype = MPI_CHAR;
    MPI_Get_count(status, datatype, &recv_sz);

     if(!EVP_AEAD_CTX_open(ctx, bufptr[waitCounter],
                        &count, (recv_sz-12),
                        &Ideciphertext[waitCounter][0], 12,
                         &Ideciphertext[waitCounter][12], (recv_sz-12),
                        NULL, 0)){
            printf("Decryption error: wait\n");
            fflush(stdout);
        }

    waitCounter++;
     if(waitCounter == (NON_BLOCKING_SEND_RECV_SIZE-1))
        waitCounter=0;
    return mpi_errno;
}
#elif ( OPENSSL_LIB)
/* This implementation is for variable nonce */
int MPI_SEC_Wait(MPI_Request *request, MPI_Status *status){
    
    int mpi_errno = MPI_SUCCESS;
    int  recv_sz=0; 
    //int var;
    unsigned long count;          
       
    mpi_errno=MPI_Wait(request, status);
    MPI_Datatype datatype = MPI_CHAR;
    MPI_Get_count(status, datatype, &recv_sz);
    //var = openssl_dec_core(ciphertext,0,buf,0,blocktype_recv);
    openssl_dec_core(&Ideciphertext[waitCounter][0],0,bufptr[waitCounter],0,recv_sz-16);
     /*var = crypto_aead_aes256gcm_decrypt_afternm(bufptr[waitCounter], &count,
                                  NULL,
                                  &Ideciphertext[waitCounter][12], (unsigned long)(recv_sz-12),
                                  NULL,
                                  0,
                                  &Ideciphertext[waitCounter][0],(const crypto_aead_aes256gcm_state *) &ctx);
    if(var != 0){
        printf("Decryption failed\n");
        fflush(stdout);
    } 
*/
    waitCounter++;
    if(waitCounter == (NON_BLOCKING_SEND_RECV_SIZE-1))
        waitCounter=0;
    return mpi_errno;
}
#elif ( LIBSODIUM_LIB)
/* This implementation is for variable nonce */
int MPI_SEC_Wait(MPI_Request *request, MPI_Status *status){
    
    int mpi_errno = MPI_SUCCESS;
    int  recv_sz=0; 
    int var;
    unsigned long count;          
       
    mpi_errno=MPI_Wait(request, status);
    MPI_Datatype datatype = MPI_CHAR;
    MPI_Get_count(status, datatype, &recv_sz);

    var = crypto_aead_aes256gcm_decrypt_afternm(bufptr[waitCounter], &count,
                                  NULL,
                                  &Ideciphertext[waitCounter][12], (unsigned long)(recv_sz-12),
                                  NULL,
                                  0,
                                  &Ideciphertext[waitCounter][0],(const crypto_aead_aes256gcm_state *) &ctx);
    if(var != 0){
        printf("Decryption failed, wait\n");
        fflush(stdout);
    } 

    waitCounter++;
    if(waitCounter == (NON_BLOCKING_SEND_RECV_SIZE-1))
        waitCounter=0;
    return mpi_errno;
}
#elif ( CRYPTOPP_LIB)
/* This implementation is for variable nonce */
int MPI_SEC_Wait(MPI_Request *request, MPI_Status *status){
    
    int mpi_errno = MPI_SUCCESS;
    int  recv_sz=0; 
    int var;
    unsigned long count;          
       
    mpi_errno=MPI_Wait(request, status);
    MPI_Datatype datatype = MPI_CHAR;
    MPI_Get_count(status, datatype, &recv_sz);

    /* var = crypto_aead_aes256gcm_decrypt_afternm(bufptr[waitCounter], &count,
                                  NULL,
                                  &Ideciphertext[waitCounter][12], (unsigned long)(recv_sz-12),
                                  NULL,
                                  0,
                                  &Ideciphertext[waitCounter][0],(const crypto_aead_aes256gcm_state *) &ctx);
    if(var != 0){
        printf("Decryption failed, wait\n");
        fflush(stdout);
    }  */
	
	//void wrapper_decrypt( char *ciphertext ,  char *recvbuf, int datacount+tagsize, unsigned long key_size, unsigned char *key, unsigned char *nonce)
	wrapper_decrypt( &Ideciphertext[waitCounter][12] ,  bufptr[waitCounter], (unsigned long)(recv_sz-12), key_size, gcm_key,&Ideciphertext[waitCounter][0]);

    waitCounter++;
    if(waitCounter == (500-1))
        waitCounter=0;
    return mpi_errno;
}
#endif