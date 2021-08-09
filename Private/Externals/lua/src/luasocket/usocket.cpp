/*=========================================================================*\
* Socket compatibilization module for Unix
* LuaSocket toolkit
*
* The code is now interrupt-safe.
* The penalty of calling select to avoid busy-wait is only paid when
* the I/O call fail in the first place. 
\*=========================================================================*/
#ifndef _WIN32
#include "usocket.h"

#include <string.h> 
#include <signal.h>

#include "socket.h"


/*-------------------------------------------------------------------------*\
* Wait for readable/writable/connected socket with timeout
\*-------------------------------------------------------------------------*/
#ifndef SOCKET_SELECT
#include <sys/poll.h>

namespace NS_PDR_SLUA {    

#define ___PDR_WAITFD_R        POLLIN
#define ___PDR_WAITFD_W        POLLOUT
#define ___PDR_WAITFD_C        (POLLIN|POLLOUT)
int ___pdr_socket_waitfd(___pdr_p_socket ps, int sw, ___pdr_p_timeout tm) {
    int ret;
    struct pollfd pfd;
    pfd.fd = *ps;
    pfd.events = sw;
    pfd.revents = 0;
    if (timeout_iszero(tm)) return PDR_IO_TIMEOUT;  /* optimize timeout == 0 case */
    do {
        int t = (int)(___pdr_timeout_getretry(tm)*1e3);
        ret = poll(&pfd, 1, t >= 0? t: -1);
    } while (ret == -1 && errno == EINTR);
    if (ret == -1) return errno;
    if (ret == 0) return PDR_IO_TIMEOUT;
    if (sw == ___PDR_WAITFD_C && (pfd.revents & (POLLIN|POLLERR))) return PDR_IO_CLOSED;
    return PDR_IO_DONE;
}
#else

#define ___PDR_WAITFD_R        1
#define ___PDR_WAITFD_W        2
#define ___PDR_WAITFD_C        (___PDR_WAITFD_R|___PDR_WAITFD_W)

int ___pdr_socket_waitfd(___pdr_p_socket ps, int sw, ___pdr_p_timeout tm) {
    int ret;
    fd_set rfds, wfds, *rp, *wp;
    struct timeval tv, *tp;
    double t;
    if (*ps >= FD_SETSIZE) return EINVAL;
    if (timeout_iszero(tm)) return PDR_IO_TIMEOUT;  /* optimize timeout == 0 case */
    do {
        /* must set bits within loop, because select may have modifed them */
        rp = wp = NULL;
        if (sw & ___PDR_WAITFD_R) { FD_ZERO(&rfds); FD_SET(*ps, &rfds); rp = &rfds; }
        if (sw & ___PDR_WAITFD_W) { FD_ZERO(&wfds); FD_SET(*ps, &wfds); wp = &wfds; }
        t = ___pdr_timeout_getretry(tm);
        tp = NULL;
        if (t >= 0.0) {
            tv.tv_sec = (int)t;
            tv.tv_usec = (int)((t-tv.tv_sec)*1.0e6);
            tp = &tv;
        }
        ret = select(*ps+1, rp, wp, NULL, tp);
    } while (ret == -1 && errno == EINTR);
    if (ret == -1) return errno;
    if (ret == 0) return PDR_IO_TIMEOUT;
    if (sw == ___PDR_WAITFD_C && FD_ISSET(*ps, &rfds)) return PDR_IO_CLOSED;
    return PDR_IO_DONE;
}
#endif


/*-------------------------------------------------------------------------*\
* Initializes module 
\*-------------------------------------------------------------------------*/
int ___pdr_socket_open(void) {
    /* instals a handler to ignore sigpipe or it will crash us */
    signal(SIGPIPE, SIG_IGN);
    return 1;
}

/*-------------------------------------------------------------------------*\
* Close module 
\*-------------------------------------------------------------------------*/
int ___pdr_socket_close(void) {
    return 1;
}

/*-------------------------------------------------------------------------*\
* Close and inutilize socket
\*-------------------------------------------------------------------------*/
void ___pdr_socket_destroy(___pdr_p_socket ps) {
    if (*ps != SOCKET_INVALID) {
        ___pdr_socket_setblocking(ps);
        close(*ps);
        *ps = SOCKET_INVALID;
    }
}

/*-------------------------------------------------------------------------*\
* Select with timeout control
\*-------------------------------------------------------------------------*/
int ___pdr_socket_select(___pdr_t_socket n, fd_set *rfds, fd_set *wfds, fd_set *efds,
        ___pdr_p_timeout tm) {
    int ret;
    do {
        struct timeval tv;
        double t = ___pdr_timeout_getretry(tm);
        tv.tv_sec = (int) t;
        tv.tv_usec = (int) ((t - tv.tv_sec) * 1.0e6);
        /* timeout = 0 means no wait */
        ret = select(n, rfds, wfds, efds, t >= 0.0 ? &tv: NULL);
    } while (ret < 0 && errno == EINTR);
    return ret;
}

/*-------------------------------------------------------------------------*\
* Creates and sets up a socket
\*-------------------------------------------------------------------------*/
int ___pdr_socket_create(___pdr_p_socket ps, int domain, int type, int protocol) {
    *ps = socket(domain, type, protocol);
    if (*ps != SOCKET_INVALID) return PDR_IO_DONE; 
    else return errno; 
}

/*-------------------------------------------------------------------------*\
* Binds or returns error message
\*-------------------------------------------------------------------------*/
int ___pdr_socket_bind(___pdr_p_socket ps, ___pdr_SA *addr, ___pdr_socklen_t len) {
    int err = PDR_IO_DONE;
    ___pdr_socket_setblocking(ps);
    if (bind(*ps, addr, len) < 0) err = errno; 
    ___pdr_socket_setnonblocking(ps);
    return err;
}

/*-------------------------------------------------------------------------*\
* 
\*-------------------------------------------------------------------------*/
int ___pdr_socket_listen(___pdr_p_socket ps, int backlog) {
    int err = PDR_IO_DONE; 
    ___pdr_socket_setblocking(ps);
    if (listen(*ps, backlog)) err = errno; 
    ___pdr_socket_setnonblocking(ps);
    return err;
}

/*-------------------------------------------------------------------------*\
* 
\*-------------------------------------------------------------------------*/
void ___pdr_socket_shutdown(___pdr_p_socket ps, int how) {
    ___pdr_socket_setblocking(ps);
    shutdown(*ps, how);
    ___pdr_socket_setnonblocking(ps);
}

/*-------------------------------------------------------------------------*\
* Connects or returns error message
\*-------------------------------------------------------------------------*/
int ___pdr_socket_connect(___pdr_p_socket ps, ___pdr_SA *addr, ___pdr_socklen_t len, ___pdr_p_timeout tm) {
    int err;
    /* avoid calling on closed sockets */
    if (*ps == SOCKET_INVALID) return PDR_IO_CLOSED;
    /* call connect until done or failed without being interrupted */
    do if (connect(*ps, addr, len) == 0) return PDR_IO_DONE;
    while ((err = errno) == EINTR);
    /* if connection failed immediately, return error code */
    if (err != EINPROGRESS && err != EAGAIN) return err; 
    /* zero timeout case optimization */
    if (timeout_iszero(tm)) return PDR_IO_TIMEOUT;
    /* wait until we have the result of the connection attempt or timeout */
    err = ___pdr_socket_waitfd(ps, ___PDR_WAITFD_C, tm);
    if (err == PDR_IO_CLOSED) {
        if (recv(*ps, (char *) &err, 0, 0) == 0) return PDR_IO_DONE;
        else return errno;
    } else return err;
}

/*-------------------------------------------------------------------------*\
* Accept with timeout
\*-------------------------------------------------------------------------*/
int ___pdr_socket_accept(___pdr_p_socket ps, ___pdr_p_socket pa, ___pdr_SA *addr, ___pdr_socklen_t *len, ___pdr_p_timeout tm) {
    if (*ps == SOCKET_INVALID) return PDR_IO_CLOSED; 
    for ( ;; ) {
        int err;
        if ((*pa = accept(*ps, addr, len)) != SOCKET_INVALID) return PDR_IO_DONE;
        err = errno;
        if (err == EINTR) continue;
        if (err != EAGAIN && err != ECONNABORTED) return err;
        if ((err = ___pdr_socket_waitfd(ps, ___PDR_WAITFD_R, tm)) != PDR_IO_DONE) return err;
    }
    /* can't reach here */
    return PDR_IO_UNKNOWN;
}

/*-------------------------------------------------------------------------*\
* Send with timeout
\*-------------------------------------------------------------------------*/
int ___pdr_socket_send(___pdr_p_socket ps, const char *data, size_t count,
        size_t *sent, ___pdr_p_timeout tm)
{
    int err;
    *sent = 0;
    /* avoid making system calls on closed sockets */
    if (*ps == SOCKET_INVALID) return PDR_IO_CLOSED;
    /* loop until we send something or we give up on error */
    for ( ;; ) {
        long put = (long) send(*ps, data, count, 0);
        /* if we sent anything, we are done */
        if (put >= 0) {
            *sent = put;
            return PDR_IO_DONE;
        }
        err = errno;
        /* EPIPE means the connection was closed */
        if (err == EPIPE) return PDR_IO_CLOSED;
        /* we call was interrupted, just try again */
        if (err == EINTR) continue;
        /* if failed fatal reason, report error */
        if (err != EAGAIN) return err;
        /* wait until we can send something or we timeout */
        if ((err = ___pdr_socket_waitfd(ps, ___PDR_WAITFD_W, tm)) != PDR_IO_DONE) return err;
    }
    /* can't reach here */
    return PDR_IO_UNKNOWN;
}

/*-------------------------------------------------------------------------*\
* Sendto with timeout
\*-------------------------------------------------------------------------*/
int ___pdr_socket_sendto(___pdr_p_socket ps, const char *data, size_t count, size_t *sent,
        ___pdr_SA *addr, ___pdr_socklen_t len, ___pdr_p_timeout tm)
{
    int err;
    *sent = 0;
    if (*ps == SOCKET_INVALID) return PDR_IO_CLOSED;
    for ( ;; ) {
        long put = (long) sendto(*ps, data, count, 0, addr, len);  
        if (put >= 0) {
            *sent = put;
            return PDR_IO_DONE;
        }
        err = errno;
        if (err == EPIPE) return PDR_IO_CLOSED;
        if (err == EINTR) continue;
        if (err != EAGAIN) return err;
        if ((err = ___pdr_socket_waitfd(ps, ___PDR_WAITFD_W, tm)) != PDR_IO_DONE) return err;
    }
    return PDR_IO_UNKNOWN;
}

/*-------------------------------------------------------------------------*\
* Receive with timeout
\*-------------------------------------------------------------------------*/
int ___pdr_socket_recv(___pdr_p_socket ps, char *data, size_t count, size_t *got, ___pdr_p_timeout tm) {
    int err;
    *got = 0;
    if (*ps == SOCKET_INVALID) return PDR_IO_CLOSED;
    for ( ;; ) {
        long taken = (long) recv(*ps, data, count, 0);
        if (taken > 0) {
            *got = taken;
            return PDR_IO_DONE;
        }
        err = errno;
        if (taken == 0) return PDR_IO_CLOSED;
        if (err == EINTR) continue;
        if (err != EAGAIN) return err; 
        if ((err = ___pdr_socket_waitfd(ps, ___PDR_WAITFD_R, tm)) != PDR_IO_DONE) return err;
    }
    return PDR_IO_UNKNOWN;
}

/*-------------------------------------------------------------------------*\
* Recvfrom with timeout
\*-------------------------------------------------------------------------*/
int ___pdr_socket_recvfrom(___pdr_p_socket ps, char *data, size_t count, size_t *got,
        ___pdr_SA *addr, ___pdr_socklen_t *len, ___pdr_p_timeout tm) {
    int err;
    *got = 0;
    if (*ps == SOCKET_INVALID) return PDR_IO_CLOSED;
    for ( ;; ) {
        long taken = (long) recvfrom(*ps, data, count, 0, addr, len);
        if (taken > 0) {
            *got = taken;
            return PDR_IO_DONE;
        }
        err = errno;
        if (taken == 0) return PDR_IO_CLOSED;
        if (err == EINTR) continue;
        if (err != EAGAIN) return err; 
        if ((err = ___pdr_socket_waitfd(ps, ___PDR_WAITFD_R, tm)) != PDR_IO_DONE) return err;
    }
    return PDR_IO_UNKNOWN;
}


/*-------------------------------------------------------------------------*\
* Write with timeout
*
* socket_read and socket_write are cut-n-paste of socket_send and socket_recv,
* with send/recv replaced with write/read. We can't just use write/read
* in the socket version, because behaviour when size is zero is different.
\*-------------------------------------------------------------------------*/
int ___pdr_socket_write(___pdr_p_socket ps, const char *data, size_t count,
        size_t *sent, ___pdr_p_timeout tm)
{
    int err;
    *sent = 0;
    /* avoid making system calls on closed sockets */
    if (*ps == SOCKET_INVALID) return PDR_IO_CLOSED;
    /* loop until we send something or we give up on error */
    for ( ;; ) {
        long put = (long) write(*ps, data, count);
        /* if we sent anything, we are done */
        if (put >= 0) {
            *sent = put;
            return PDR_IO_DONE;
        }
        err = errno;
        /* EPIPE means the connection was closed */
        if (err == EPIPE) return PDR_IO_CLOSED;
        /* we call was interrupted, just try again */
        if (err == EINTR) continue;
        /* if failed fatal reason, report error */
        if (err != EAGAIN) return err;
        /* wait until we can send something or we timeout */
        if ((err = ___pdr_socket_waitfd(ps, ___PDR_WAITFD_W, tm)) != PDR_IO_DONE) return err;
    }
    /* can't reach here */
    return PDR_IO_UNKNOWN;
}

/*-------------------------------------------------------------------------*\
* Read with timeout
* See note for socket_write
\*-------------------------------------------------------------------------*/
int ___pdr_socket_read(___pdr_p_socket ps, char *data, size_t count, size_t *got, ___pdr_p_timeout tm) {
    int err;
    *got = 0;
    if (*ps == SOCKET_INVALID) return PDR_IO_CLOSED;
    for ( ;; ) {
        long taken = (long) read(*ps, data, count);
        if (taken > 0) {
            *got = taken;
            return PDR_IO_DONE;
        }
        err = errno;
        if (taken == 0) return PDR_IO_CLOSED;
        if (err == EINTR) continue;
        if (err != EAGAIN) return err; 
        if ((err = ___pdr_socket_waitfd(ps, ___PDR_WAITFD_R, tm)) != PDR_IO_DONE) return err;
    }
    return PDR_IO_UNKNOWN;
}

/*-------------------------------------------------------------------------*\
* Put socket into blocking mode
\*-------------------------------------------------------------------------*/
void ___pdr_socket_setblocking(___pdr_p_socket ps) {
    int flags = fcntl(*ps, F_GETFL, 0);
    flags &= (~(O_NONBLOCK));
    fcntl(*ps, F_SETFL, flags);
}

/*-------------------------------------------------------------------------*\
* Put socket into non-blocking mode
\*-------------------------------------------------------------------------*/
void ___pdr_socket_setnonblocking(___pdr_p_socket ps) {
    int flags = fcntl(*ps, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(*ps, F_SETFL, flags);
}

/*-------------------------------------------------------------------------*\
* DNS helpers 
\*-------------------------------------------------------------------------*/
int ___pdr_socket_gethostbyaddr(const char *addr, ___pdr_socklen_t len, struct hostent **hp) {
    *hp = gethostbyaddr(addr, len, AF_INET);
    if (*hp) return PDR_IO_DONE;
    else if (h_errno) return h_errno;
    else if (errno) return errno;
    else return PDR_IO_UNKNOWN;
}

int ___pdr_socket_gethostbyname(const char *addr, struct hostent **hp) {
    *hp = gethostbyname(addr);
    if (*hp) return PDR_IO_DONE;
    else if (h_errno) return h_errno;
    else if (errno) return errno;
    else return PDR_IO_UNKNOWN;
}

/*-------------------------------------------------------------------------*\
* Error translation functions
* Make sure important error messages are standard
\*-------------------------------------------------------------------------*/
const char * ___pdr_socket_hoststrerror(int err) {
    if (err <= 0) return ___pdr_io_strerror(err);
    switch (err) {
        case HOST_NOT_FOUND: return "host not found";
        default: return hstrerror(err);
    }
}

const char * ___pdr_socket_strerror(int err) {
    if (err <= 0) return ___pdr_io_strerror(err);
    switch (err) {
        case EADDRINUSE: return "address already in use";
        case EISCONN: return "already connected";
        case EACCES: return "permission denied";
        case ECONNREFUSED: return "connection refused";
        case ECONNABORTED: return "closed";
        case ECONNRESET: return "closed";
        case ETIMEDOUT: return "timeout";
        default: return strerror(err);
    }
}

const char * ___pdr_socket_ioerror(___pdr_p_socket ps, int err) {
    (void) ps;
    return ___pdr_socket_strerror(err);
} 

const char * ___pdr_socket_gaistrerror(int err) {
    if (err == 0) return NULL; 
    switch (err) {
        case EAI_AGAIN: return "temporary failure in name resolution";
        case EAI_BADFLAGS: return "invalid value for ai_flags";
#ifdef EAI_BADHINTS
        case EAI_BADHINTS: return "invalid value for hints";
#endif
        case EAI_FAIL: return "non-recoverable failure in name resolution";
        case EAI_FAMILY: return "ai_family not supported";
        case EAI_MEMORY: return "memory allocation failure";
        case EAI_NONAME: 
            return "host or service not provided, or not known";
        case EAI_OVERFLOW: return "argument buffer overflow";
#ifdef EAI_PROTOCOL
        case EAI_PROTOCOL: return "resolved protocol is unknown";
#endif
        case EAI_SERVICE: return "service not supported for socket type";
        case EAI_SOCKTYPE: return "ai_socktype not supported";
        case EAI_SYSTEM: return strerror(errno); 
        default: return gai_strerror(err);
    }
}


} // end NS_PDR_SLUA

#endif // _WIN32