/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#if ( BORINGSSL_LIB)
unsigned char Ideciphertext[NON_BLOCKING_SEND_RECV_SIZE][NON_BLOCKING_SEND_RECV_SIZE_2];
unsigned char * bufptr[100000];
int reqCounter = 0;
#elif ( OPENSSL_LIB)
unsigned char Ideciphertext[NON_BLOCKING_SEND_RECV_SIZE][NON_BLOCKING_SEND_RECV_SIZE_2];
unsigned char * bufptr[100000];
int reqCounter = 0;
#elif ( LIBSODIUM_LIB)
unsigned char Ideciphertext[NON_BLOCKING_SEND_RECV_SIZE][NON_BLOCKING_SEND_RECV_SIZE_2];
unsigned char * bufptr[100000];
int reqCounter = 0;
#elif ( CRYPTOPP_LIB)
unsigned char Ideciphertext[NON_BLOCKING_SEND_RECV_SIZE][NON_BLOCKING_SEND_RECV_SIZE_2];
unsigned char * bufptr[50000];
int reqCounter = 0;
#endif

/* -- Begin Profiling Symbol Block for routine MPI_Irecv */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Irecv = PMPI_Irecv
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Irecv  MPI_Irecv
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Irecv as PMPI_Irecv
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
              MPI_Comm comm, MPI_Request *request) __attribute__((weak,alias("PMPI_Irecv")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Irecv
#define MPI_Irecv PMPI_Irecv

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Irecv

/*@
    MPI_Irecv - Begins a nonblocking receive

Input Parameters:
+ buf - initial address of receive buffer (choice) 
. count - number of elements in receive buffer (integer) 
. datatype - datatype of each receive buffer element (handle) 
. source - rank of source (integer) 
. tag - message tag (integer) 
- comm - communicator (handle) 

Output Parameters:
. request - communication request (handle) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_TAG
.N MPI_ERR_RANK
.N MPI_ERR_EXHAUSTED
@*/
int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source,
	      int tag, MPI_Comm comm, MPI_Request *request)
{
    static const char FCNAME[] = "MPI_Irecv";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Request *request_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_IRECV);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_PT2PT_FUNC_ENTER_BACK(MPID_STATE_MPI_IRECV);

    /* Validate handle parameters needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( comm, comm_ptr );

    /* Validate parameters if error checking is enabled */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (mpi_errno) goto fn_fail;
	    
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_RECV_RANK(comm_ptr, source, mpi_errno);
	    MPIR_ERRTEST_RECV_TAG(tag, mpi_errno);
	    MPIR_ERRTEST_ARGNULL(request,"request",mpi_errno);

	    /* Validate datatype handle */
	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
	    
	    /* Validate datatype object */
	    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
	    {
		MPID_Datatype *datatype_ptr = NULL;

		MPID_Datatype_get_ptr(datatype, datatype_ptr);
		MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
		if (mpi_errno) goto fn_fail;
		MPID_Datatype_committed_ptr(datatype_ptr, mpi_errno);
		if (mpi_errno) goto fn_fail;
	    }
	    
	    /* Validate buffer */
	    MPIR_ERRTEST_USERBUFFER(buf,count,datatype,mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPID_Irecv(buf, count, datatype, source, tag, comm_ptr, 
			   MPID_CONTEXT_INTRA_PT2PT, &request_ptr);
    /* return the handle of the request to the user */
    /* MPIU_OBJ_HANDLE_PUBLISH is unnecessary for irecv, lower-level access is
     * responsible for its own consistency, while upper-level field access is
     * controlled by the completion counter */
    *request = request_ptr->handle;

    /* Put this part after setting the request so that if the request is
     * pending (which is still considered an error), it will still be set
     * correctly here. For real error cases, the user might get garbage as
     * their request value, but that's fine since the definition is
     * undefined anyway. */
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */
    
  fn_exit:
    MPID_MPI_PT2PT_FUNC_EXIT_BACK(MPID_STATE_MPI_IRECV);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_irecv",
	    "**mpi_irecv %p %d %D %i %t %C %p", buf, count, datatype, source, tag, comm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
/* Added by Abu Naser */
#if ( BORINGSSL_LIB)
/* variable nonce */
/* variable nonce */
int MPI_SEC_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Status status;
    unsigned long ciphertext_len = 0;
    unsigned long decrypted_len = 0;
     MPI_Request req; 

    int  recvtype_sz;           
    MPI_Type_size(datatype, &recvtype_sz);         
    
    //ciphertext_len = (unsigned long)((count * recvtype_sz) + 16); 
    mpi_errno=MPI_Irecv(&Ideciphertext[reqCounter][0], (recvtype_sz*count+16+12), MPI_CHAR, source, tag, comm, &req);
    * request = req;
    bufptr[reqCounter]=buf;
    reqCounter++;
    if(reqCounter == (NON_BLOCKING_SEND_RECV_SIZE-1))
        reqCounter=0;

       
return mpi_errno;

}
#elif ( OPENSSL_LIB)
/* This implementation is for variable nonce */
int MPI_SEC_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;

    int var;
    int i;


    unsigned long long ciphertext_len;
    MPI_Status status;
    MPI_Request req; 

    int  recvtype_sz=0;           
    MPI_Type_size(datatype, &recvtype_sz);
    MPID_Comm *comm_ptr = NULL;
    MPID_Comm_get_ptr( comm, comm_ptr );
    int rank;
    rank = comm_ptr->rank; 

   // printf("Receiving rank=%d source = %d tag = %d count=%d recvtype_sz=%d\n",rank, source, tag, count, recvtype_sz); fflush(stdout);

    //mpi_errno=MPI_Irecv(&Ideciphertext[reqCounter][0], (recvtype_sz*count), MPI_CHAR, source, tag, comm, &req);
    mpi_errno=MPI_Irecv(&Ideciphertext[reqCounter][0], (recvtype_sz*count+16+12), MPI_CHAR, source, tag, comm, &req);
    * request = req;
    bufptr[reqCounter]=buf;
    reqCounter++;
    if(reqCounter == (NON_BLOCKING_SEND_RECV_SIZE-1))
        reqCounter=0;

#if 0
MPI_Wait(request, &status);
ciphertext_len = (unsigned long long)(recvtype_sz*count + 16); 

var = crypto_aead_aes256gcm_decrypt_afternm(buf, &count,
                                  NULL,
                                  Ideciphertext, ciphertext_len,
                                  NULL,
                                  0,
                                  nonce,(const crypto_aead_aes256gcm_state *) &ctx);
    if(var != 0)
        printf("Decryption failed\n");fflush(stdout);        
#endif

return mpi_errno;

}
#elif ( LIBSODIUM_LIB)
/* This implementation is for variable nonce */
int MPI_SEC_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;

    int var;
    int i;


    unsigned long long ciphertext_len;
    MPI_Status status;
    MPI_Request req; 

    int  recvtype_sz=0;           
    MPI_Type_size(datatype, &recvtype_sz);
    MPID_Comm *comm_ptr = NULL;
    MPID_Comm_get_ptr( comm, comm_ptr );
    int rank;
    rank = comm_ptr->rank; 

   // printf("Receiving rank=%d source = %d tag = %d count=%d recvtype_sz=%d\n",rank, source, tag, count, recvtype_sz); fflush(stdout);

    //mpi_errno=MPI_Irecv(&Ideciphertext[reqCounter][0], (recvtype_sz*count), MPI_CHAR, source, tag, comm, &req);
    mpi_errno=MPI_Irecv(&Ideciphertext[reqCounter][0], (recvtype_sz*count+16+12), MPI_CHAR, source, tag, comm, &req);
    * request = req;
    bufptr[reqCounter]=buf;
    reqCounter++;
    if(reqCounter == (NON_BLOCKING_SEND_RECV_SIZE-1))
        reqCounter=0;

#if 0
MPI_Wait(request, &status);
ciphertext_len = (unsigned long long)(recvtype_sz*count + 16); 

var = crypto_aead_aes256gcm_decrypt_afternm(buf, &count,
                                  NULL,
                                  Ideciphertext, ciphertext_len,
                                  NULL,
                                  0,
                                  nonce,(const crypto_aead_aes256gcm_state *) &ctx);
    if(var != 0)
        printf("Decryption failed\n");fflush(stdout);        
#endif

return mpi_errno;

}
#elif ( CRYPTOPP_LIB)
/* This implementation is for variable nonce */
int MPI_SEC_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;

    int var;
    int i;


    unsigned long long ciphertext_len;
    MPI_Status status;
    MPI_Request req; 

    int  recvtype_sz=0;           
    MPI_Type_size(datatype, &recvtype_sz);
    MPID_Comm *comm_ptr = NULL;
    MPID_Comm_get_ptr( comm, comm_ptr );
    int rank;
    rank = comm_ptr->rank; 
 
    mpi_errno=MPI_Irecv(&Ideciphertext[reqCounter][0], (recvtype_sz*count+12+12), MPI_CHAR, source, tag, comm, &req);
    * request = req;
    bufptr[reqCounter]=buf;
    reqCounter++;
    if(reqCounter == (NON_BLOCKING_SEND_RECV_SIZE-1))
        reqCounter=0;

return mpi_errno;

}

#endif

/* fixed nonce */
#if 0
int MPI_SEC_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Status status;
    unsigned long ciphertext_len = 0;
    unsigned long decrypted_len = 0;
     MPI_Request req; 
	if(reqCounter == (NON_BLOCKING_SEND_RECV_SIZE-1))
        reqCounter=0;

    int  recvtype_sz;           
    MPI_Type_size(datatype, &recvtype_sz);         
    
    //ciphertext_len = (unsigned long)((count * recvtype_sz) + 16); 
    mpi_errno=MPI_Irecv(&Ideciphertext[reqCounter][0], (recvtype_sz*count+16), MPI_CHAR, source, tag, comm, &req);
    * request = req;
    bufptr[reqCounter]=buf;
    reqCounter++;
 #if 0   
    mpi_errno = MPI_Irecv(Ideciphertext, ciphertext_len, MPI_CHAR, source, tag, comm,request);
    MPI_Wait(request, &status);

    if(!EVP_AEAD_CTX_open(ctx, buf,
                        &decrypted_len, ciphertext_len,
                        nonce, 12,
                        Ideciphertext, ciphertext_len,
                        NULL, 0)){
            printf("Decryption error.\n");
            fflush(stdout);
        }
#endif        
return mpi_errno;

}
#endif