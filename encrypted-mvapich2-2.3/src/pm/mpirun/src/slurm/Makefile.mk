##
## Copyright (c) 2001-2018, The Ohio State University. All rights
## reserved.
##
## This file is part of the MVAPICH2 software package developed by the
## team members of The Ohio State University's Network-Based Computing
## Laboratory (NBCL), headed by Professor Dhabaleswar K. (DK) Panda.
##
## For detailed copyright and licensing information, please refer to the
## copyright file COPYRIGHT in the top level MVAPICH2 directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch3/channels/common/include
AM_CPPFLAGS += -I$(top_srcdir)/src/pm/mpirun  -I$(top_srcdir)/src/pm/mpirun/src -I$(top_srcdir)/src/pm/mpirun/src/slurm

noinst_LIBRARIES += src/pm/mpirun/src/slurm/libslurm.a 	\
					src/pm/mpirun/src/slurm/libnodelist.a 	\
					src/pm/mpirun/src/slurm/libtasklist.a

BUILT_SOURCES += src/pm/mpirun/src/slurm/libnodelist_a-nodelist_parser.h \
				 src/pm/mpirun/src/slurm/libtasklist_a-tasklist_parser.h

src_pm_mpirun_src_slurm_libslurm_a_SOURCES = 	\
	src/pm/mpirun/src/slurm/slurm_startup.c

src_pm_mpirun_src_slurm_libnodelist_a_SOURCES = 	\
	src/pm/mpirun/src/slurm/nodelist_parser.y 	\
	src/pm/mpirun/src/slurm/nodelist_scanner.l 	\
	src/pm/mpirun/src/slurm/suffixlist.c

src_pm_mpirun_src_slurm_libtasklist_a_SOURCES = 	\
	src/pm/mpirun/src/slurm/tasklist_parser.y 		\
	src/pm/mpirun/src/slurm/tasklist_scanner.l
