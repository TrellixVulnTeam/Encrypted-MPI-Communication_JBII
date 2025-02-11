/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
/* Added by Abu Naser */
#if ( BORINGSSL_LIB)
unsigned char alltoall_ciphertext_sendbuf[268435456+4000]; // 268435456 = 4MB * 64
unsigned char alltoall_ciphertext_recvbuf[268435456+4000]; // 268435456 = 4MB * 64
#elif ( OPENSSL_LIB)
unsigned char ciphertext_send[268435456+4000]; // 268435456 = 4MB * 64
unsigned char ciphertext_recv[268435456+4000]; // 268435456 = 4MB * 64
#elif ( LIBSODIUM_LIB)
unsigned char alltoall_ciphertext_sendbuf[268435456+4000]; // 268435456 = 4MB * 64
unsigned char alltoall_ciphertext_recvbuf[268435456+4000]; // 268435456 = 4MB * 64
#elif ( CRYPTOPP_LIB)
unsigned char alltoall_ciphertext_sendbuf[268435456+4000]; // 268435456 = 4MB * 64
unsigned char alltoall_ciphertext_recvbuf[268435456+4000]; // 268435456 = 4MB * 64
#endif

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories :
   - name : COLLECTIVE
     description : A category for collective communication variables.

cvars:
   - name      : MPIR_CVAR_ALLTOALL_SHORT_MSG_SIZE
     category  : COLLECTIVE
     type      : int
     default   : 256
     class     : device
     verbosity : MPI_T_VERBOSITY_USER_BASIC
     scope     : MPI_T_SCOPE_ALL_EQ
     description : >-
       the short message algorithm will be used if the per-destination
       message size (sendcount*size(sendtype)) is <= this value
       (See also: MPIR_CVAR_ALLTOALL_MEDIUM_MSG_SIZE)

   - name      : MPIR_CVAR_ALLTOALL_MEDIUM_MSG_SIZE
     category  : COLLECTIVE
     type      : int
     default   : 32768
     class     : device
     verbosity : MPI_T_VERBOSITY_USER_BASIC
     scope     : MPI_T_SCOPE_ALL_EQ
     description : >-
       the medium message algorithm will be used if the per-destination
       message size (sendcount*size(sendtype)) is <= this value and
       larger than MPIR_CVAR_ALLTOALL_SHORT_MSG_SIZE
       (See also: MPIR_CVAR_ALLTOALL_SHORT_MSG_SIZE)

   - name      : MPIR_CVAR_ALLTOALL_THROTTLE
     category  : COLLECTIVE
     type      : int
     default   : 32
     class     : device
     verbosity : MPI_T_VERBOSITY_USER_BASIC
     scope     : MPI_T_SCOPE_ALL_EQ
     description : >-
       max no. of irecvs/isends posted at a time in some alltoall
       algorithms. Setting it to 0 causes all irecvs/isends to be
       posted at once

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Alltoall */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Alltoall = PMPI_Alltoall
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Alltoall  MPI_Alltoall
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Alltoall as PMPI_Alltoall
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                 int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
                 __attribute__((weak,alias("PMPI_Alltoall")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Alltoall
#define MPI_Alltoall PMPI_Alltoall

/* This is the default implementation of alltoall. The algorithm is:
   
   Algorithm: MPI_Alltoall

   We use four algorithms for alltoall. For short messages and
   (comm_size >= 8), we use the algorithm by Jehoshua Bruck et al,
   IEEE TPDS, Nov. 1997. It is a store-and-forward algorithm that
   takes lgp steps. Because of the extra communication, the bandwidth
   requirement is (n/2).lgp.beta.

   Cost = lgp.alpha + (n/2).lgp.beta

   where n is the total amount of data a process needs to send to all
   other processes.

   For medium size messages and (short messages for comm_size < 8), we
   use an algorithm that posts all irecvs and isends and then does a
   waitall. We scatter the order of sources and destinations among the
   processes, so that all processes don't try to send/recv to/from the
   same process at the same time.

   *** Modification: We post only a small number of isends and irecvs 
   at a time and wait on them as suggested by Tony Ladd. ***
   *** See comments below about an additional modification that 
   we may want to consider ***

   For long messages and power-of-two number of processes, we use a
   pairwise exchange algorithm, which takes p-1 steps. We
   calculate the pairs by using an exclusive-or algorithm:
           for (i=1; i<comm_size; i++)
               dest = rank ^ i;
   This algorithm doesn't work if the number of processes is not a power of
   two. For a non-power-of-two number of processes, we use an
   algorithm in which, in step i, each process  receives from (rank-i)
   and sends to (rank+i). 

   Cost = (p-1).alpha + n.beta

   where n is the total amount of data a process needs to send to all
   other processes.

   Possible improvements: 

   End Algorithm: MPI_Alltoall
*/


/* not declared static because a machine-specific function may call this one in some cases */
#undef FUNCNAME
#define FUNCNAME MPIR_Alltoall_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Alltoall_intra( 
    const void *sendbuf,
    int sendcount, 
    MPI_Datatype sendtype, 
    void *recvbuf, 
    int recvcount, 
    MPI_Datatype recvtype, 
    MPID_Comm *comm_ptr,
    MPIR_Errflag_t *errflag )
{
    int          comm_size, i, j, pof2;
    MPI_Aint     sendtype_extent, recvtype_extent;
    MPI_Aint recvtype_true_extent, recvbuf_extent, recvtype_true_lb;
    int mpi_errno=MPI_SUCCESS, src, dst, rank, nbytes;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Status status;
    int sendtype_size, block, *displs, count;
    MPI_Aint pack_size, position;
    MPI_Datatype newtype = MPI_DATATYPE_NULL;
    void *tmp_buf;
    MPID_Request **reqarray;
    MPI_Status *starray;
    MPIU_CHKLMEM_DECL(6);

    if (recvcount == 0) return MPI_SUCCESS;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* Get extent of send and recv types */
    MPID_Datatype_get_extent_macro(recvtype, recvtype_extent);
    MPID_Datatype_get_extent_macro(sendtype, sendtype_extent);

    MPID_Datatype_get_size_macro(sendtype, sendtype_size);
    nbytes = sendtype_size * sendcount;

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
                    mpi_errno = MPIC_Sendrecv_replace(((char *)recvbuf + j*recvcount*recvtype_extent),
                                                         recvcount, recvtype,
                                                         j, MPIR_ALLTOALL_TAG,
                                                         j, MPIR_ALLTOALL_TAG,
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
                    mpi_errno = MPIC_Sendrecv_replace(((char *)recvbuf + i*recvcount*recvtype_extent),
                                                         recvcount, recvtype,
                                                         i, MPIR_ALLTOALL_TAG,
                                                         i, MPIR_ALLTOALL_TAG,
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
    else if ((nbytes <= MPIR_CVAR_ALLTOALL_SHORT_MSG_SIZE) && (comm_size >= 8)) {

        /* use the indexing algorithm by Jehoshua Bruck et al,
         * IEEE TPDS, Nov. 97 */ 

        /* allocate temporary buffer */
        MPIR_Pack_size_impl(recvcount*comm_size, recvtype, &pack_size);
        MPIU_CHKLMEM_MALLOC(tmp_buf, void *, pack_size, mpi_errno, "tmp_buf");

        /* Do Phase 1 of the algorithim. Shift the data blocks on process i
         * upwards by a distance of i blocks. Store the result in recvbuf. */
        mpi_errno = MPIR_Localcopy((char *) sendbuf + 
			   rank*sendcount*sendtype_extent, 
                           (comm_size - rank)*sendcount, sendtype, recvbuf, 
                           (comm_size - rank)*recvcount, recvtype);
	if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }
        mpi_errno = MPIR_Localcopy(sendbuf, rank*sendcount, sendtype, 
                        (char *) recvbuf + 
				   (comm_size-rank)*recvcount*recvtype_extent, 
                                   rank*recvcount, recvtype);
	if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }
        /* Input data is now stored in recvbuf with datatype recvtype */

        /* Now do Phase 2, the communication phase. It takes
           ceiling(lg p) steps. In each step i, each process sends to rank+2^i
           and receives from rank-2^i, and exchanges all data blocks
           whose ith bit is 1. */

        /* allocate displacements array for indexed datatype used in
           communication */

        MPIU_CHKLMEM_MALLOC(displs, int *, comm_size * sizeof(int), mpi_errno, "displs");

        pof2 = 1;
        while (pof2 < comm_size) {
            dst = (rank + pof2) % comm_size;
            src = (rank - pof2 + comm_size) % comm_size;

            /* Exchange all data blocks whose ith bit is 1 */
            /* Create an indexed datatype for the purpose */

            count = 0;
            for (block=1; block<comm_size; block++) {
                if (block & pof2) {
                    displs[count] = block * recvcount;
                    count++;
                }
            }

            mpi_errno = MPIR_Type_create_indexed_block_impl(count, recvcount,
                                                            displs, recvtype, &newtype);
	    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

            mpi_errno = MPIR_Type_commit_impl(&newtype);
	    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

            position = 0;
            mpi_errno = MPIR_Pack_impl(recvbuf, 1, newtype, tmp_buf, pack_size, &position);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

            mpi_errno = MPIC_Sendrecv(tmp_buf, position, MPI_PACKED, dst,
                                         MPIR_ALLTOALL_TAG, recvbuf, 1, newtype,
                                         src, MPIR_ALLTOALL_TAG, comm_ptr,
                                         MPI_STATUS_IGNORE, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }

            MPIR_Type_free_impl(&newtype);

            pof2 *= 2;
        }

        /* Rotate blocks in recvbuf upwards by (rank + 1) blocks. Need
         * a temporary buffer of the same size as recvbuf. */
        
        /* get true extent of recvtype */
        MPIR_Type_get_true_extent_impl(recvtype, &recvtype_true_lb, &recvtype_true_extent);

        recvbuf_extent = recvcount * comm_size *
            (MPIR_MAX(recvtype_true_extent, recvtype_extent));
        MPIU_CHKLMEM_MALLOC(tmp_buf, void *, recvbuf_extent, mpi_errno, "tmp_buf");
        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *)((char*)tmp_buf - recvtype_true_lb);

        mpi_errno = MPIR_Localcopy((char *) recvbuf + (rank+1)*recvcount*recvtype_extent, 
                       (comm_size - rank - 1)*recvcount, recvtype, tmp_buf, 
                       (comm_size - rank - 1)*recvcount, recvtype);
	if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }
        mpi_errno = MPIR_Localcopy(recvbuf, (rank+1)*recvcount, recvtype, 
                       (char *) tmp_buf + (comm_size-rank-1)*recvcount*recvtype_extent, 
                       (rank+1)*recvcount, recvtype);
	if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }

        /* Blocks are in the reverse order now (comm_size-1 to 0). 
         * Reorder them to (0 to comm_size-1) and store them in recvbuf. */

        for (i=0; i<comm_size; i++){
            mpi_errno = MPIR_Localcopy((char *) tmp_buf + i*recvcount*recvtype_extent,
                                       recvcount, recvtype, 
                                       (char *) recvbuf + (comm_size-i-1)*recvcount*recvtype_extent, 
                                       recvcount, recvtype);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }

    else if (nbytes <= MPIR_CVAR_ALLTOALL_MEDIUM_MSG_SIZE) {
        /* Medium-size message. Use isend/irecv with scattered
           destinations. Use Tony Ladd's modification to post only
           a small number of isends/irecvs at a time. */
	/* FIXME: This converts the Alltoall to a set of blocking phases.
	   Two alternatives should be considered:
	   1) the choice of communication pattern could try to avoid
	      contending routes in each phase
	   2) rather than wait for all communication to finish (waitall),
	      we could maintain constant queue size by using waitsome 
	      and posting new isend/irecv as others complete.  This avoids
	      synchronization delays at the end of each block (when
	      there are only a few isend/irecvs left)
	 */
        int ii, ss, bblock;

        bblock = MPIR_CVAR_ALLTOALL_THROTTLE;
        if (bblock == 0) bblock = comm_size;

        MPIU_CHKLMEM_MALLOC(reqarray, MPID_Request **, 2*bblock*sizeof(MPID_Request*), mpi_errno, "reqarray");

        MPIU_CHKLMEM_MALLOC(starray, MPI_Status *, 2*bblock*sizeof(MPI_Status), mpi_errno, "starray");

        for (ii=0; ii<comm_size; ii+=bblock) {
            ss = comm_size-ii < bblock ? comm_size-ii : bblock;
            /* do the communication -- post ss sends and receives: */
            for ( i=0; i<ss; i++ ) { 
                dst = (rank+i+ii) % comm_size;
                mpi_errno = MPIC_Irecv((char *)recvbuf +
                                          dst*recvcount*recvtype_extent, 
                                          recvcount, recvtype, dst,
                                          MPIR_ALLTOALL_TAG, comm_ptr,
                                          &reqarray[i]);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }

            for ( i=0; i<ss; i++ ) { 
                dst = (rank-i-ii+comm_size) % comm_size;
                mpi_errno = MPIC_Isend((char *)sendbuf +
                                          dst*sendcount*sendtype_extent, 
                                          sendcount, sendtype, dst,
                                          MPIR_ALLTOALL_TAG, comm_ptr,
                                          &reqarray[i+ss], errflag);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
  
            /* ... then wait for them to finish: */
            mpi_errno = MPIC_Waitall(2*ss,reqarray,starray, errflag);
            if (mpi_errno && mpi_errno != MPI_ERR_IN_STATUS) MPIR_ERR_POP(mpi_errno);
            
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno == MPI_ERR_IN_STATUS) {
                for (j=0; j<2*ss; j++) {
                    if (starray[j].MPI_ERROR != MPI_SUCCESS) {
                        mpi_errno = starray[j].MPI_ERROR;
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

    else {
        /* Long message. If comm_size is a power-of-two, do a pairwise
           exchange using exclusive-or to create pairs. Else send to
           rank+i, receive from rank-i. */
        
        /* Make local copy first */
        mpi_errno = MPIR_Localcopy(((char *)sendbuf + 
                                    rank*sendcount*sendtype_extent), 
                                   sendcount, sendtype, 
                                   ((char *)recvbuf +
                                    rank*recvcount*recvtype_extent),
                                   recvcount, recvtype);
	if (mpi_errno) { MPIR_ERR_POP(mpi_errno ); }

        /* Is comm_size a power-of-two? */
        i = 1;
        while (i < comm_size)
            i *= 2;
        if (i == comm_size)
            pof2 = 1;
        else 
            pof2 = 0;

        /* Do the pairwise exchanges */
        for (i=1; i<comm_size; i++) {
            if (pof2 == 1) {
                /* use exclusive-or algorithm */
                src = dst = rank ^ i;
            }
            else {
                src = (rank - i + comm_size) % comm_size;
                dst = (rank + i) % comm_size;
            }

            mpi_errno = MPIC_Sendrecv(((char *)sendbuf +
                                          dst*sendcount*sendtype_extent), 
                                         sendcount, sendtype, dst,
                                         MPIR_ALLTOALL_TAG, 
                                         ((char *)recvbuf +
                                          src*recvcount*recvtype_extent),
                                         recvcount, recvtype, src,
                                         MPIR_ALLTOALL_TAG, comm_ptr, &status, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
    }

 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT( comm_ptr );
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");

    return mpi_errno;
 fn_fail:
    if (newtype != MPI_DATATYPE_NULL)
        MPIR_Type_free_impl(&newtype);
    goto fn_exit;
}



/* not declared static because a machine-specific function may call this one in some cases */
#undef FUNCNAME
#define FUNCNAME MPIR_Alltoall_inter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Alltoall_inter( 
    const void *sendbuf,
    int sendcount, 
    MPI_Datatype sendtype, 
    void *recvbuf, 
    int recvcount, 
    MPI_Datatype recvtype, 
    MPID_Comm *comm_ptr,
    MPIR_Errflag_t *errflag )
{
/* Intercommunicator alltoall. We use a pairwise exchange algorithm
   similar to the one used in intracommunicator alltoall for long
   messages. Since the local and remote groups can be of different
   sizes, we first compute the max of local_group_size,
   remote_group_size. At step i, 0 <= i < max_size, each process
   receives from src = (rank - i + max_size) % max_size if src <
   remote_size, and sends to dst = (rank + i) % max_size if dst <
   remote_size. 
*/
    int          local_size, remote_size, max_size, i;
    MPI_Aint     sendtype_extent, recvtype_extent;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Status status;
    int src, dst, rank;
    char *sendaddr, *recvaddr;

    local_size = comm_ptr->local_size; 
    remote_size = comm_ptr->remote_size;
    rank = comm_ptr->rank;

    /* Get extent of send and recv types */
    MPID_Datatype_get_extent_macro(sendtype, sendtype_extent);
    MPID_Datatype_get_extent_macro(recvtype, recvtype_extent);
    
    /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER( comm_ptr );

    /* Do the pairwise exchanges */
    max_size = MPIR_MAX(local_size, remote_size);
    MPIU_Ensure_Aint_fits_in_pointer(MPIU_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
				     max_size*recvcount*recvtype_extent);
    MPIU_Ensure_Aint_fits_in_pointer(MPIU_VOID_PTR_CAST_TO_MPI_AINT sendbuf +
				     max_size*sendcount*sendtype_extent);
    for (i=0; i<max_size; i++) {
        src = (rank - i + max_size) % max_size;
        dst = (rank + i) % max_size;
        if (src >= remote_size) {
            src = MPI_PROC_NULL;
            recvaddr = NULL;
        }
        else {
            recvaddr = (char *)recvbuf + src*recvcount*recvtype_extent;
        }
        if (dst >= remote_size) {
            dst = MPI_PROC_NULL;
            sendaddr = NULL;
        }
        else {
            sendaddr = (char *)sendbuf + dst*sendcount*sendtype_extent;
        }

        mpi_errno = MPIC_Sendrecv(sendaddr, sendcount, sendtype, dst,
                                     MPIR_ALLTOALL_TAG, recvaddr,
                                     recvcount, recvtype, src,
                                     MPIR_ALLTOALL_TAG, comm_ptr, &status, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

    /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT( comm_ptr );
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");

    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIR_Alltoall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  MPID_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;
        
    if (comm_ptr->comm_kind == MPID_INTRACOMM) {
        /* intracommunicator */
        mpi_errno = MPIR_Alltoall_intra(sendbuf, sendcount, sendtype,
                                        recvbuf, recvcount, recvtype,
                                        comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    } else {
        /* intercommunicator */
        mpi_errno = MPIR_Alltoall_inter(sendbuf, sendcount, sendtype,
                                        recvbuf, recvcount, recvtype,
                                        comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Alltoall_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Alltoall_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                       void *recvbuf, int recvcount, MPI_Datatype recvtype,
                       MPID_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Alltoall != NULL) {
	/* --BEGIN USEREXTENSION-- */
	mpi_errno = comm_ptr->coll_fns->Alltoall(sendbuf, sendcount, sendtype,
                                                 recvbuf, recvcount, recvtype,
                                                 comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
	/* --END USEREXTENSION-- */
    } else {
        mpi_errno = MPIR_Alltoall(sendbuf, sendcount, sendtype,
                                  recvbuf, recvcount, recvtype,
                                  comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#endif

#undef FUNCNAME
#define FUNCNAME MPI_Alltoall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Alltoall - Sends data from all to all processes

Input Parameters:
+ sendbuf - starting address of send buffer (choice) 
. sendcount - number of elements to send to each process (integer) 
. sendtype - data type of send buffer elements (handle) 
. recvcount - number of elements received from any process (integer) 
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
int MPI_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype,
                 MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ALLTOALL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_ALLTOALL);

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
	    
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

            if (sendbuf != MPI_IN_PLACE) {
                MPIR_ERRTEST_COUNT(sendcount, mpi_errno);
                MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                    MPID_Datatype_get_ptr(sendtype, sendtype_ptr);
                    MPID_Datatype_valid_ptr( sendtype_ptr, mpi_errno );
                    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    MPID_Datatype_committed_ptr( sendtype_ptr, mpi_errno );
                    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                }

                if (comm_ptr->comm_kind == MPID_INTRACOMM &&
                        sendbuf != MPI_IN_PLACE &&
                        sendcount == recvcount &&
                        sendtype == recvtype &&
                        sendcount != 0)
                    MPIR_ERRTEST_ALIAS_COLL(sendbuf,recvbuf,mpi_errno);
            }

	    MPIR_ERRTEST_COUNT(recvcount, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
            if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype_get_ptr(recvtype, recvtype_ptr);
                MPID_Datatype_valid_ptr( recvtype_ptr, mpi_errno );
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                MPID_Datatype_committed_ptr( recvtype_ptr, mpi_errno );
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }

            if (comm_ptr->comm_kind == MPID_INTERCOMM) {
                MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sendcount, mpi_errno);
            }
            MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcount, mpi_errno);
            MPIR_ERRTEST_USERBUFFER(sendbuf,sendcount,sendtype,mpi_errno);
	    MPIR_ERRTEST_USERBUFFER(recvbuf,recvcount,recvtype,mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Alltoall_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm_ptr, &errflag);
    if (mpi_errno) goto fn_fail;

    /* ... end of body of routine ... */
    
  fn_exit:
    MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLTOALL);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_alltoall",
	    "**mpi_alltoall %p %d %D %p %d %D %C", sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#if ( BORINGSSL_LIB)
/* variable nonce */
int MPI_SEC_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
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

	unsigned long count=0;
    unsigned long next, dest;
	unsigned long  t=0;
    t = (unsigned long )(sendtype_sz*sendcount);
    unsigned long   max_out_len = (unsigned long) (16 + (sendtype_sz*sendcount));
    unsigned int j ;
    for( j = 0; j < comm_ptr->local_size; j++){
        next =(unsigned long )(j*(sendcount*sendtype_sz));
        dest =(unsigned long )(j*((sendcount*sendtype_sz)+16+12));

        /* Set the nonce in alltoall_ciphertext */
        RAND_bytes(&alltoall_ciphertext_sendbuf[dest], 12); // 12 bytes of nonce
        /*nonceCounter++;
        memset(&alltoall_ciphertext_sendbuf[dest], 0, 8);
        alltoall_ciphertext_sendbuf[dest+8] = (nonceCounter >> 24) & 0xFF;
        alltoall_ciphertext_sendbuf[dest+9] = (nonceCounter >> 16) & 0xFF;
        alltoall_ciphertext_sendbuf[dest+10] = (nonceCounter >> 8) & 0xFF;
        alltoall_ciphertext_sendbuf[dest+11] = nonceCounter & 0xFF;*/
       
        if(!EVP_AEAD_CTX_seal(ctx, (alltoall_ciphertext_sendbuf+dest+12),
                         &ciphertext_sendbuf_len, max_out_len,
                         (alltoall_ciphertext_sendbuf+dest), 12,
                         sendbuf+next, t,
                        NULL, 0)){
              printf("Error in encryption: alltoall\n");
              fflush(stdout);
        }         
            
     }        
               
  
    mpi_errno=MPI_Alltoall(alltoall_ciphertext_sendbuf, ciphertext_sendbuf_len+12, MPI_CHAR,
                  alltoall_ciphertext_recvbuf, ((recvcount*recvtype_sz) + 16 +12), MPI_CHAR, comm);

 
 
    unsigned int i;
    for(i = 0; i < comm_ptr->local_size; i++){
        next =(unsigned long )(i*((recvcount*recvtype_sz) + 16 + 12));
        dest =(unsigned long )(i*(recvcount*recvtype_sz));
       
        if(!EVP_AEAD_CTX_open(ctx, ((recvbuf+dest)),
                        &count, (unsigned long )((recvcount*recvtype_sz)+16),
                        (alltoall_ciphertext_recvbuf+next), 12,
                        (alltoall_ciphertext_recvbuf+next+12), (unsigned long )((recvcount*recvtype_sz)+16),
                        NULL, 0)){
                    printf("Decryption error alltoall\n");fflush(stdout);        
            }       
        
    }
    
    return mpi_errno;
}
#elif ( OPENSSL_LIB)
int MPI_SEC_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,  MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_BCAST);

	int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	
	int sendtype_sz, recvtype_sz;
	
	MPI_Type_size(sendtype, &sendtype_sz);
    MPI_Type_size(recvtype, &recvtype_sz);
	
	unsigned long long blocktype_send= (unsigned long long) sendtype_sz*sendcount;
	unsigned long long blocktype_recv= (unsigned long long) recvtype_sz*recvcount;
    
    //unsigned char * ciphertext_send;
	//unsigned char * ciphertext_recv;
	unsigned long long dest, src;
	//ciphertext_send=(char*) MPIU_Malloc((blocktype_send+32) * world_size);		
	//ciphertext_recv=(char*) MPIU_Malloc((blocktype_recv+32) * world_size);	
	
	int i;
	for (i=0; i<world_size ; i++ ){

		dest = (unsigned long long) i*blocktype_send; // Message sent
		src =(unsigned long long) i*(blocktype_send+16+12); //Real send cipher 
        //EVP_EncryptUpdate(ctx_enc, send_ciphertext+12+src, &outlen_enc, sendbuf+dest, blocktype_send);
		openssl_enc_core(ciphertext_send,src,sendbuf,dest,blocktype_send);
		
	}
		
	MPI_Alltoall(ciphertext_send, (blocktype_send+16+12), MPI_CHAR, ciphertext_recv,blocktype_recv+16+12, MPI_CHAR, comm);
	//int i;
	for ( i=0; i<world_size ; i++ ){

		dest = (unsigned int) i*blocktype_recv;
		src =(unsigned int)i*(blocktype_recv+16+12);
        //EVP_DecryptUpdate(ctx_dec, recvbuf+dest, &outlen_dec, ciphertext_recv+12+src, (blocktype_recv-12));	
		openssl_dec_core(ciphertext_recv,src,recvbuf,dest,blocktype_recv+12);	
	}
	
		
	//MPIU_Free(ciphertext_send);
	//MPIU_Free(ciphertext_recv);
	
	return mpi_errno;

}
#elif ( LIBSODIUM_LIB)
/* This implementation is using variable nonce */
int MPI_SEC_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
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
    t = (unsigned long long)(sendtype_sz*sendcount);
    //printf("t=%llu ciphertext_sendbuf_len=%llu\n",t,ciphertext_sendbuf_len);
	unsigned int j;
    for( j = 0; j < comm_ptr->local_size; j++){
        next =(unsigned long long)(j*(sendcount*sendtype_sz));
        dest =(unsigned long long)(j*((sendcount*sendtype_sz)+16+12));

        /* Set the nonce in send_ciphertext */
        randombytes_buf(&alltoall_ciphertext_sendbuf[dest], 12);
        
        /*nonceCounter++;
        memset(&alltoall_ciphertext_sendbuf[dest], 0, 8);
        alltoall_ciphertext_sendbuf[dest+8] = (nonceCounter >> 24) & 0xFF;
        alltoall_ciphertext_sendbuf[dest+9] = (nonceCounter >> 16) & 0xFF;
        alltoall_ciphertext_sendbuf[dest+10] = (nonceCounter >> 8) & 0xFF;
        alltoall_ciphertext_sendbuf[dest+11] = nonceCounter & 0xFF;*/
        
        var = crypto_aead_aes256gcm_encrypt_afternm((alltoall_ciphertext_sendbuf+dest+12), &ciphertext_sendbuf_len,
            sendbuf+next, t,
            NULL, 0,
            NULL, alltoall_ciphertext_sendbuf+dest, (const crypto_aead_aes256gcm_state *) &ctx);
        if(var != 0)
            printf("Encryption failed\n");fflush(stdout);      
         //printf(":: rank=%d will send %u bytes, status=%d \n", comm_ptr->rank, ciphertext_sendbuf_len, var); fflush(stdout);    
     }        
               
   // printf(":: rank=%d will send %u bytes, status=%d \n", comm_ptr->rank, ciphertext_sendbuf_len, var); fflush(stdout);
    
   //unsigned long long procees_number = (unsigned long long )comm_ptr->local_size;
    //unsigned long long cipher_length = ciphertext_sendbuf_len * procees_number;
    mpi_errno=MPI_Alltoall(alltoall_ciphertext_sendbuf, ciphertext_sendbuf_len+12, MPI_CHAR,
                  alltoall_ciphertext_recvbuf, ((recvcount*recvtype_sz) + 16 +12), MPI_CHAR, comm);

 
 
    unsigned int i;
    for( i = 0; i < comm_ptr->local_size; i++){
        next =(unsigned long long)(i*((recvcount*recvtype_sz) + 16 + 12));
        dest =(unsigned long long)(i*(recvcount*recvtype_sz));
       // printf("next=%llu dest=%llu\n",next,dest);fflush(stdout);
        var = crypto_aead_aes256gcm_decrypt_afternm(((recvbuf+dest)), &count,
                                  NULL,
                                  (alltoall_ciphertext_recvbuf+next+12), (unsigned long long)((recvcount*recvtype_sz)+16),
                                  NULL,
                                  0,
                                  (alltoall_ciphertext_recvbuf+next),(const crypto_aead_aes256gcm_state *) &ctx);
        if(var != 0)
            printf("Decryption failed\n");fflush(stdout);  
        //printf(":: rank=%d receive %llu bytes, status=%d %02x\n", comm_ptr->rank, count, var, *((unsigned char *)(recvbuf+ dest))); fflush(stdout);
    }
    
   
   return mpi_errno;
}

#elif ( CRYPTOPP_LIB)
/* This implementation is using variable nonce */
int MPI_SEC_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
	int var;
	
    int sendtype_sz, recvtype_sz;
    unsigned long ciphertext_sendbuf_len = 0;
    sendtype_sz= recvtype_sz= 0;

    var=MPI_Type_size(sendtype, &sendtype_sz);
    var=MPI_Type_size(recvtype, &recvtype_sz);

    MPID_Comm_get_ptr( comm, comm_ptr);
	int rank;
	rank = comm_ptr->rank;

	unsigned long count=0;
    unsigned int next, dest;
	unsigned long t=0;
    t = (unsigned long)(sendtype_sz*sendcount);
    //printf("t=%llu ciphertext_sendbuf_len=%llu\n",t,ciphertext_sendbuf_len);
	unsigned int j ;
    for(j = 0; j < comm_ptr->local_size; j++){
        next =(unsigned long)(j*(sendcount*sendtype_sz));
        dest =(unsigned long)(j*((sendcount*sendtype_sz)+12+12));

        /* Set the nonce in send_ciphertext */
        wrapper_nonce(&alltoall_ciphertext_sendbuf[dest], 12);
        
       /*  nonceCounter++;
        memset(&alltoall_ciphertext_sendbuf[dest], 0, 8);
        alltoall_ciphertext_sendbuf[dest+8] = (nonceCounter >> 24) & 0xFF;
        alltoall_ciphertext_sendbuf[dest+9] = (nonceCounter >> 16) & 0xFF;
        alltoall_ciphertext_sendbuf[dest+10] = (nonceCounter >> 8) & 0xFF;
        alltoall_ciphertext_sendbuf[dest+11] = nonceCounter & 0xFF; */
        
        /* var = crypto_aead_aes256gcm_encrypt_afternm((alltoall_ciphertext_sendbuf+dest+12), &ciphertext_sendbuf_len,
            sendbuf+next, t,
            NULL, 0,
            NULL, alltoall_ciphertext_sendbuf+dest, (const crypto_aead_aes256gcm_state *) &ctx); */
		
		//wrapper_encrypt( char *sendbuf ,  char *ciphertext , int datacount, unsigned long key_size, unsigned char *key, unsigned char *nonce)
		wrapper_encrypt( sendbuf+next ,  alltoall_ciphertext_sendbuf+dest+12 , sendtype_sz*sendcount, key_size, gcm_key, alltoall_ciphertext_sendbuf+dest);
        
     }        
   
    mpi_errno=MPI_Alltoall(alltoall_ciphertext_sendbuf, sendtype_sz*sendcount+12+12, MPI_CHAR,
                  alltoall_ciphertext_recvbuf, ((recvcount*recvtype_sz) + 12 +12), MPI_CHAR, comm);
 
    unsigned int i;
    for(i = 0; i < comm_ptr->local_size; i++){
        next =(unsigned long)(i*((recvcount*recvtype_sz) + 12 + 12));
        dest =(unsigned long)(i*(recvcount*recvtype_sz));
       
        // var = crypto_aead_aes256gcm_decrypt_afternm(((recvbuf+dest)), &count,
                                  // NULL,
                                  // (alltoall_ciphertext_recvbuf+next+12), (unsigned long long)((recvcount*recvtype_sz)+16),
                                  // NULL,
                                  // 0,
                                  // (alltoall_ciphertext_recvbuf+next),(const crypto_aead_aes256gcm_state *) &ctx);
       //wrapper_decrypt( char *ciphertext ,  char *recvbuf, int datacount+tagsize, unsigned long key_size, unsigned char *key, unsigned char *nonce)
		
		wrapper_decrypt( alltoall_ciphertext_recvbuf+next+12,  recvbuf+dest, (recvtype_sz*recvcount)+12, key_size, gcm_key,alltoall_ciphertext_recvbuf+next);
    }
    
   
   return mpi_errno;
}
#endif