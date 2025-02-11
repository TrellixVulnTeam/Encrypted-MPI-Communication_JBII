/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
/* added by abu naser */
#if ( BORINGSSL_LIB)
unsigned char alltoallv_ciphertext_sendbuf[268435456+4000]; // 268435456 = 4MB * 64
unsigned char alltoallv_ciphertext_recvbuf[268435456+4000]; // 268435456 = 4MB * 64
int cipher_send_dis[50000], cipher_recv_dis[50000];
int cipher_sendcounts[50000], cipher_recvcounts[50000];
#elif ( OPENSSL_LIB)
unsigned char alltoallv_ciphertext_sendbuf[268435456+4000];
unsigned char alltoallv_ciphertext_recvbuf[268435456+4000];
int cipher_send_dis[50000], cipher_recv_dis[50000];
int cipher_sendcounts[50000], cipher_recvcounts[50000];
#elif ( LIBSODIUM_LIB)
unsigned char alltoallv_ciphertext_sendbuf[268435456+4000]; // 268435456 = 4MB * 64
unsigned char alltoallv_ciphertext_recvbuf[268435456+4000]; // 268435456 = 4MB * 64
int cipher_send_dis[50000], cipher_recv_dis[50000];
int cipher_sendcounts[50000], cipher_recvcounts[50000];
#elif ( CRYPTOPP_LIB)
unsigned char alltoallv_ciphertext_sendbuf[268435456+4000]; // 268435456 = 4MB * 64
unsigned char alltoallv_ciphertext_recvbuf[268435456+4000]; // 268435456 = 4MB * 64
int cipher_send_dis[50000], cipher_recv_dis[50000];
int cipher_sendcounts[50000], cipher_recvcounts[50000];
#endif


/* end of add */

/* -- Begin Profiling Symbol Block for routine MPI_Alltoallv */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Alltoallv = PMPI_Alltoallv
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Alltoallv  MPI_Alltoallv
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Alltoallv as PMPI_Alltoallv
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Alltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls,
                  MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                  const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm)
                  __attribute__((weak,alias("PMPI_Alltoallv")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Alltoallv
#define MPI_Alltoallv PMPI_Alltoallv
/* This is the default implementation of alltoallv. The algorithm is:
   
   Algorithm: MPI_Alltoallv

   Since each process sends/receives different amounts of data to
   every other process, we don't know the total message size for all
   processes without additional communication. Therefore we simply use
   the "middle of the road" isend/irecv algorithm that works
   reasonably well in all cases.

   We post all irecvs and isends and then do a waitall. We scatter the
   order of sources and destinations among the processes, so that all
   processes don't try to send/recv to/from the same process at the
   same time. 

   *** Modification: We post only a small number of isends and irecvs 
   at a time and wait on them as suggested by Tony Ladd. ***

   For MPI_IN_PLACE we use a completely different algorithm.  We perform
   pair-wise exchanges among all processes using sendrecv_replace.  This
   conserves memory usage at the expense of time performance.

   Possible improvements: 

   End Algorithm: MPI_Alltoallv
*/
 

/* not declared static because a machine-specific function may call this one in some cases */
#undef FUNCNAME
#define FUNCNAME MPIR_Alltoallv_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Alltoallv_intra(const void *sendbuf, const int *sendcounts, const int *sdispls,
                         MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                         const int *rdispls, MPI_Datatype recvtype, MPID_Comm *comm_ptr,
                         MPIR_Errflag_t *errflag)
{
    int        comm_size, i, j;
    MPI_Aint   send_extent, recv_extent;
    int        mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Status *starray;
    MPI_Status status;
    MPID_Request **reqarray;
    int dst, rank, req_cnt;
    int ii, ss, bblock;
    int type_size;

    MPIU_CHKLMEM_DECL(2);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* Get extent of recv type, but send type is only valid if (sendbuf!=MPI_IN_PLACE) */
    MPID_Datatype_get_extent_macro(recvtype, recv_extent);

    /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER( comm_ptr );

    if (sendbuf == MPI_IN_PLACE) {
        /* We use pair-wise sendrecv_replace in order to conserve memory usage,
         * which is keeping with the spirit of the MPI-2.2 Standard.  But
         * because of this approach all processes must agree on the global
         * schedule of sendrecv_replace operations to avoid deadlock.
         *
         * Note that this is not an especially efficient algorithm in terms of
         * time and there will be multiple repeated malloc/free's rather than
         * maintaining a single buffer across the whole loop.  Something like
         * MADRE is probably the best solution for the MPI_IN_PLACE scenario. */
        for (i = 0; i < comm_size; ++i) {
            /* start inner loop at i to avoid re-exchanging data */
            for (j = i; j < comm_size; ++j) {
                if (rank == i) {
                    /* also covers the (rank == i && rank == j) case */
                    mpi_errno = MPIC_Sendrecv_replace(((char *)recvbuf + rdispls[j]*recv_extent),
                                                         recvcounts[j], recvtype,
                                                         j, MPIR_ALLTOALLV_TAG,
                                                         j, MPIR_ALLTOALLV_TAG,
                                                         comm_ptr, &status, errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }

                }
                else if (rank == j) {
                    /* same as above with i/j args reversed */
                    mpi_errno = MPIC_Sendrecv_replace(((char *)recvbuf + rdispls[i]*recv_extent),
                                                         recvcounts[i], recvtype,
                                                         i, MPIR_ALLTOALLV_TAG,
                                                         i, MPIR_ALLTOALLV_TAG,
                                                         comm_ptr, &status, errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                }
            }
        }
    }
    else {
        bblock = MPIR_CVAR_ALLTOALL_THROTTLE;
        if (bblock == 0) bblock = comm_size;

        MPID_Datatype_get_extent_macro(sendtype, send_extent);

        MPIU_CHKLMEM_MALLOC(starray,  MPI_Status*,  2*bblock*sizeof(MPI_Status),  mpi_errno, "starray");
        MPIU_CHKLMEM_MALLOC(reqarray, MPID_Request**, 2*bblock*sizeof(MPID_Request *), mpi_errno, "reqarray");

        /* post only bblock isends/irecvs at a time as suggested by Tony Ladd */
        for (ii=0; ii<comm_size; ii+=bblock) {
            req_cnt = 0;
            ss = comm_size-ii < bblock ? comm_size-ii : bblock;

            /* do the communication -- post ss sends and receives: */
            for ( i=0; i<ss; i++ ) { 
                dst = (rank+i+ii) % comm_size;
                if (recvcounts[dst]) {
                    MPID_Datatype_get_size_macro(recvtype, type_size);
                    if (type_size) {
                        MPIU_Ensure_Aint_fits_in_pointer(MPIU_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
                                                         rdispls[dst]*recv_extent);
                        mpi_errno = MPIC_Irecv((char *)recvbuf+rdispls[dst]*recv_extent,
                                                  recvcounts[dst], recvtype, dst,
                                                  MPIR_ALLTOALLV_TAG, comm_ptr,
                                                  &reqarray[req_cnt]);
                        if (mpi_errno) {
                            /* for communication errors, just record the error but continue */
                            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                        }
                        req_cnt++;
                    }
                }
            }

            for ( i=0; i<ss; i++ ) { 
                dst = (rank-i-ii+comm_size) % comm_size;
                if (sendcounts[dst]) {
                    MPID_Datatype_get_size_macro(sendtype, type_size);
                    if (type_size) {
                        MPIU_Ensure_Aint_fits_in_pointer(MPIU_VOID_PTR_CAST_TO_MPI_AINT sendbuf +
                                                         sdispls[dst]*send_extent);
                        mpi_errno = MPIC_Isend((char *)sendbuf+sdispls[dst]*send_extent,
                                                  sendcounts[dst], sendtype, dst,
                                                  MPIR_ALLTOALLV_TAG, comm_ptr,
                                                  &reqarray[req_cnt], errflag);
                        if (mpi_errno) {
                            /* for communication errors, just record the error but continue */
                            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                        }
                        req_cnt++;
                    }
                }
            }

            mpi_errno = MPIC_Waitall(req_cnt, reqarray, starray, errflag);
            if (mpi_errno && mpi_errno != MPI_ERR_IN_STATUS) MPIR_ERR_POP(mpi_errno);

            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno == MPI_ERR_IN_STATUS) {
                for (i=0; i<req_cnt; i++) {
                    if (starray[i].MPI_ERROR != MPI_SUCCESS) {
                        mpi_errno = starray[i].MPI_ERROR;
                        if (mpi_errno) {
                            /* for communication errors, just record the error but continue */
                            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                        }
                    }
                }
            }
            /* --END ERROR HANDLING-- */
        }
    }

fn_exit:
    /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT( comm_ptr );
    MPIU_CHKLMEM_FREEALL();

    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");

    return mpi_errno;

fn_fail:
    goto fn_exit;
}



/* not declared static because a machine-specific function may call this one in some cases */
#undef FUNCNAME
#define FUNCNAME MPIR_Alltoallv_inter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Alltoallv_inter(const void *sendbuf, const int *sendcounts, const int *sdispls,
                         MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                         const int *rdispls, MPI_Datatype recvtype, MPID_Comm *comm_ptr,
                         MPIR_Errflag_t *errflag)
{
/* Intercommunicator alltoallv. We use a pairwise exchange algorithm
   similar to the one used in intracommunicator alltoallv. Since the
   local and remote groups can be of different 
   sizes, we first compute the max of local_group_size,
   remote_group_size. At step i, 0 <= i < max_size, each process
   receives from src = (rank - i + max_size) % max_size if src <
   remote_size, and sends to dst = (rank + i) % max_size if dst <
   remote_size. 

   FIXME: change algorithm to match intracommunicator alltoallv

*/
    int local_size, remote_size, max_size, i;
    MPI_Aint   send_extent, recv_extent;
    int        mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Status status;
    int src, dst, rank, sendcount, recvcount;
    char *sendaddr, *recvaddr;

    local_size = comm_ptr->local_size; 
    remote_size = comm_ptr->remote_size;
    rank = comm_ptr->rank;
    
    /* Get extent of send and recv types */
    MPID_Datatype_get_extent_macro(sendtype, send_extent);
    MPID_Datatype_get_extent_macro(recvtype, recv_extent);
    
    /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER( comm_ptr );

    /* Use pairwise exchange algorithm. */
    max_size = MPIR_MAX(local_size, remote_size);
    for (i=0; i<max_size; i++) {
        src = (rank - i + max_size) % max_size;
        dst = (rank + i) % max_size;
        if (src >= remote_size) {
            src = MPI_PROC_NULL;
            recvaddr = NULL;
            recvcount = 0;
        }
        else {
            MPIU_Ensure_Aint_fits_in_pointer(MPIU_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
					     rdispls[src]*recv_extent);
            recvaddr = (char *)recvbuf + rdispls[src]*recv_extent;
            recvcount = recvcounts[src];
        }
        if (dst >= remote_size) {
            dst = MPI_PROC_NULL;
            sendaddr = NULL;
            sendcount = 0;
        }
        else {
            MPIU_Ensure_Aint_fits_in_pointer(MPIU_VOID_PTR_CAST_TO_MPI_AINT sendbuf +
					     sdispls[dst]*send_extent);
            sendaddr = (char *)sendbuf + sdispls[dst]*send_extent;
            sendcount = sendcounts[dst];
        }

        mpi_errno = MPIC_Sendrecv(sendaddr, sendcount, sendtype, dst,
                                     MPIR_ALLTOALLV_TAG, recvaddr, recvcount, 
                                     recvtype, src, MPIR_ALLTOALLV_TAG,
                                     comm_ptr, &status, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

 fn_exit:
    /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT( comm_ptr );
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIR_Alltoallv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Alltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls,
                   MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls,
                   MPI_Datatype recvtype, MPID_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;
        
    if (comm_ptr->comm_kind == MPID_INTRACOMM) {
        /* intracommunicator */
        mpi_errno = MPIR_Alltoallv_intra(sendbuf, sendcounts, sdispls,
                                         sendtype, recvbuf, recvcounts,
                                         rdispls, recvtype, comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    } else {
        /* intercommunicator */
        mpi_errno = MPIR_Alltoallv_inter(sendbuf, sendcounts, sdispls,
                                         sendtype, recvbuf, recvcounts,
                                         rdispls, recvtype, comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Alltoallv_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Alltoallv_impl(const void *sendbuf, const int *sendcounts, const int *sdispls,
                        MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                        const int *rdispls, MPI_Datatype recvtype,
                        MPID_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;
        
    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Alltoallv != NULL) {
	/* --BEGIN USEREXTENSION-- */
	mpi_errno = comm_ptr->coll_fns->Alltoallv(sendbuf, sendcounts, sdispls,
                                                 sendtype, recvbuf, recvcounts,
                                                 rdispls, recvtype, comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
	/* --END USEREXTENSION-- */
    } else {
        mpi_errno = MPIR_Alltoallv(sendbuf, sendcounts, sdispls,
                                   sendtype, recvbuf, recvcounts,
                                   rdispls, recvtype, comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}



#endif


#undef FUNCNAME
#define FUNCNAME MPI_Alltoallv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Alltoallv - Sends data from all to all processes; each process may 
   send a different amount of data and provide displacements for the input
   and output data.

Input Parameters:
+ sendbuf - starting address of send buffer (choice) 
. sendcounts - integer array equal to the group size 
specifying the number of elements to send to each processor 
. sdispls - integer array (of length group size). Entry 
 'j'  specifies the displacement (relative to sendbuf  from
which to take the outgoing data destined for process  'j'  
. sendtype - data type of send buffer elements (handle) 
. recvcounts - integer array equal to the group size 
specifying the maximum number of elements that can be received from
each processor 
. rdispls - integer array (of length group size). Entry 
 'i'  specifies the displacement (relative to recvbuf  at
which to place the incoming data from process  'i'  
. recvtype - data type of receive buffer elements (handle) 
- comm - communicator (handle) 

Output Parameters:
. recvbuf - address of receive buffer (choice) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_BUFFER
@*/
int MPI_Alltoallv(const void *sendbuf, const int *sendcounts,
                  const int *sdispls, MPI_Datatype sendtype, void *recvbuf,
                  const int *recvcounts, const int *rdispls, MPI_Datatype recvtype,
                  MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ALLTOALLV);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_ALLTOALLV);

    /* Validate parameters, especially handles needing to be converted */
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

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPID_Datatype *sendtype_ptr=NULL, *recvtype_ptr=NULL;
            int i, comm_size;
            int check_send = (comm_ptr->comm_kind == MPID_INTRACOMM && sendbuf != MPI_IN_PLACE);

            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

            if (comm_ptr->comm_kind == MPID_INTRACOMM) {
                comm_size = comm_ptr->local_size;

                if (sendbuf != MPI_IN_PLACE && sendtype == recvtype && sendcounts == recvcounts)
                    MPIR_ERRTEST_ALIAS_COLL(sendbuf, recvbuf, mpi_errno);
            } else
                comm_size = comm_ptr->remote_size;

            if (comm_ptr->comm_kind == MPID_INTERCOMM && sendbuf == MPI_IN_PLACE) {
                MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**sendbuf_inplace");
            }

            for (i=0; i<comm_size; i++) {
                if (check_send) {
                    MPIR_ERRTEST_COUNT(sendcounts[i], mpi_errno);
                    MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                }
                MPIR_ERRTEST_COUNT(recvcounts[i], mpi_errno);
                MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
            }
            if (check_send && HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype_get_ptr(sendtype, sendtype_ptr);
                MPID_Datatype_valid_ptr( sendtype_ptr, mpi_errno );
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                MPID_Datatype_committed_ptr( sendtype_ptr, mpi_errno );
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }
            if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype_get_ptr(recvtype, recvtype_ptr);
                MPID_Datatype_valid_ptr( recvtype_ptr, mpi_errno );
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                MPID_Datatype_committed_ptr( recvtype_ptr, mpi_errno );
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }

            for (i=0; i<comm_size && check_send; i++) {
                if (sendcounts[i] > 0) {
                    MPIR_ERRTEST_USERBUFFER(sendbuf,sendcounts[i],sendtype,mpi_errno);
                }
            }
            for (i=0; i<comm_size; i++) {
                if (recvcounts[i] > 0) {
                    MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcounts[i], mpi_errno);
                    MPIR_ERRTEST_USERBUFFER(recvbuf,recvcounts[i],recvtype,mpi_errno);
                    break;
                }
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Alltoallv_impl(sendbuf, sendcounts, sdispls,
                                    sendtype, recvbuf, recvcounts,
                                    rdispls, recvtype, comm_ptr, &errflag);
    if (mpi_errno) goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLTOALLV);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_alltoallv",
	    "**mpi_alltoallv %p %p %p %D %p %p %p %D %C", sendbuf, sendcounts, sdispls, sendtype,
	    recvbuf, recvcounts, rdispls, recvtype, comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* Added by Abu Naser */
#if ( BORINGSSL_LIB)
/* variable nonce */
int MPI_SEC_Alltoallv(const void *sendbuf, const int *sendcounts,
                  const int *sdispls, MPI_Datatype sendtype, void *recvbuf,
                  const int *recvcounts, const int *rdispls, MPI_Datatype recvtype,
                  MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
     MPID_Comm *comm_ptr = NULL;
    int var;
	
    int sendtype_sz, recvtype_sz;
    unsigned long  ciphertext_sendbuf_len = 0;
    sendtype_sz= recvtype_sz= 0;

    var=MPI_Type_size(sendtype, &sendtype_sz);
    var=MPI_Type_size(recvtype, &recvtype_sz);

    MPID_Comm_get_ptr( comm, comm_ptr);
	int rank;
	rank = comm_ptr->rank;

	unsigned long  count=0;
    unsigned long next, dest;
	unsigned long t=0;
    unsigned int j;
    int k;
    int send_index = 0;
    int recv_index = 0;
    unsigned long   max_out_len;
    
    dest = 0;
    cipher_send_dis[0] = 0; // send data to 0 process from 0.
    cipher_recv_dis[0] = 0; // reveive data from 0 process to 0.
    
    for(j = 0, k=0; j < comm_ptr->local_size; j++, k++){
        t = (unsigned long )(sendtype_sz*sendcounts[k]);
        max_out_len = t+16;
        next = (unsigned long)(sdispls[k]*sendtype_sz);
        
       
        /* Set the nonce in alltoall_ciphertext */
        RAND_bytes(&alltoallv_ciphertext_sendbuf[send_index], 12); // 12 bytes of nonce
        /*nonceCounter++;
        memset(&alltoallv_ciphertext_sendbuf[send_index], 0, 8);
        alltoallv_ciphertext_sendbuf[send_index+8] = (nonceCounter >> 24) & 0xFF;
        alltoallv_ciphertext_sendbuf[send_index+9] = (nonceCounter >> 16) & 0xFF;
        alltoallv_ciphertext_sendbuf[send_index+10] = (nonceCounter >> 8) & 0xFF;
        alltoallv_ciphertext_sendbuf[send_index+11] = nonceCounter & 0xFF;*/
     
        if(!EVP_AEAD_CTX_seal(ctx, (alltoallv_ciphertext_sendbuf+send_index+12),
                         &ciphertext_sendbuf_len, max_out_len,
                         (alltoallv_ciphertext_sendbuf+send_index), 12,
                         sendbuf+next, t,
                        NULL, 0)){
              printf("Error in encryption: alltoallv\n");
              fflush(stdout);
        }        

        
        /* update new displacement for send and receive */
        cipher_send_dis[k] = send_index;
        cipher_recv_dis[k] = recv_index;

        send_index +=(sendcounts[k]*sendtype_sz+16+12);
        recv_index +=(recvcounts[k]*recvtype_sz+16+12);

        /* update cipher sendcounts and receive counts */
        cipher_sendcounts[k] = (sendcounts[k]*sendtype_sz+16+12);
        cipher_recvcounts[k] = (recvcounts[k]*recvtype_sz+16+12); 
        
     }

     var=MPI_Alltoallv(alltoallv_ciphertext_sendbuf, cipher_sendcounts,
                  cipher_send_dis, MPI_CHAR, alltoallv_ciphertext_recvbuf,
                  cipher_recvcounts, cipher_recv_dis, MPI_CHAR, comm);

unsigned int i;
     for(i = 0, k=0; i < comm_ptr->local_size; i++, k++){
        
        /* decrypt from modified location. */
        next = (unsigned long)cipher_recv_dis[k];

        /* receive in actual destination as user passed */
        dest = (unsigned long )(rdispls[k]*recvtype_sz);

      
         if(!EVP_AEAD_CTX_open(ctx, ((recvbuf+dest)),
                        &count, (unsigned long )(cipher_recvcounts[k]),
                        (alltoallv_ciphertext_recvbuf+next), 12,
                        (alltoallv_ciphertext_recvbuf+next+12), (unsigned long )(cipher_recvcounts[k]-12),
                        NULL, 0)){
                    printf("Decryption error: alltoallv\n");fflush(stdout);        
            }       
    }                       
    return mpi_errno;
}
#elif ( OPENSSL_LIB)
int MPI_SEC_Alltoallv(const void *sendbuf, const int *sendcounts,
                  const int *sdispls, MPI_Datatype sendtype, void *recvbuf,
                  const int *recvcounts, const int *rdispls, MPI_Datatype recvtype,
                  MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    int var;
	
    int sendtype_sz, recvtype_sz;
    unsigned long long ciphertext_sendbuf_len = 0;
    sendtype_sz= recvtype_sz= 0;

    var=MPI_Type_size(sendtype, &sendtype_sz);
    var=MPI_Type_size(recvtype, &recvtype_sz);

    MPID_Comm_get_ptr( comm, comm_ptr);
	int rank;
	rank = comm_ptr->rank;

	unsigned long long count=0;
    unsigned long long next, dest;
	unsigned long long blocktype_send=0;
    
    int k;
    int send_index = 0;
    int recv_index = 0;
  
    dest = 0;
    cipher_send_dis[0] = 0; // send data to 0 process from 0.
    cipher_recv_dis[0] = 0; // reveive data from 0 process to 0.

    for( k=0; k < comm_ptr->local_size; k++){
		
        blocktype_send = (unsigned long long)(sendtype_sz*sendcounts[k]);
        next = (unsigned long long)(sdispls[k]*sendtype_sz);


        openssl_enc_core(alltoallv_ciphertext_sendbuf,send_index,sendbuf,next,blocktype_send);
        
        /* update new displacement for send and receive */
        cipher_send_dis[k] = send_index;
        cipher_recv_dis[k] = recv_index;

        send_index +=(sendcounts[k]*sendtype_sz+16+12);
        recv_index +=(recvcounts[k]*recvtype_sz+16+12);

        /* update cipher sendcounts and receive counts */
        cipher_sendcounts[k] = (sendcounts[k]*sendtype_sz+16+12);
        cipher_recvcounts[k] = (recvcounts[k]*recvtype_sz+16+12); 
        
     }

     var=MPI_Alltoallv(alltoallv_ciphertext_sendbuf, cipher_sendcounts,
                  cipher_send_dis, MPI_CHAR, alltoallv_ciphertext_recvbuf,
                  cipher_recvcounts, cipher_recv_dis, MPI_CHAR, comm);

     for( k=0; k < comm_ptr->local_size; k++){   

  
        next = (unsigned long long)cipher_recv_dis[k];   
        dest = (unsigned long long)(rdispls[k]*recvtype_sz);
		
		
		openssl_dec_core(alltoallv_ciphertext_recvbuf,next,recvbuf,dest,((unsigned long long)cipher_recvcounts[k]-16));

    }                       


    return mpi_errno;
}

#elif ( LIBSODIUM_LIB)
/* This implementation use variable nonce */
int MPI_SEC_Alltoallv(const void *sendbuf, const int *sendcounts,
                  const int *sdispls, MPI_Datatype sendtype, void *recvbuf,
                  const int *recvcounts, const int *rdispls, MPI_Datatype recvtype,
                  MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    int var;
	
    int sendtype_sz, recvtype_sz;
    unsigned long long ciphertext_sendbuf_len = 0;
    sendtype_sz= recvtype_sz= 0;

    var=MPI_Type_size(sendtype, &sendtype_sz);
    var=MPI_Type_size(recvtype, &recvtype_sz);

    MPID_Comm_get_ptr( comm, comm_ptr);
	int rank;
	rank = comm_ptr->rank;

	unsigned long long count=0;
    unsigned int next, dest;
	unsigned long long t=0;
    unsigned int j;
    int k;
    int send_index = 0;
    int recv_index = 0;
    
    dest = 0;
    cipher_send_dis[0] = 0; // send data to 0 process from 0.
    cipher_recv_dis[0] = 0; // reveive data from 0 process to 0.

    for(j = 0, k=0; j < comm_ptr->local_size; j++, k++){
        t = (unsigned long long)(sendtype_sz*sendcounts[k]);
        next = (unsigned int)(sdispls[k]*sendtype_sz);

        /* Set the nonce in send_ciphertext */
        randombytes_buf(&alltoallv_ciphertext_sendbuf[send_index], 12);

        /*nonceCounter++;
        memset(&alltoallv_ciphertext_sendbuf[send_index], 0, 8);
        alltoallv_ciphertext_sendbuf[send_index+8] = (nonceCounter >> 24) & 0xFF;
        alltoallv_ciphertext_sendbuf[send_index+9] = (nonceCounter >> 16) & 0xFF;
        alltoallv_ciphertext_sendbuf[send_index+10] = (nonceCounter >> 8) & 0xFF;
        alltoallv_ciphertext_sendbuf[send_index+11] = nonceCounter & 0xFF;*/
     
        var = crypto_aead_aes256gcm_encrypt_afternm((alltoallv_ciphertext_sendbuf+send_index+12), &ciphertext_sendbuf_len,
            sendbuf+next, t,
            NULL, 0,
            NULL, (alltoallv_ciphertext_sendbuf+send_index), (const crypto_aead_aes256gcm_state *) &ctx); 

        if(var != 0)
            printf("Encryption failed\n");fflush(stdout);

           
        //dest = (unsigned int)(sendtype_sz*sendcounts[k]+16);
        
        /* update new displacement for send and receive */
        cipher_send_dis[k] = send_index;
        cipher_recv_dis[k] = recv_index;

        send_index +=(sendcounts[k]*sendtype_sz+16+12);
        recv_index +=(recvcounts[k]*recvtype_sz+16+12);

        /* update cipher sendcounts and receive counts */
        cipher_sendcounts[k] = (sendcounts[k]*sendtype_sz+16+12);
        cipher_recvcounts[k] = (recvcounts[k]*recvtype_sz+16+12); 
       
     }

     var=MPI_Alltoallv(alltoallv_ciphertext_sendbuf, cipher_sendcounts,
                  cipher_send_dis, MPI_CHAR, alltoallv_ciphertext_recvbuf,
                  cipher_recvcounts, cipher_recv_dis, MPI_CHAR, comm);
		
	unsigned int i;
     for(i = 0, k=0; i < comm_ptr->local_size; i++, k++){
        
        /* decrypt from modified location. */
        next = (unsigned int)cipher_recv_dis[k];

        /* receive in actual destination as user passed */
        dest = (unsigned int )(rdispls[k]*recvtype_sz);

        var = crypto_aead_aes256gcm_decrypt_afternm(((recvbuf+dest)), &count,
                                  NULL,
                                  (alltoallv_ciphertext_recvbuf+next+12), (unsigned long long)(cipher_recvcounts[k]-12),
                                  NULL,
                                  0,
                                  (alltoallv_ciphertext_recvbuf+next),(const crypto_aead_aes256gcm_state *) &ctx);
        if(var != 0)
            printf("Decryption failed\n");fflush(stdout);  
      
    }                       


    return mpi_errno;
}
#elif ( CRYPTOPP_LIB)
int MPI_SEC_Alltoallv(const void *sendbuf, const int *sendcounts,
                  const int *sdispls, MPI_Datatype sendtype, void *recvbuf,
                  const int *recvcounts, const int *rdispls, MPI_Datatype recvtype,
                  MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    int var;
	
    int sendtype_sz, recvtype_sz;
    unsigned long long ciphertext_sendbuf_len = 0;
    sendtype_sz= recvtype_sz= 0;

    var=MPI_Type_size(sendtype, &sendtype_sz);
    var=MPI_Type_size(recvtype, &recvtype_sz);

    MPID_Comm_get_ptr( comm, comm_ptr);
	int rank;
	rank = comm_ptr->rank;

	unsigned long long count=0;
    unsigned int next, dest;
	unsigned long long t=0;
    unsigned int j;
    int k;
    int send_index = 0;
    int recv_index = 0;
    
    dest = 0;
    cipher_send_dis[0] = 0; // send data to 0 process from 0.
    cipher_recv_dis[0] = 0; // reveive data from 0 process to 0.

    for(j = 0, k=0; j < comm_ptr->local_size; j++, k++){
        t = (unsigned long )(sendtype_sz*sendcounts[k]);
        next = (unsigned int)(sdispls[k]*sendtype_sz);

        /* Set the nonce in send_ciphertext */
        //randombytes_buf(&alltoallv_ciphertext_sendbuf[send_index], 12);
		
		//wrapper_nonce(unsigned char * nonce, unsigned long nonce_len)
		wrapper_nonce(&alltoallv_ciphertext_sendbuf[send_index], 12);

        /* nonceCounter++;
        memset(&alltoallv_ciphertext_sendbuf[send_index], 0, 8);
        alltoallv_ciphertext_sendbuf[send_index+8] = (nonceCounter >> 24) & 0xFF;
        alltoallv_ciphertext_sendbuf[send_index+9] = (nonceCounter >> 16) & 0xFF;
        alltoallv_ciphertext_sendbuf[send_index+10] = (nonceCounter >> 8) & 0xFF;
        alltoallv_ciphertext_sendbuf[send_index+11] = nonceCounter & 0xFF; */
     
        /* var = crypto_aead_aes256gcm_encrypt_afternm((alltoallv_ciphertext_sendbuf+send_index+12), &ciphertext_sendbuf_len,
            sendbuf+next, t,
            NULL, 0,
            NULL, (alltoallv_ciphertext_sendbuf+send_index), (const crypto_aead_aes256gcm_state *) &ctx);  */
		
		//wrapper_encrypt( char *sendbuf ,  char *ciphertext , int datacount, unsigned long key_size, unsigned char *key, unsigned char *nonce)
		wrapper_encrypt( sendbuf+next ,  alltoallv_ciphertext_sendbuf+send_index+12 , t, key_size, gcm_key, alltoallv_ciphertext_sendbuf+send_index);

           
        //dest = (unsigned int)(sendtype_sz*sendcounts[k]+16);
        
        /* update new displacement for send and receive */
        cipher_send_dis[k] = send_index;
        cipher_recv_dis[k] = recv_index;

        send_index +=(sendcounts[k]*sendtype_sz+12+12);
        recv_index +=(recvcounts[k]*recvtype_sz+12+12);

        /* update cipher sendcounts and receive counts */
        cipher_sendcounts[k] = (sendcounts[k]*sendtype_sz+12+12);
        cipher_recvcounts[k] = (recvcounts[k]*recvtype_sz+12+12); 
       
     }

     var=MPI_Alltoallv(alltoallv_ciphertext_sendbuf, cipher_sendcounts,
                  cipher_send_dis, MPI_CHAR, alltoallv_ciphertext_recvbuf,
                  cipher_recvcounts, cipher_recv_dis, MPI_CHAR, comm);
	unsigned int i ;
     for(i = 0, k=0; i < comm_ptr->local_size; i++, k++){
        
        /* decrypt from modified location. */
        next = (unsigned int)cipher_recv_dis[k];

        /* receive in actual destination as user passed */
        dest = (unsigned int )(rdispls[k]*recvtype_sz);

        /* var = crypto_aead_aes256gcm_decrypt_afternm(((recvbuf+dest)), &count,
                                  NULL,
                                  (alltoallv_ciphertext_recvbuf+next+12), (unsigned long long)(cipher_recvcounts[k]-12),
                                  NULL,
                                  0,
                                  (alltoallv_ciphertext_recvbuf+next),(const crypto_aead_aes256gcm_state *) &ctx); */
        
		//wrapper_decrypt( char *ciphertext ,  char *recvbuf, int datacount+tagsize, unsigned long key_size, unsigned char *key, unsigned char *nonce)
		
		wrapper_decrypt( alltoallv_ciphertext_recvbuf+next+12 ,  recvbuf+dest, cipher_recvcounts[k]-12, key_size, gcm_key,alltoallv_ciphertext_recvbuf+next);
    }                       


    return mpi_errno;
}

#endif
/* End of add. */    