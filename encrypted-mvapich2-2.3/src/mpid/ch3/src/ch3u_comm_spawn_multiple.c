/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 * Copyright (c) 2001-2018, The Ohio State University. All rights
 * reserved.
 *
 * This file is part of the MVAPICH2 software package developed by the
 * team members of The Ohio State University's Network-Based Computing
 * Laboratory (NBCL), headed by Professor Dhabaleswar K. (DK) Panda.
 *
 * For detailed copyright and licensing information, please refer to the
 * copyright file COPYRIGHT in the top level MVAPICH2 directory.
 */


#include "mpidi_ch3_impl.h"

/* FIXME:
   Place all of this within the mpid_comm_spawn_multiple file */

#ifndef MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS
/* 
 * We require support for the PMI calls.  If a channel cannot support
 * a PMI call, it should provide a stub and return an error code.
 */
   

#include "upmi.h"

/* Define the name of the kvs key used to provide the port name to the
   children */
#define PARENT_PORT_KVSKEY "PARENT_ROOT_PORT_NAME"

/* FIXME: We can avoid these two routines if we define PMI as using 
   MPI info values */
/* Turn a SINGLE MPI_Info into an array of PMI_keyvals (return the pointer
   to the array of PMI keyvals) */
#undef FUNCNAME
#define FUNCNAME mpi_to_pmi_keyvals
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int  mpi_to_pmi_keyvals( MPID_Info *info_ptr, PMI_keyval_t **kv_ptr, 
				int *nkeys_ptr )
{
    char key[MPI_MAX_INFO_KEY];
    PMI_keyval_t *kv = 0;
    int          i, nkeys = 0, vallen, flag, mpi_errno=MPI_SUCCESS;

    if (!info_ptr || info_ptr->handle == MPI_INFO_NULL) {
	goto fn_exit;
    }

    MPIR_Info_get_nkeys_impl( info_ptr, &nkeys );
    if (nkeys == 0) {
	goto fn_exit;
    }
    kv = (PMI_keyval_t *)MPIU_Malloc( nkeys * sizeof(PMI_keyval_t) );
    if (!kv) { MPIR_ERR_POP(mpi_errno); }

    for (i=0; i<nkeys; i++) {
	mpi_errno = MPIR_Info_get_nthkey_impl( info_ptr, i, key );
	if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }
	MPIR_Info_get_valuelen_impl( info_ptr, key, &vallen, &flag );
        MPIR_ERR_CHKANDJUMP1(!flag, mpi_errno, MPI_ERR_OTHER,"**infonokey", "**infonokey %s", key);

	kv[i].key = MPIU_Strdup(key);
	kv[i].val = MPIU_Malloc( vallen + 1 );
	if (!kv[i].key || !kv[i].val) { 
	    MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem" );
	}
	MPIR_Info_get_impl( info_ptr, key, vallen+1, kv[i].val, &flag );
        MPIR_ERR_CHKANDJUMP1(!flag, mpi_errno, MPI_ERR_OTHER,"**infonokey", "**infonokey %s", key);
	MPIU_DBG_PRINTF(("key: <%s>, value: <%s>\n", kv[i].key, kv[i].val));
    }

 fn_fail:
 fn_exit:
    *kv_ptr    = kv;
    *nkeys_ptr = nkeys;
    return mpi_errno;
}
/* Free the entire array of PMI keyvals */
static void free_pmi_keyvals(PMI_keyval_t **kv, int size, int *counts)
{
    int i,j;

    for (i=0; i<size; i++)
    {
	for (j=0; j<counts[i]; j++)
	{
	    if (kv[i][j].key != NULL)
		MPIU_Free(kv[i][j].key);
	    if (kv[i][j].val != NULL)
		MPIU_Free(kv[i][j].val);
	}
	if (kv[i] != NULL)
	{
	    MPIU_Free(kv[i]);
	}
    }
}

/*
 * MPIDI_CH3_Comm_spawn_multiple()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_Comm_spawn_multiple
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_Comm_spawn_multiple(int count, char **commands,
                                  char ***argvs, const int *maxprocs,
                                  MPID_Info **info_ptrs, int root,
                                  MPID_Comm *comm_ptr, MPID_Comm
                                  **intercomm, int *errcodes) 
{
    char port_name[MPI_MAX_PORT_NAME];
    int *info_keyval_sizes=0, i, mpi_errno=MPI_SUCCESS;
    PMI_keyval_t **info_keyval_vectors=0, preput_keyval_vector;
    int *pmi_errcodes = 0, pmi_errno;
    int total_num_processes, should_accept = 1;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_COMM_SPAWN_MULTIPLE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_COMM_SPAWN_MULTIPLE);


    if (comm_ptr->rank == root) {
	/* create an array for the pmi error codes */
	total_num_processes = 0;
	for (i=0; i<count; i++) {
	    total_num_processes += maxprocs[i];
	}
	pmi_errcodes = (int*)MPIU_Malloc(sizeof(int) * total_num_processes);
	if (pmi_errcodes == NULL) {
	    MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem");
	}

	/* initialize them to 0 */
	for (i=0; i<total_num_processes; i++)
	    pmi_errcodes[i] = 0;

	/* Open a port for the spawned processes to connect to */
	/* FIXME: info may be needed for port name */
        mpi_errno = MPID_Open_port(NULL, port_name);
	/* --BEGIN ERROR HANDLING-- */
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
	/* --END ERROR HANDLING-- */

	/* Spawn the processes */
#ifdef USE_PMI2_API
        MPIU_Assert(count > 0);
        {
            int *argcs = MPIU_Malloc(count*sizeof(int));
            struct MPID_Info preput;
            struct MPID_Info *preput_p[1] = { &preput };

            MPIU_Assert(argcs);
            /*
            info_keyval_sizes = MPIU_Malloc(count * sizeof(int));
            */

            /* FIXME cheating on constness */
            preput.key = (char *)PARENT_PORT_KVSKEY;
            preput.value = port_name;
            preput.next = NULL;

            /* compute argcs array */
            for (i = 0; i < count; ++i) {
                argcs[i] = 0;
                if (argvs != NULL && argvs[i] != NULL) {
                    while (argvs[i][argcs[i]]) {
                        ++argcs[i];
                    }
                }

                /* a fib for now */
                /*
                info_keyval_sizes[i] = 0;
                */
            }
            /* XXX DJG don't need this, PMI API is thread-safe? */
            /*MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_PMI_MUTEX);*/
            /* release the global CS for spawn PMI calls */
            MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
            pmi_errno = PMI2_Job_Spawn(count, (const char **)commands,
                                       argcs, (const char ***)argvs,
                                       maxprocs,
                                       info_keyval_sizes, (const MPID_Info **)info_ptrs,
                                       1, (const struct MPID_Info **)preput_p,
                                       NULL, 0,
                                       /*jobId, jobIdSize,*/ /* XXX DJG job stuff? */
                                       pmi_errcodes);
            MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
            /*MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_PMI_MUTEX);*/
            MPIU_Free(argcs);
            if (pmi_errno != PMI2_SUCCESS) {
                MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER,
                     "**pmi_spawn_multiple", "**pmi_spawn_multiple %d", pmi_errno);
            }
        }
#else
        /* FIXME: This is *really* awkward.  We should either
           Fix on MPI-style info data structures for PMI (avoid unnecessary
           duplication) or add an MPIU_Info_getall(...) that creates
           the necessary arrays of key/value pairs */

        /* convert the infos into PMI keyvals */
        info_keyval_sizes   = (int *) MPIU_Malloc(count * sizeof(int));
        info_keyval_vectors = 
            (PMI_keyval_t**) MPIU_Malloc(count * sizeof(PMI_keyval_t*));
        if (!info_keyval_sizes || !info_keyval_vectors) { 
            MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem");
        }

        if (!info_ptrs) {
            for (i=0; i<count; i++) {
                info_keyval_vectors[i] = 0;
                info_keyval_sizes[i]   = 0;
            }
        }
        else {
            for (i=0; i<count; i++) {
                mpi_errno = mpi_to_pmi_keyvals( info_ptrs[i], 
                                                &info_keyval_vectors[i],
                                                &info_keyval_sizes[i] );
                if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }
            }
        }

        preput_keyval_vector.key = PARENT_PORT_KVSKEY;
        preput_keyval_vector.val = port_name;


        MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_PMI_MUTEX);
        pmi_errno = UPMI_JOB_SPAWN(count, (const char **)
                                       commands, 
                                       NULL,
                                       (const char ***) argvs,
                                       maxprocs, info_keyval_sizes,
                                       (const void**)
                                       info_keyval_vectors, 1, 
                                       (const void*) &preput_keyval_vector,
                                       NULL, 0, 
                                       pmi_errcodes);
	MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_PMI_MUTEX);
        if (pmi_errno != PMI_SUCCESS) {
	    MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER,
		 "**pmi_spawn_multiple", "**pmi_spawn_multiple %d", pmi_errno);
        }
#endif

	if (errcodes != MPI_ERRCODES_IGNORE) {
	    for (i=0; i<total_num_processes; i++) {
		/* FIXME: translate the pmi error codes here */
		errcodes[i] = pmi_errcodes[i];
                /* We want to accept if any of the spawns succeeded.
                   Alternatively, this is the same as we want to NOT accept if
                   all of them failed.  should_accept = NAND(e_0, ..., e_n)
                   Remember, success equals false (0). */
                should_accept = should_accept && errcodes[i];
	    }
            should_accept = !should_accept; /* the `N' in NAND */
	}
    }

    if (errcodes != MPI_ERRCODES_IGNORE) {
        MPIR_Errflag_t errflag = MPIR_ERR_NONE;
        mpi_errno = MPIR_Bcast_impl(&should_accept, 1, MPI_INT, root, comm_ptr, &errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        mpi_errno = MPIR_Bcast_impl(&total_num_processes, 1, MPI_INT, root, comm_ptr, &errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        
        mpi_errno = MPIR_Bcast_impl(errcodes, total_num_processes, MPI_INT, root, comm_ptr, &errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    }

    if (should_accept) {
        mpi_errno = MPID_Comm_accept(port_name, NULL, root, comm_ptr, intercomm); 
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**pmi_spawn_multiple");
    }

    if (comm_ptr->rank == root) {
	/* Close the port opened for the spawned processes to connect to */
	mpi_errno = MPID_Close_port(port_name);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIR_ERR_POP(mpi_errno);
	}
	/* --END ERROR HANDLING-- */
    }

 fn_exit:
    if (info_keyval_vectors) {
	free_pmi_keyvals(info_keyval_vectors, count, info_keyval_sizes);
	MPIU_Free(info_keyval_sizes);
	MPIU_Free(info_keyval_vectors);
    }
    if (pmi_errcodes) {
	MPIU_Free(pmi_errcodes);
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_COMM_SPAWN_MULTIPLE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* FIXME: What does this function do?  Who calls it?  Can we assume that
   it is called only dynamic process operations (specifically spawn) 
   are supported?  Do we need the concept of a port? For example,
   could a channel that supported only shared memory call this (it doesn't
   look like it right now, so this could go into util/sock, perhaps?
   
   It might make more sense to have this function provided as a function 
   pointer as part of the channel init setup, particularly since this
   function appears to access channel-specific storage (MPIDI_CH3_Process) */


/* This function is used only with mpid_init to set up the parent communicator
   if there is one.  The routine should be in this file because the parent 
   port name is setup with the "preput" arguments to PMI_Spawn_multiple */
static char *parent_port_name = 0;    /* Name of parent port if this
					 process was spawned (and is root
					 of comm world) or null */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_GetParentPort
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_GetParentPort(char ** parent_port)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    char val[MPIDI_MAX_KVS_VALUE_LEN];

    if (parent_port_name == NULL)
    {
	char *kvsname = NULL;
	/* We can always use UPMI_KVS_GET on our own process group */
	MPIDI_PG_GetConnKVSname( &kvsname );
	MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_PMI_MUTEX);
	pmi_errno = UPMI_KVS_GET( kvsname, PARENT_PORT_KVSKEY, val, sizeof(val));
	MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_PMI_MUTEX);
	if (pmi_errno) {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvsget", "**pmi_kvsget %d", pmi_errno);
            goto fn_exit;
	}
	parent_port_name = MPIU_Strdup(val);
	if (parent_port_name == NULL) {
	    MPIR_ERR_POP(mpi_errno); /* FIXME DARIUS */
	}
    }

    *parent_port = parent_port_name;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
void MPIDI_CH3_FreeParentPort(void)
{
    if (parent_port_name) {
	MPIU_Free( parent_port_name );
	parent_port_name = 0;
    }
}
#endif
