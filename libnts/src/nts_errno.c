/*
 * <p>Title: nts_errno.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Bob Huang
 * @date Nov 23, 2019 
 * @version 1.0
 */

#include "nts_errno.h"

static const char* faults[] = {
    [nts_EPERM] = "Operation not permitted",
    [nts_ENOENT] = "No such file or directory",
    [nts_ESRCH] = "No such process",
    [nts_EINTR] = "Interrupted system call",
    [nts_EIO] = "Input/output error",
    [nts_ENXIO] = "Device not configured",
    [nts_E2BIG] = "Argument list too long",
    [nts_ENOEXEC] = "Exec format error",
    [nts_EBADF] = "Bad file descriptor",
    [nts_ECHILD] = "No child processes",
    [nts_EDEADLK] = "Resource deadlock avoided",
    [nts_ENOMEM] = "Cannot allocate memory",
    [nts_EACCES] = "Permission denied",
    [nts_EFAULT] = "Bad address",
    [nts_ENOTBLK] = "Block device required",
    [nts_EBUSY] = "Device busy",
    [nts_EEXIST] = "File exists",
    [nts_EXDEV] = "Cross-device link",
    [nts_ENODEV] = "Operation not supported by device",
    [nts_ENOTDIR] = "Not a directory",
    [nts_EISDIR] = "Is a directory",
    [nts_EINVAL] = "Invalid argument",
    [nts_ENFILE] = "Too many open files in system",
    [nts_EMFILE] = "Too many open files",
    [nts_ENOTTY] = "Inappropriate ioctl for device",
    [nts_ETXTBSY] = "Text file busy",
    [nts_EFBIG] = "File too large",
    [nts_ENOSPC] = "No space left on device",
    [nts_ESPIPE] = "Illegal seek",
    [nts_EROFS] = "Read-only filesystem",
    [nts_EMLINK] = "Too many links",
    [nts_EPIPE] = "Broken pipe",

    /* math software */
    [nts_EDOM] = "Numerical argument out of domain",
    [nts_ERANGE] = "Result too large",

    /* non-blocking and interrupt i/o */
    [nts_EAGAIN] = "Resource temporarily unavailable",
    [nts_EWOULDBLOCK] = "Operation would block",
    [nts_EINPROGRESS] = "Operation now in progress",
    [nts_EALREADY] = "Operation already in progress",

    /* ipc/network software -- argument errors */
    [nts_ENOTSOCK] = "Socket operation on non-socket",
    [nts_EDESTADDRREQ] = "Destination address required",
    [nts_EMSGSIZE] = "Message too long",
    [nts_EPROTOTYPE] = "Protocol wrong type for socket",
    [nts_ENOPROTOOPT] = "Protocol not available",
    [nts_EPROTONOSUPPORT] = "Protocol not supported",
    [nts_ESOCKTNOSUPPORT] = "Socket type not supported",
    [nts_EOPNOTSUPP] = "Operation not supported",
    [nts_ENOTSUP] = "Operation not supported",
    [nts_EPFNOSUPPORT] = "Protocol family not supported",
    [nts_EAFNOSUPPORT] = "Address family not supported by protocol family",
    [nts_EADDRINUSE] = "Address already in use",
    [nts_EADDRNOTAVAIL] = "Can't assign requested address",

    /* ipc/network software -- operational errors */
    [nts_ENETDOWN] = "Network is down",
    [nts_ENETUNREACH] = "Network is unreachable",
    [nts_ENETRESET] = "Network dropped connection on reset",
    [nts_ECONNABORTED] = "Software caused connection abort",
    [nts_ECONNRESET] = "Connection reset by peer",
    [nts_ENOBUFS] = "No buffer space available",
    [nts_EISCONN] = "Socket is already connected",
    [nts_ENOTCONN] = "Socket is not connected",
    [nts_ESHUTDOWN] = "Can't send after socket shutdown",
    [nts_ETOOMANYREFS] = "Too many references: can't splice",
    [nts_ETIMEDOUT] = "Operation timed out",
    [nts_ECONNREFUSED] = "Connection refused",
    
    [nts_ELOOP] = "Too many levels of symbolic links",
    [nts_ENAMETOOLONG] = "File name too long",

    /* should be rearranged */
    [nts_EHOSTDOWN] = "Host is down",
    [nts_EHOSTUNREACH] = "No route to host",
    [nts_ENOTEMPTY] = "Directory not empty",

    /* quotas & mush */
    [nts_EPROCLIM] = "Too many processes",
    [nts_EUSERS] = "Too many users",
    [nts_EDQUOT] = "Disc quota exceeded",
   
    [nts_ESTALE] = "Stale NFS file handle",
    [nts_EREMOTE] = "Too many levels of remote in path",
    [nts_EBADRPC] = "RPC struct is bad",
    [nts_ERPCMISMATCH] = "RPC version wrong",
    [nts_EPROGUNAVAIL] = "RPC prog. not avail",
    [nts_EPROGMISMATCH] = "Program version wrong",
    [nts_EPROCUNAVAIL] = "Bad procedure for program",
    
    [nts_ENOLCK] = "No locks available",
    [nts_ENOSYS] = "Function not implemented",
    
    [nts_EFTYPE] = "Inappropriate file type or format",
    [nts_EAUTH] = "Authentication error",
    [nts_ENEEDAUTH] = "Need authenticator",
    [nts_EIDRM] = "Identifier removed",
    [nts_ENOMSG] = "No message of desired type",
    [nts_EOVERFLOW] = "Value too large to be stored in data type",
    [nts_ECANCELED] = "Operation canceled",
    [nts_EILSEQ] = "Illegal byte sequence",
    [nts_ENOATTR] = "Attribute not found",

    [nts_EDOOFUS] = "Programming error",
   
    [nts_EBADMSG] = "Bad message",
    [nts_EMULTIHOP] = "Multihop attempted",
    [nts_ENOLINK] = "Link has been severed",
    [nts_EPROTO] = "Protocol error",
    
    [nts_ENOTCAPABLE] = "Capabilities insufficient",
    [nts_ECAPMODE] = "Not permitted in capability mode",
    [nts_ENOTRECOVERABLE] = "State not recoverable",
    [nts_EOWNERDEAD] = "Previous owner died"
};

const char* errmsg(int errno){
    if(errno < 0) errno = -errno;
    if (errno < nts_EPERM || errno > nts_EOWNERDEAD)
    {
        return "Unknown error code";
    }
    return faults[errno];
}
