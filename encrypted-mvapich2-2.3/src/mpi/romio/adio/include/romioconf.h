/* adio/include/romioconf.h.  Generated from romioconf.h.in by configure.  */
/* adio/include/romioconf.h.in.  Generated from configure.ac by autoheader.  */

/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef ROMIOCONF_H_INCLUDED
#define ROMIOCONF_H_INCLUDED


/* BGQ platform */
/* #undef BGQPLATFORM */

/* Define to 1 if you have the <aio.h> header file. */
#define HAVE_AIO_H 1

/* Define to 1 if you have the <aio-lite.h> header file. */
/* #undef HAVE_AIO_LITE_H */

/* Define to 1 if you have the declaration of `MPI_COMBINER_HINDEXED_BLOCK',
   and to 0 if you don't. */
#define HAVE_DECL_MPI_COMBINER_HINDEXED_BLOCK 1

/* Define to 1 if you have the declaration of `pwrite', and to 0 if you don't.
   */
#define HAVE_DECL_PWRITE 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the `fsync' function. */
#define HAVE_FSYNC 1

/* Define to 1 if you have the `ftruncate' function. */
#define HAVE_FTRUNCATE 1

/* Define if GNU __attribute__ is supported */
#define HAVE_GCC_ATTRIBUTE 1

/* Define to 1 if you have the <gpfs_fcntl.h> header file. */
/* #undef HAVE_GPFS_FCNTL_H */

/* Define to 1 if you have the <gpfs.h> header file. */
/* #undef HAVE_GPFS_H */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define if int smaller than pointer */
#define HAVE_INT_LT_POINTER 1

/* Define to 1 if you have the `aio-lite' library (-laio-lite). */
/* #undef HAVE_LIBAIO_LITE */

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define if long long is 64 bits */
#define HAVE_LONG_LONG_64 1

/* Define to 1 if you have the `lseek64' function. */
#define HAVE_LSEEK64 1

/* Define to 1 if you have the `lstat' function. */
#define HAVE_LSTAT 1

/* Define to 1 if you have the <lustre/lustre_user.h> header file. */
/* #undef HAVE_LUSTRE_LUSTRE_USER_H */

/* Define to 1 if you have the <malloc.h> header file. */
#define HAVE_MALLOC_H 1

/* Define to 1 if you have the `memalign' function. */
#define HAVE_MEMALIGN 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define if MPI-IO routines need a const qualifier */
#define HAVE_MPIIO_CONST const

/* Define if MPICH memory tracing macros defined */
#define HAVE_MPIU_FUNCS 1

/* Define to 1 if you have the <mpix.h> header file. */
#define HAVE_MPIX_H 1

/* Define if MPI combiners available */
/* #undef HAVE_MPI_COMBINERS */

/* Define if Darray is available */
#define HAVE_MPI_DARRAY_SUBARRAY 1

/* Define if generalized requests avaliable */
/* #undef HAVE_MPI_GREQUEST */

/* Define if MPI Info is available */
#define HAVE_MPI_INFO 1

/* Define if supports long long int */
#define HAVE_MPI_LONG_LONG_INT 1

/* Define to 1 if you have the `MPI_Status_set_elements_x' function. */
#define HAVE_MPI_STATUS_SET_ELEMENTS_X 1

/* Define to 1 if you have the `MPI_Type_size_x' function. */
#define HAVE_MPI_TYPE_SIZE_X 1

/* Define if multiple weak symbols may be defined */
#define HAVE_MULTIPLE_PRAGMA_WEAK 1

/* Define to 1 if the system has the type `pan_fs_client_raidn_encoding_t'. */
/* #undef HAVE_PAN_FS_CLIENT_RAIDN_ENCODING_T */

/* Cray style weak pragma */
/* #undef HAVE_PRAGMA_CRI_DUP */

/* HP style weak pragma */
/* #undef HAVE_PRAGMA_HP_SEC_DEF */

/* Supports weak pragma */
#define HAVE_PRAGMA_WEAK 1

/* Define if PVFS_sys_create does not have layout parameter */
/* #undef HAVE_PVFS2_CREATE_WITHOUT_LAYOUT */

/* Define to 1 if you have the <pvfs2.h> header file. */
/* #undef HAVE_PVFS2_H */

/* Define if PVFS2_SUPER_MAGIC defined. */
/* #undef HAVE_PVFS2_SUPER_MAGIC */

/* Define to 1 if you have the <pvfs.h> header file. */
/* #undef HAVE_PVFS_H */

/* Define if PVFS_SUPER_MAGIC defined. */
/* #undef HAVE_PVFS_SUPER_MAGIC */

/* Define to 1 if you have the `readlink' function. */
#define HAVE_READLINK 1

/* Define to 1 if you have the <signal.h> header file. */
#define HAVE_SIGNAL_H 1

/* Define to 1 if you have the `snprintf' function. */
#define HAVE_SNPRINTF 1

/* Define to 1 if you have the `stat' function. */
#define HAVE_STAT 1

/* Define to 1 if you have the `statfs' function. */
#define HAVE_STATFS 1

/* Define if status set bytes available */
#define HAVE_STATUS_SET_BYTES 1

/* Define to 1 if you have the `statvfs' function. */
#define HAVE_STATVFS 1

/* Define to 1 if you have the <stddef.h> header file. */
#define HAVE_STDDEF_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strdup' function. */
#define HAVE_STRDUP 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if `aio_fildes' is a member of `struct aiocb'. */
#define HAVE_STRUCT_AIOCB_AIO_FILDES 1

/* Define to 1 if `aio_handle' is a member of `struct aiocb'. */
/* #undef HAVE_STRUCT_AIOCB_AIO_HANDLE */

/* Define to 1 if `aio_reqprio' is a member of `struct aiocb'. */
#define HAVE_STRUCT_AIOCB_AIO_REQPRIO 1

/* Define to 1 if `aio_sigevent' is a member of `struct aiocb'. */
#define HAVE_STRUCT_AIOCB_AIO_SIGEVENT 1

/* Define to 1 if `aio_whence' is a member of `struct aiocb'. */
/* #undef HAVE_STRUCT_AIOCB_AIO_WHENCE */

/* Define if struct statfs can be compiled */
#define HAVE_STRUCT_STATFS 1

/* Define to 1 if you have the <sys/aio.h> header file. */
/* #undef HAVE_SYS_AIO_H */

/* Define to 1 if you have the <sys/attr.h> header file. */
/* #undef HAVE_SYS_ATTR_H */

/* Define to 1 if you have the <sys/mount.h> header file. */
#define HAVE_SYS_MOUNT_H 1

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/statvfs.h> header file. */
#define HAVE_SYS_STATVFS_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/vfs.h> header file. */
#define HAVE_SYS_VFS_H 1

/* Define to 1 if you have the <time.h> header file. */
#define HAVE_TIME_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `usleep' function. */
#define HAVE_USLEEP 1

/* Define to 1 if the system has the type `u_int64_t'. */
/* #undef HAVE_U_INT64_T */

/* Attribute style weak pragma */
#define HAVE_WEAK_ATTRIBUTE 1

/* Define if weak symbols available */
#define HAVE_WEAK_SYMBOLS 1

/* Define to 1 if you have the <zoidfs.h> header file. */
/* #undef HAVE_ZOIDFS_H */

/* Define if int smaller than pointer */
#define INT_LT_POINTER 1

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* Define if using MPICH */
/* #undef MPICH */

/* Define if using HP MPI */
/* #undef MPIHP */

/* Define if compiling within LAM/MPI */
/* #undef MPILAM */

/* hack to make ROMIO build without profiling */
/* #undef MPIO_BUILD_PROFILING */

/* Define if SGI MPI */
/* #undef MPISGI */

/* Define to "MPI_Aint" if MPI does not provide MPI_Count */
/* #undef MPI_Count */

/* Define if MPI_Offset is int */
/* #undef MPI_OFFSET_IS_INT */

/* Define if fsync needs a declaration */
/* #undef NEEDS_FSYNC_DECL */

/* Define if ftruncate needs a declaration */
/* #undef NEEDS_FTRUNCATE_DECL */

/* Define if l_start and l_len data should be cast as int */
/* #undef NEEDS_INT_CAST_WITH_FLOCK */

/* Define if lseek64 needs a declaration */
#define NEEDS_LSEEK64_DECL 1

/* Define if lstat needs a declaration */
/* #undef NEEDS_LSTAT_DECL */

/* Define if mpi_test needed */
/* #undef NEEDS_MPI_TEST */

/* Define if readlink needs a declaration */
/* #undef NEEDS_READLINK_DECL */

/* Define if snprintf needs a declaration */
/* #undef NEEDS_SNPRINTF_DECL */

/* Define if strdup needs a declaration */
/* #undef NEEDS_STRDUP_DECL */

/* Define if usleep needs a declaration */
/* #undef NEEDS_USLEEP_DECL */

/* Define if no MPI type is contig */
/* #undef NO_MPI_SGI_type_is_contig */

/* Name of package */
#define PACKAGE "romio"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "discuss@mpich.org"

/* Define to the full name of this package. */
#define PACKAGE_NAME "ROMIO"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "ROMIO 3.2.1"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "romio"

/* Define to the home page for this package. */
#define PACKAGE_URL "http://www.mpich.org/"

/* Define to the version of this package. */
#define PACKAGE_VERSION "3.2.1"

/* PE platform */
/* #undef PEPLATFORM */

/* Define for printing error messages */
/* #undef PRINT_ERR_MSG */

/* Define for ROMIO with GPFS */
/* #undef ROMIO_GPFS */

/* Define for ROMIO with gridftp */
/* #undef ROMIO_GRIDFTP */

/* Define if AIO calls need file descriptor */
/* #undef ROMIO_HAVE_AIO_CALLS_NEED_FILEDES */

/* Define if aio_suspend needs two arguments */
/* #undef ROMIO_HAVE_AIO_SUSPEND_TWO_ARGS */

/* Define if statfs has f_fstypename */
/* #undef ROMIO_HAVE_STRUCT_STATFS_WITH_F_FSTYPENAME */

/* defined if struct statvfs has a f_basetype member */
/* #undef ROMIO_HAVE_STRUCT_STATVFS_WITH_F_BASETYPE */

/* Define if struct stat has a st_fstype member */
/* #undef ROMIO_HAVE_STRUCT_STAT_WITH_ST_FSTYPE */

/* Define if AIO calls seem to work */
#define ROMIO_HAVE_WORKING_AIO 1

/* Define for ROMIO with HFS */
/* #undef ROMIO_HFS */

/* Define if compiling within MPICH */
#define ROMIO_INSIDE_MPICH 1

/* Define for ROMIO with LUSTRE */
/* #undef ROMIO_LUSTRE */

/* Define for ROMIO with NFS */
#define ROMIO_NFS 1

/* Define for ROMIO with PANFS */
/* #undef ROMIO_PANFS */

/* Define for ROMIO with PFS */
/* #undef ROMIO_PFS */

/* Define for ROMIO with PVFS */
/* #undef ROMIO_PVFS */

/* Define for ROMIO with PVFS2 */
/* #undef ROMIO_PVFS2 */

/* Define if int64_t must be defined for PVFS */
/* #undef ROMIO_PVFS_NEEDS_INT64_DEFINITION */

/* Define if run on Linux */
#define ROMIO_RUN_ON_LINUX 1

/* Define for ROMIO with SFS */
/* #undef ROMIO_SFS */

/* Define for ROMIO with TESTFS */
#define ROMIO_TESTFS 1

/* Define for ROMIO with UFS */
#define ROMIO_UFS 1

/* Define for ROMIO with XFS */
/* #undef ROMIO_XFS */

/* Define for ROMIO with ZoidFD */
/* #undef ROMIO_ZOIDFS */

/* The size of `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of `long long', as computed by sizeof. */
#define SIZEOF_LONG_LONG 8

/* The size of `void *', as computed by sizeof. */
#define SIZEOF_VOID_P 8

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define if performing coverage tests */
/* #undef USE_COVERAGE */

/* Define if weak symbols should be used */
#define USE_WEAK_SYMBOLS 1

/* Version number of package */
#define VERSION "3.2.1"

/* Enable large inode numbers on Mac OS X 10.5.  */
#ifndef _DARWIN_USE_64_BIT_INODE
# define _DARWIN_USE_64_BIT_INODE 1
#endif

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Provide blksize_t if not available */
/* #undef blksize_t */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to `long int' if <sys/types.h> does not define. */
/* #undef off_t */

/* Define to the equivalent of the C99 'restrict' keyword, or to
   nothing if this is not supported.  Do not define if restrict is
   supported directly.  */
#define restrict __restrict
/* Work around a bug in Sun C++: it does not support _Restrict or
   __restrict__, even though the corresponding Sun C compiler ends up with
   "#define restrict _Restrict" or "#define restrict __restrict__" in the
   previous line.  Perhaps some future version of Sun C++ will work with
   restrict; if so, hopefully it defines __RESTRICT like Sun C does.  */
#if defined __SUNPRO_CC && !defined __RESTRICT
# define _Restrict
# define __restrict__
#endif

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef ssize_t */

/* Define to "unsigned char" if sys/types.h does not define. */
/* #undef u_char */

/* Define to "unsigned int" if sys/types.h does not define. */
/* #undef u_int */

/* Define missing 64 bit type */
/* #undef u_int64_t */

/* Define to "unsigned long" if sys/types.h does not define. */
/* #undef u_long */

/* Define to "unsigned short" if sys/types.h does not define. */
/* #undef u_short */


/* quash PACKAGE and PACKAGE_* vars, see MPICH top-level configure.ac for
 * more info */
#include "nopackage.h"

#endif /* !defined(ROMIOCONF_H_INCLUDED) */

