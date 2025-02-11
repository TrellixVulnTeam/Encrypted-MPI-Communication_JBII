/*
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

#define PSM__RI__2PPN {		\
	{		\
	  2,		\
	  20,		\
	  {		\
	    {1, &MPIR_Gather_MV2_Direct},		\
	    {2, &MPIR_Gather_MV2_Direct},		\
	    {4, &MPIR_Gather_MV2_Direct},		\
	    {8, &MPIR_Gather_MV2_Direct},		\
	    {16, &MPIR_Gather_MV2_Direct},		\
	    {32, &MPIR_Gather_MV2_Direct},		\
	    {64, &MPIR_Gather_MV2_Direct},		\
	    {128, &MPIR_Gather_MV2_Direct},		\
	    {256, &MPIR_Gather_MV2_Direct},		\
	    {512, &MPIR_Gather_MV2_Direct},		\
	    {1024, &MPIR_Gather_intra},		\
	    {2048, &MPIR_Gather_intra},		\
	    {4096, &MPIR_Gather_intra},		\
	    {8192, &MPIR_Gather_intra},		\
	    {16384, &MPIR_Gather_MV2_Direct},		\
	    {32768, &MPIR_Gather_intra},		\
	    {65536, &MPIR_Gather_intra},		\
	    {131072, &MPIR_Gather_intra},		\
	    {262144, &MPIR_Gather_intra},		\
	    {524288, &MPIR_Gather_MV2_Direct},		\
	    {1048576, &MPIR_Gather_intra}		\
	  },		\
	  20,		\
	  {		\
	    {1, &MPIR_Gather_MV2_Direct},		\
	    {2, &MPIR_Gather_MV2_Direct},		\
	    {4, &MPIR_Gather_MV2_Direct},		\
	    {8, &MPIR_Gather_MV2_Direct},		\
	    {16, &MPIR_Gather_MV2_Direct},		\
	    {32, &MPIR_Gather_MV2_Direct},		\
	    {64, &MPIR_Gather_MV2_Direct},		\
	    {128, &MPIR_Gather_MV2_Direct},		\
	    {256, &MPIR_Gather_MV2_Direct},		\
	    {512, &MPIR_Gather_MV2_Direct},		\
	    {1024, &MPIR_Gather_MV2_Direct},		\
	    {2048, &MPIR_Gather_MV2_Direct},		\
	    {4096, &MPIR_Gather_MV2_Direct},		\
	    {8192, &MPIR_Gather_MV2_Direct},		\
	    {16384, &MPIR_Gather_MV2_Direct},		\
	    {32768, &MPIR_Gather_MV2_Direct},		\
	    {65536, &MPIR_Gather_MV2_Direct},		\
	    {131072, &MPIR_Gather_MV2_Direct},		\
	    {262144, &MPIR_Gather_MV2_Direct},		\
	    {524288, &MPIR_Gather_MV2_Direct},		\
	    {1048576, &MPIR_Gather_MV2_Direct}		\
	  }		\
	},		\
	{		\
	  4,		\
	  20,		\
	  {		\
	    {1, &MPIR_Gather_MV2_two_level_Direct},		\
	    {2, &MPIR_Gather_MV2_Direct},		\
	    {4, &MPIR_Gather_MV2_two_level_Direct},		\
	    {8, &MPIR_Gather_MV2_Direct},		\
	    {16, &MPIR_Gather_MV2_Direct},		\
	    {32, &MPIR_Gather_MV2_Direct},		\
	    {64, &MPIR_Gather_MV2_Direct},		\
	    {128, &MPIR_Gather_MV2_Direct},		\
	    {256, &MPIR_Gather_MV2_Direct},		\
	    {512, &MPIR_Gather_MV2_Direct},		\
	    {1024, &MPIR_Gather_MV2_Direct},		\
	    {2048, &MPIR_Gather_MV2_Direct},		\
	    {4096, &MPIR_Gather_MV2_Direct},		\
	    {8192, &MPIR_Gather_MV2_Direct},		\
	    {16384, &MPIR_Gather_MV2_Direct},		\
	    {32768, &MPIR_Gather_MV2_Direct},		\
	    {65536, &MPIR_Gather_MV2_Direct},		\
	    {131072, &MPIR_Gather_MV2_Direct},		\
	    {262144, &MPIR_Gather_MV2_Direct},		\
	    {524288, &MPIR_Gather_MV2_Direct},		\
	    {1048576, &MPIR_Gather_MV2_Direct}		\
	  },		\
	  20,		\
	  {		\
	    {1, &MPIR_Gather_MV2_Direct},		\
	    {2, &MPIR_Gather_MV2_Direct},		\
	    {4, &MPIR_Gather_MV2_Direct},		\
	    {8, &MPIR_Gather_MV2_Direct},		\
	    {16, &MPIR_Gather_MV2_Direct},		\
	    {32, &MPIR_Gather_MV2_Direct},		\
	    {64, &MPIR_Gather_MV2_Direct},		\
	    {128, &MPIR_Gather_MV2_Direct},		\
	    {256, &MPIR_Gather_MV2_Direct},		\
	    {512, &MPIR_Gather_MV2_Direct},		\
	    {1024, &MPIR_Gather_MV2_Direct},		\
	    {2048, &MPIR_Gather_MV2_Direct},		\
	    {4096, &MPIR_Gather_MV2_Direct},		\
	    {8192, &MPIR_Gather_MV2_Direct},		\
	    {16384, &MPIR_Gather_MV2_Direct},		\
	    {32768, &MPIR_Gather_MV2_Direct},		\
	    {65536, &MPIR_Gather_MV2_Direct},		\
	    {131072, &MPIR_Gather_MV2_Direct},		\
	    {262144, &MPIR_Gather_MV2_Direct},		\
	    {524288, &MPIR_Gather_MV2_Direct},		\
	    {1048576, &MPIR_Gather_MV2_Direct}		\
	  }		\
	}		\
};		
