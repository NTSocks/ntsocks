/*
 * <p>Title: nt_errno.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date Jan 4, 2020 
 * @version 1.0
 */

#ifndef NT_ERRNO_H_
#define NT_ERRNO_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MALLOC_ERR_MSG                      "malloc() failed"


// for bind
#define NT_ERR_INVALID_IP 		            11
#define NT_ERR_INVALID_PORT		            12
#define NT_ERR_INUSE_PORT                   13
#define NT_ERR_REQUIRE_CLOSED_FIRST         14
#define NT_ERR_INVALID_SOCK		            20

// for listen
#define NT_ERR_NTS_SHM_CONN_NOTFOUND        30
#define NT_ERR_REQUIRE_BIND_FIRST           31

// for accept
#define NT_ERR_REQUIRE_LISTEN_FIRST         40

// for connect
#define NT_ERR_REQUIRE_CLOSED_OR_BOUND_FIRST    50
#define NT_ERR_REMOTE_NTM_NOT_FOUND             51

// for finish
#define NT_ERR_REQUIRE_ESTABLISHED_FIRST  60

#ifdef __cplusplus
};
#endif

#endif /* NT_ERRNO_H_ */
