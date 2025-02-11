#ifndef MPIRUN_ENVIRON_H
#define MPIRUN_ENVIRON_H 1
/* Copyright (c) 2001-2018, The Ohio State University. All rights
 * reserved.
 *
 * This file is part of the MVAPICH2 software package developed by the
 * team members of The Ohio State University's Network-Based Computing
 * Laboratory (NBCL), headed by Professor Dhabaleswar K. (DK) Panda.
 *
 * For detailed copyright and licensing information, please refer to the
 * copyright file COPYRIGHT in the top level MVAPICH2 directory.
 */

void enable_send_environ (int overwrite);
int send_environ (int socket);
int recv_environ (int socket);

#endif
