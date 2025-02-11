/* Copyright (c) 2001-2018, The Ohio State University. All rights
 * reserved.
 *
 * This file is part of the MVAPICH2 software package developed by the
 * team members of The Ohio State University's Network-Based Computing
 * Laboratory (NBCL), headed by Professor Dhabaleswar K. (DK) Panda.
 *
 * For detailed copyright and licensing information, please refer to the
 * copyright file COPYRIGHT in the top level MVAPICH2 directory.
 *
 */

#ifndef _IALLGATHERV_TUNING_
#define _IALLGATHERV_TUNING_

#include "coll_shmem.h"
#if defined(CHANNEL_MRAIL)
#include "ibv_param.h"
#endif                          /* #if defined(CHANNEL_MRAIL) */

#define NMATCH (3+1)

/* Note: Several members of the structures used are meant to be used
   sometime in the future */

typedef struct {
    int min;
    int max;
  int (*MV2_pt_Iallgatherv_function) (const void *sendbuf, int sendcount, MPI_Datatype sendtype,
				      void *recvbuf, const int recvcounts[], const int displs[],
				      MPI_Datatype recvtype, MPID_Comm *comm_ptr, MPID_Sched_t s);
    int zcopy_factor;
} mv2_iallgatherv_tuning_element;

typedef struct {
    int numproc;
    int iallgatherv_segment_size;
    int is_two_level_iallgatherv[MV2_MAX_NB_THRESHOLDS];
    int size_inter_table;
    mv2_iallgatherv_tuning_element inter_leader[MV2_MAX_NB_THRESHOLDS];
    int size_intra_table;
    mv2_iallgatherv_tuning_element intra_node[MV2_MAX_NB_THRESHOLDS];
} mv2_iallgatherv_tuning_table;

//extern int mv2_use_pipelined_allgatherv;
//extern int mv2_pipelined_knomial_factor; 
//extern int mv2_pipelined_zcpy_knomial_factor; 
//extern int zcpy_knomial_factor;
extern int iallgatherv_segment_size;
extern int mv2_size_iallgatherv_tuning_table;
extern mv2_iallgatherv_tuning_table *mv2_iallgatherv_thresholds_table;

/* Architecture detection tuning */
int MV2_set_iallgatherv_tuning_table(int heterogeneity);

/* Function to clean free memory allocated by iallgatherv tuning table*/
void MV2_cleanup_iallgatherv_tuning_table();

// Consider removing
/* Function used inside ch3_shmem_coll.c to tune iallgatherv thresholds */
int MV2_internode_Iallgatherv_is_define(char *mv2_user_iallgatherv_inter, char *mv2_user_iallgatherv_intra); 
int MV2_intranode_Iallgatherv_is_define(char *mv2_user_iallgatherv_intra);

extern int MPIR_Iallgatherv_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
				 void *recvbuf, const int recvcounts[], const int displs[],
				 MPI_Datatype recvtype, MPID_Comm *comm_ptr, MPID_Sched_t s);
extern int MPIR_Iallgatherv_bruck(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
				  void *recvbuf, const int recvcounts[], const int displs[],
				  MPI_Datatype recvtype, MPID_Comm *comm_ptr, MPID_Sched_t s);
extern int MPIR_Iallgatherv_rec_dbl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
				    void *recvbuf, const int recvcounts[], const int displs[],
				    MPI_Datatype recvtype, MPID_Comm *comm_ptr, MPID_Sched_t s);
#endif
