/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
/* Added by Abu Naser */
#if ( BORINGSSL_LIB)
unsigned char deciphertext[4194304+18];
#elif ( OPENSSL_LIB)
unsigned char ciphertext_recv[4194304+400];
int outlen_dec;
int outlen_dec_org;
EVP_CIPHER_CTX *ctx_dec;
#elif ( LIBSODIUM_LIB)
unsigned char deciphertext[4194304+18];
#elif ( CRYPTOPP_LIB)
unsigned char ciphertext_recv[4194304+400];
#endif

/* -- Begin Profiling Symbol Block for routine MPI_Recv */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Recv = PMPI_Recv
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Recv  MPI_Recv
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Recv as PMPI_Recv
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
             MPI_Comm comm, MPI_Status *status) __attribute__((weak,alias("PMPI_Recv")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Recv
#define MPI_Recv PMPI_Recv

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Recv

/*@
    MPI_Recv - Blocking receive for a message

Output Parameters:
+ buf - initial address of receive buffer (choice) 
- status - status object (Status) 

Input Parameters:
+ count - maximum number of elements in receive buffer (integer) 
. datatype - datatype of each receive buffer element (handle) 
. source - rank of source (integer) 
. tag - message tag (integer) 
- comm - communicator (handle) 

Notes:
The 'count' argument indicates the maximum length of a message; the actual 
length of the message can be determined with 'MPI_Get_count'.  

.N ThreadSafe

.N Fortran

.N FortranStatus

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_TYPE
.N MPI_ERR_COUNT
.N MPI_ERR_TAG
.N MPI_ERR_RANK

@*/
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
	     MPI_Comm comm, MPI_Status *status)
{
    static const char FCNAME[] = "MPI_Recv";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Request * request_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_RECV);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_PT2PT_FUNC_ENTER_BACK(MPID_STATE_MPI_RECV);
    
    /* Validate handle parameters needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
	    /* NOTE: MPI_STATUS_IGNORE != NULL */
	    MPIR_ERRTEST_ARGNULL(status, "status", mpi_errno);
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

    /* MT: Note that MPID_Recv may release the SINGLE_CS if it
       decides to block internally.  MPID_Recv in that case will
       re-aquire the SINGLE_CS before returnning */
    mpi_errno = MPID_Recv(buf, count, datatype, source, tag, comm_ptr, 
			  MPID_CONTEXT_INTRA_PT2PT, status, &request_ptr);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    if (request_ptr == NULL)
    {
	goto fn_exit;
    }
    
    /* If a request was returned, then we need to block until the request is 
       complete */
    if (!MPID_Request_is_complete(request_ptr))
    {
	MPID_Progress_state progress_state;
	    
	MPID_Progress_start(&progress_state);
        while (!MPID_Request_is_complete(request_ptr))
	{
	    /* MT: Progress_wait may release the SINGLE_CS while it
	       waits */
	    mpi_errno = MPID_Progress_wait(&progress_state);
	    if (mpi_errno != MPI_SUCCESS)
	    { 
		/* --BEGIN ERROR HANDLING-- */
		MPID_Progress_end(&progress_state);
		goto fn_fail;
		/* --END ERROR HANDLING-- */
	    }

            if (unlikely(MPIR_CVAR_ENABLE_FT &&
                        !MPID_Request_is_complete(request_ptr) &&
                        MPID_Request_is_anysource(request_ptr) &&
                        !MPID_Comm_AS_enabled(request_ptr->comm))) {
                /* --BEGIN ERROR HANDLING-- */
                MPID_Cancel_recv(request_ptr);
                MPIR_STATUS_SET_CANCEL_BIT(request_ptr->status, FALSE);
                MPIR_ERR_SET(request_ptr->status.MPI_ERROR, MPIX_ERR_PROC_FAILED, "**proc_failed");
                mpi_errno = request_ptr->status.MPI_ERROR;
                goto fn_fail;
                /* --END ERROR HANDLING-- */
            }
	}
	MPID_Progress_end(&progress_state);
    }

    mpi_errno = request_ptr->status.MPI_ERROR;
    MPIR_Request_extract_status(request_ptr, status);
    MPID_Request_release(request_ptr);

    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */
    
  fn_exit:
    MPID_MPI_PT2PT_FUNC_EXIT_BACK(MPID_STATE_MPI_RECV);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_recv",
	    "**mpi_recv %p %d %D %i %t %C %p", buf, count, datatype, source, tag, comm, status);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* Added by Abu Naser */
#if ( BORINGSSL_LIB)
/* Variable nonce */
int MPI_SEC_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
	     MPI_Comm comm, MPI_Status *status)
{
    int mpi_errno = MPI_SUCCESS;

    unsigned long  ciphertext_len;
    unsigned long decrypted_len=0;

    int  recvtype_sz;           
    MPI_Type_size(datatype, &recvtype_sz);         
    
    ciphertext_len = (unsigned long)((count * recvtype_sz) + 16); 
    
    /* Receive additional 12 bytes for the nonce and 16 bytes for the tag */
    mpi_errno = MPI_Recv(deciphertext, ciphertext_len+12, MPI_CHAR, source, tag, comm, status);
   
    /* First 12 bytes of the deciphertext is nonce. So ciphertext */ 
    /* begins from deciphertext+12. And nonce from deciphertext   */   
	if(!EVP_AEAD_CTX_open(ctx, buf,
                        &decrypted_len, (ciphertext_len),
                        deciphertext, 12,
                        (deciphertext+12), ciphertext_len,
                        NULL, 0)){
                    printf("Decryption error\n");fflush(stdout);        
            }
           
    return mpi_errno;
}
#elif ( OPENSSL_LIB)
void openssl_dec_core(unsigned char * ciphertext_recv, unsigned long long src, 
                      const void *recvbuf, unsigned long long dest, unsigned long long blocktype_recv)
{
        
    EVP_DecryptInit_ex(ctx_dec, NULL, NULL, NULL, ciphertext_recv+src);       
    EVP_DecryptUpdate(ctx_dec, recvbuf+dest, &outlen_dec, ciphertext_recv+12+src, (blocktype_recv-12));
    EVP_CIPHER_CTX_ctrl(ctx_dec, EVP_CTRL_GCM_SET_TAG, 16,(ciphertext_recv+src+blocktype_recv));              
    if (!(EVP_DecryptFinal_ex(ctx_dec, (recvbuf+dest+outlen_dec), &outlen_dec) > 0)) printf("Tag Verify Failed!\n");
}

int MPI_SEC_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status)
{    
	
	//const unsigned char gcm_key[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	//unsigned char gcm_iv[] = {0,0,0,0,0,0,0,0,0,0,0,0}; 	
	int i;
	int recvtype_sz;
	MPI_Type_size(datatype, &recvtype_sz);
	unsigned long long blocktype_recv= (unsigned long long) recvtype_sz*count;
	
	//unsigned char * ciphertext_recv;
	unsigned long long next, src;
	
	int mpi_errno = MPI_SUCCESS;

	
	
	mpi_errno=MPI_Recv(ciphertext_recv, blocktype_recv+16+12, MPI_CHAR, source, tag, comm,status);
	
	// printf("Ciphertext @ Receiver:\n");
	// BIO_dump_fp(stdout, ciphertext, count+16);
	
	openssl_dec_core(ciphertext_recv,0,buf,0,blocktype_recv+12);
	
	//MPIU_Free(ciphertext_recv);
	
	return mpi_errno;
}
#elif ( LIBSODIUM_LIB)
/* This implementation used a variable nonce */
int MPI_SEC_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
         MPI_Comm comm, MPI_Status *status)
{
    int mpi_errno = MPI_SUCCESS;
    int var;
    int i;
    int  recvtype_sz;           
    MPI_Type_size(datatype, &recvtype_sz); 

    /* Receive additional 12 bytes for the nonce and 16 bytes for the tag */  
    mpi_errno=MPI_Recv(deciphertext, (recvtype_sz*count+16+12), MPI_CHAR, source, tag, comm,status);

    /* First 12 bytes of the deciphertext is nonce. So ciphertext */ 
    /* begins from deciphertext+12. And nonce from deciphertext   */                              
    var = crypto_aead_aes256gcm_decrypt_afternm(buf, &count,
                                  NULL,
                                  (deciphertext+12), (recvtype_sz*count+16),
                                  NULL,
                                  0,
                                  deciphertext,(const crypto_aead_aes256gcm_state *) &ctx);                                                           
    if(var != 0)
        printf("Decryption failed\n");fflush(stdout);                                                              

    return mpi_errno;
}
#elif ( CRYPTOPP_LIB)
int MPI_SEC_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status)
{   
	int mpi_errno = MPI_SUCCESS;

	//unsigned char * ciphertext;
     int  recvtype_sz;           
    MPI_Type_size(datatype, &recvtype_sz);
 
    mpi_errno=MPI_Recv(ciphertext_recv, (count*recvtype_sz+12+12), MPI_CHAR, source, tag, comm,status);
	
	int decryptedtext_len=0;

	//decryptedtext_len =decrypt((unsigned char *)ciphertext, buf , (count*recvtype_sz+12));
    
	//wrapper_decrypt( char *ciphertext ,  char *recvbuf, int datacount+tagsize, unsigned long key_size, unsigned char *key, unsigned char *nonce)
	wrapper_decrypt( ciphertext_recv+12 ,  buf, (recvtype_sz*count)+12, key_size, gcm_key,ciphertext_recv);
	 

    //MPIU_Free(ciphertext);
	
    return mpi_errno;
}

#endif



/* Fixed nonce implementation */
#if 0
int MPI_SEC_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
	     MPI_Comm comm, MPI_Status *status)
{
    int mpi_errno = MPI_SUCCESS;

    unsigned long  ciphertext_len;
    unsigned long decrypted_len=0;

    int  recvtype_sz;           
    MPI_Type_size(datatype, &recvtype_sz);         
    
    ciphertext_len = (unsigned long)((count * recvtype_sz) + 16); 
    mpi_errno = MPI_Recv(deciphertext, ciphertext_len, MPI_CHAR, source, tag, comm, status);
   
	if(!EVP_AEAD_CTX_open(ctx, buf,
                        &decrypted_len, ciphertext_len,
                        nonce, 12,
                        deciphertext, ciphertext_len,
                        NULL, 0)){
                    printf("Decryption error\n");fflush(stdout);        
            }
           
    return mpi_errno;
}
#endif
