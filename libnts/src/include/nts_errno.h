/*
 * <p>Title: nts_errno.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL </p>
 *
 * @author Bob Huang
 * @date Nov 23, 2019 
 * @version 1.0
 */

#ifndef NTS_ERRNO_H_
#define NTS_ERRNO_H_

#define nts_EPERM         1        /* Operation not permitted */
#define nts_ENOENT        2        /* No such file or directory */
#define nts_ESRCH         3        /* No such process */
#define nts_EINTR         4        /* Interrupted system call */
#define nts_EIO           5        /* Input/output error */
#define nts_ENXIO         6        /* Device not configured */
#define nts_E2BIG         7        /* Argument list too long */
#define nts_ENOEXEC       8        /* Exec format error */
#define nts_EBADF         9        /* Bad file descriptor */
#define nts_ECHILD        10        /* No child processes */
#define nts_EDEADLK       11        /* Resource deadlock avoided */
#define nts_ENOMEM        12        /* Cannot allocate memory */
#define nts_EACCES        13        /* Permission denied */
#define nts_EFAULT        14        /* Bad address */
#define nts_ENOTBLK       15        /* Block device required */
#define nts_EBUSY         16        /* Device busy */
#define nts_EEXIST        17        /* File exists */
#define nts_EXDEV         18        /* Cross-device link */
#define nts_ENODEV        19        /* Operation not supported by device */
#define nts_ENOTDIR       20        /* Not a directory */
#define nts_EISDIR        21        /* Is a directory */
#define nts_EINVAL        22        /* Invalid argument */
#define nts_ENFILE        23        /* Too many open files in system */
#define nts_EMFILE        24        /* Too many open files */
#define nts_ENOTTY        25        /* Inappropriate ioctl for device */
#define nts_ETXTBSY       26        /* Text file busy */
#define nts_EFBIG         27        /* File too large */
#define nts_ENOSPC        28        /* No space left on device */
#define nts_ESPIPE        29        /* Illegal seek */
#define nts_EROFS         30        /* Read-only filesystem */
#define nts_EMLINK        31        /* Too many links */
#define nts_EPIPE         32        /* Broken pipe */

/* math software */
#define nts_EDOM          33        /* Numerical argument out of domain */
#define nts_ERANGE        34        /* Result too large */

/* non-blocking and interrupt i/o */
#define nts_EAGAIN        35        /* Resource temporarily unavailable */
#define nts_EWOULDBLOCK   nts_EAGAIN        /* Operation would block */
#define nts_EINPROGRESS   36        /* Operation now in progress */
#define nts_EALREADY      37        /* Operation already in progress */

/* ipc/network software -- argument errors */
#define nts_ENOTSOCK      38        /* Socket operation on non-socket */
#define nts_EDESTADDRREQ  39        /* Destination address required */
#define nts_EMSGSIZE      40        /* Message too long */
#define nts_EPROTOTYPE    41        /* Protocol wrong type for socket */
#define nts_ENOPROTOOPT   42        /* Protocol not available */
#define nts_EPROTONOSUPPORT    43        /* Protocol not supported */
#define nts_ESOCKTNOSUPPORT    44        /* Socket type not supported */
#define nts_EOPNOTSUPP         45        /* Operation not supported */
#define nts_ENOTSUP        nts_EOPNOTSUPP    /* Operation not supported */
#define nts_EPFNOSUPPORT       46        /* Protocol family not supported */
#define nts_EAFNOSUPPORT       47        /* Address family not supported by protocol family */
#define nts_EADDRINUSE         48        /* Address already in use */
#define nts_EADDRNOTAVAIL      49        /* Can't assign requested address */

/* ipc/network software -- operational errors */
#define nts_ENETDOWN       50        /* Network is down */
#define nts_ENETUNREACH    51        /* Network is unreachable */
#define nts_ENETRESET      52        /* Network dropped connection on reset */
#define nts_ECONNABORTED   53        /* Software caused connection abort */
#define nts_ECONNRESET     54        /* Connection reset by peer */
#define nts_ENOBUFS        55        /* No buffer space available */
#define nts_EISCONN        56        /* Socket is already connected */
#define nts_ENOTCONN       57        /* Socket is not connected */
#define nts_ESHUTDOWN      58        /* Can't send after socket shutdown */
#define nts_ETOOMANYREFS   59        /* Too many references: can't splice */
#define nts_ETIMEDOUT      60        /* Operation timed out */
#define nts_ECONNREFUSED   61        /* Connection refused */

#define nts_ELOOP          62        /* Too many levels of symbolic links */
#define nts_ENAMETOOLONG   63        /* File name too long */

/* should be rearranged */
#define nts_EHOSTDOWN      64        /* Host is down */
#define nts_EHOSTUNREACH   65        /* No route to host */
#define nts_ENOTEMPTY      66        /* Directory not empty */

/* quotas & mush */
#define nts_EPROCLIM       67        /* Too many processes */
#define nts_EUSERS         68        /* Too many users */
#define nts_EDQUOT         69        /* Disc quota exceeded */

#define nts_ESTALE         70        /* Stale NFS file handle */
#define nts_EREMOTE        71        /* Too many levels of remote in path */
#define nts_EBADRPC        72        /* RPC struct is bad */
#define nts_ERPCMISMATCH   73        /* RPC version wrong */
#define nts_EPROGUNAVAIL   74        /* RPC prog. not avail */
#define nts_EPROGMISMATCH  75        /* Program version wrong */
#define nts_EPROCUNAVAIL   76        /* Bad procedure for program */

#define nts_ENOLCK         77        /* No locks available */
#define nts_ENOSYS         78        /* Function not implemented */

#define nts_EFTYPE         79        /* Inappropriate file type or format */
#define nts_EAUTH          80        /* Authentication error */
#define nts_ENEEDAUTH      81        /* Need authenticator */
#define nts_EIDRM          82        /* Identifier removed */
#define nts_ENOMSG         83        /* No message of desired type */
#define nts_EOVERFLOW      84        /* Value too large to be stored in data type */
#define nts_ECANCELED      85        /* Operation canceled */
#define nts_EILSEQ         86        /* Illegal byte sequence */
#define nts_ENOATTR        87        /* Attribute not found */

#define nts_EDOOFUS        88        /* Programming error */

#define nts_EBADMSG        89        /* Bad message */
#define nts_EMULTIHOP      90        /* Multihop attempted */
#define nts_ENOLINK        91        /* Link has been severed */
#define nts_EPROTO         92        /* Protocol error */

#define nts_ENOTCAPABLE    93        /* Capabilities insufficient */
#define nts_ECAPMODE       94        /* Not permitted in capability mode */
#define nts_ENOTRECOVERABLE 95        /* State not recoverable */
#define nts_EOWNERDEAD      96        /* Previous owner died */

#endif /* NTS_ERRNO_H_ */
