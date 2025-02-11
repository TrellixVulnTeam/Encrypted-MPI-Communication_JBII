/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
// added by Abu Naser, an16e@my.fsu.edu, june 12, 2018
#include <sodium.h> 
 CRYPTO_ALIGN(16) crypto_aead_aes256gcm_state ctx;
 unsigned char key [32] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','a','b','c','d','e','f'};
unsigned char send_ciphertext[4194304+18];
int nonceCounter;
// end of add
/* -- Begin Profiling Symbol Block for routine MPI_Send */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Send = PMPI_Send
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Send  MPI_Send
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Send as PMPI_Send
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
             MPI_Comm comm) __attribute__((weak,alias("PMPI_Send")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Send
#define MPI_Send PMPI_Send

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Send

/*@
    MPI_Send - Performs a blocking send

Input Parameters:
+ buf - initial address of send buffer (choice) 
. count - number of elements in send buffer (nonnegative integer) 
. datatype - datatype of each send buffer element (handle) 
. dest - rank of destination (integer) 
. tag - message tag (integer) 
- comm - communicator (handle) 

Notes:
This routine may block until the message is received by the destination 
process.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_TAG
.N MPI_ERR_RANK

.seealso: MPI_Isend, MPI_Bsend
@*/
int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	     MPI_Comm comm)
{
    static const char FCNAME[] = "MPI_Send";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Request * request_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_SEND);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_PT2PT_FUNC_ENTER_FRONT(MPID_STATE_MPI_SEND);
    
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
	    MPIR_ERRTEST_SEND_RANK(comm_ptr, dest, mpi_errno);
	    MPIR_ERRTEST_SEND_TAG(tag, mpi_errno);
	    
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
    
    mpi_errno = MPID_Send(buf, count, datatype, dest, tag, comm_ptr, 
			  MPID_CONTEXT_INTRA_PT2PT, &request_ptr);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    if (request_ptr == NULL)
    {
	goto fn_exit;
    }

    /* If a request was returned, then we need to block until the request 
       is complete */
    if (!MPID_Request_is_complete(request_ptr))
    {
	MPID_Progress_state progress_state;
	    
	MPID_Progress_start(&progress_state);
        while (!MPID_Request_is_complete(request_ptr))
	{
	    mpi_errno = MPID_Progress_wait(&progress_state);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		/* --BEGIN ERROR HANDLING-- */
		MPID_Progress_end(&progress_state);
		goto fn_fail;
		/* --END ERROR HANDLING-- */
	    }
	}
	MPID_Progress_end(&progress_state);
    }

    mpi_errno = request_ptr->status.MPI_ERROR;
    MPID_Request_release(request_ptr);
    
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */
    
  fn_exit:
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_SEND);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_send", 
	    "**mpi_send %p %d %D %i %t %C", buf, count, datatype, dest, tag, comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* Added by Abu Naser, june 11 */
/* This implementation used a fixed nonce */
#if 0
int MPI_SEC_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{

int mpi_errno = MPI_SUCCESS;
int i;
int var;
int ciphertext_len=0;
int * val;
unsigned char * c;


int  sendtype_sz=0;           
MPI_Type_size(datatype, &sendtype_sz);

 MPID_Comm *comm_ptr = NULL;
    MPID_Comm_get_ptr( comm, comm_ptr );
    int rank;
    rank = comm_ptr->rank; 

int total = sendtype_sz * count;

var = crypto_aead_aes256gcm_encrypt_afternm(send_ciphertext,&ciphertext_len,
            buf, total,
            NULL, 0,
            NULL, nonce, (const crypto_aead_aes256gcm_state *) &ctx); 
 if(var != 0){
        printf("Encryption failed\n");
        fflush(stdout);
    }
  

mpi_errno=MPI_Send(send_ciphertext, ciphertext_len, MPI_CHAR, dest, tag, comm);

return mpi_errno;
}
#endif

/* This implementation will use unique nonce for each message */
int MPI_SEC_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{

int mpi_errno = MPI_SUCCESS;
int i;
int var;
unsigned long long ciphertext_len=0;
int * val;
unsigned char * c;


int  sendtype_sz=0;           
MPI_Type_size(datatype, &sendtype_sz);

 MPID_Comm *comm_ptr = NULL;
    MPID_Comm_get_ptr( comm, comm_ptr );
    int rank;
    rank = comm_ptr->rank; 

int total = sendtype_sz * count;

/* Set the nonce in send_ciphertext */
nonceCounter++;
memset(send_ciphertext, 0, 8);
send_ciphertext[8] = (nonceCounter >> 24) & 0xFF;
send_ciphertext[9] = (nonceCounter >> 16) & 0xFF;
send_ciphertext[10] = (nonceCounter >> 8) & 0xFF;
send_ciphertext[11] = nonceCounter & 0xFF;

/* send_ciphertext's first 12 byte is now nonce. so cipher will start from */
/* send_ciphertext+12. And nonce is from send_ciphertext                   */
var = crypto_aead_aes256gcm_encrypt_afternm(send_ciphertext+12,&ciphertext_len,
            buf, total,
            NULL, 0,
            NULL, send_ciphertext, (const crypto_aead_aes256gcm_state *) &ctx); 
 if(var != 0){
        printf("Encryption failed\n");
        fflush(stdout);
    }

/* Sending additional 12 bytes for nonce */  
mpi_errno=MPI_Send(send_ciphertext, ciphertext_len+12, MPI_CHAR, dest, tag, comm);

return mpi_errno;

}

/* Initializaiton of lib sodium */
void init_crypto(){
    int var= sodium_init();
    nonceCounter = 0;
    var = crypto_aead_aes256gcm_beforenm(&ctx, key);
    return;
}
/*  End of adding */
