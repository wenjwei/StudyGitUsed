/*=========================================================================*\
* Socket compatibilization module for Win32
* LuaSocket toolkit
*
* The penalty of calling select to avoid busy-wait is only paid when
* the I/O call fail in the first place. 
\*=========================================================================*/
#ifdef _WIN32

#include "wsocket.h"
#include <string.h>

#include "socket.h"


namespace NS_PDR_SLUA {    

/* WinSock doesn't have a strerror... */
static const char *wstrerror(int err);

/*-------------------------------------------------------------------------*\
* Initializes module 
\*-------------------------------------------------------------------------*/
int ___pdr_socket_open(void) {
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 0); 
    int err = WSAStartup(wVersionRequested, &wsaData );
    if (err != 0) return 0;
    if ((LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 0) &&
        (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)) {
        WSACleanup();
        return 0; 
    }
    return 1;
}

/*-------------------------------------------------------------------------*\
* Close module 
\*-------------------------------------------------------------------------*/
int ___pdr_socket_close(void) {
    WSACleanup();
    return 1;
}

/*-------------------------------------------------------------------------*\
* Wait for readable/writable/connected socket with timeout
\*-------------------------------------------------------------------------*/
#define ___PDR_WAITFD_R        1
#define ___PDR_WAITFD_W        2
#define ___PDR_WAITFD_E        4
#define ___PDR_WAITFD_C        (___PDR_WAITFD_E|___PDR_WAITFD_W)

int ___pdr_socket_waitfd(___pdr_p_socket ps, int sw, ___pdr_p_timeout tm) {
    int ret;
    fd_set rfds, wfds, efds, *rp = NULL, *wp = NULL, *ep = NULL;
    struct timeval tv, *tp = NULL;
    double t;
    if (timeout_iszero(tm)) return PDR_IO_TIMEOUT;  /* optimize timeout == 0 case */
    if (sw & ___PDR_WAITFD_R) { 
        FD_ZERO(&rfds); 
        FD_SET(*ps, &rfds);
        rp = &rfds; 
    }
    if (sw & ___PDR_WAITFD_W) { FD_ZERO(&wfds); FD_SET(*ps, &wfds); wp = &wfds; }
    if (sw & ___PDR_WAITFD_C) { FD_ZERO(&efds); FD_SET(*ps, &efds); ep = &efds; }
    if ((t = ___pdr_timeout_get(tm)) >= 0.0) {
        tv.tv_sec = (int) t;
        tv.tv_usec = (int) ((t-tv.tv_sec)*1.0e6);
        tp = &tv;
    }
    ret = select(0, rp, wp, ep, tp);
    if (ret == -1) return WSAGetLastError();
    if (ret == 0) return PDR_IO_TIMEOUT;
    if (sw == ___PDR_WAITFD_C && FD_ISSET(*ps, &efds)) return PDR_IO_CLOSED;
    return PDR_IO_DONE;
}

/*-------------------------------------------------------------------------*\
* Select with int timeout in ms
\*-------------------------------------------------------------------------*/
int ___pdr_socket_select(___pdr_t_socket n, fd_set *rfds, fd_set *wfds, fd_set *efds, 
        ___pdr_p_timeout tm) {
    struct timeval tv; 
    double t = ___pdr_timeout_get(tm);
    tv.tv_sec = (int) t;
    tv.tv_usec = (int) ((t - tv.tv_sec) * 1.0e6);
    if (n <= 0) {
        Sleep((DWORD) (1000*t));
        return 0;
    } else return select(0, rfds, wfds, efds, t >= 0.0? &tv: NULL);
}

/*-------------------------------------------------------------------------*\
* Close and inutilize socket
\*-------------------------------------------------------------------------*/
void ___pdr_socket_destroy(___pdr_p_socket ps) {
    if (*ps != SOCKET_INVALID) {
        ___pdr_socket_setblocking(ps); /* close can take a long time on WIN32 */
        closesocket(*ps);
        *ps = SOCKET_INVALID;
    }
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
* Creates and sets up a socket
\*-------------------------------------------------------------------------*/
int ___pdr_socket_create(___pdr_p_socket ps, int domain, int type, int protocol) {
    *ps = socket(domain, type, protocol);
    if (*ps != SOCKET_INVALID) return PDR_IO_DONE;
    else return WSAGetLastError();
}

/*-------------------------------------------------------------------------*\
* Connects or returns error message
\*-------------------------------------------------------------------------*/
int ___pdr_socket_connect(___pdr_p_socket ps, ___pdr_SA *addr, ___pdr_socklen_t len, ___pdr_p_timeout tm) {
    int err;
    /* don't call on closed socket */
    if (*ps == SOCKET_INVALID) return PDR_IO_CLOSED;
    /* ask system to connect */
    if (connect(*ps, addr, len) == 0) return PDR_IO_DONE;
    /* make sure the system is trying to connect */
    err = WSAGetLastError();
    if (err != WSAEWOULDBLOCK && err != WSAEINPROGRESS) return err;
    /* zero timeout case optimization */
    if (timeout_iszero(tm)) return PDR_IO_TIMEOUT;
    /* we wait until something happens */
    err = ___pdr_socket_waitfd(ps, ___PDR_WAITFD_C, tm);
    if (err == PDR_IO_CLOSED) {
        int len = sizeof(err);
        /* give windows time to set the error (yes, disgusting) */
        Sleep(10);
        /* find out why we failed */
        getsockopt(*ps, SOL_SOCKET, SO_ERROR, (char *)&err, &len); 
        /* we KNOW there was an error. if 'why' is 0, we will return
        * "unknown error", but it's not really our fault */
        return err > 0? err: PDR_IO_UNKNOWN; 
    } else return err;

}

/*-------------------------------------------------------------------------*\
* Binds or returns error message
\*-------------------------------------------------------------------------*/
int ___pdr_socket_bind(___pdr_p_socket ps, ___pdr_SA *addr, ___pdr_socklen_t len) {
    int err = PDR_IO_DONE;
    ___pdr_socket_setblocking(ps);
    if (bind(*ps, addr, len) < 0) err = WSAGetLastError();
    ___pdr_socket_setnonblocking(ps);
    return err;
}

/*-------------------------------------------------------------------------*\
* 
\*-------------------------------------------------------------------------*/
int ___pdr_socket_listen(___pdr_p_socket ps, int backlog) {
    int err = PDR_IO_DONE;
    ___pdr_socket_setblocking(ps);
    if (listen(*ps, backlog) < 0) err = WSAGetLastError();
    ___pdr_socket_setnonblocking(ps);
    return err;
}

/*-------------------------------------------------------------------------*\
* Accept with timeout
\*-------------------------------------------------------------------------*/
int ___pdr_socket_accept(___pdr_p_socket ps, ___pdr_p_socket pa, ___pdr_SA *addr, ___pdr_socklen_t *len, 
        ___pdr_p_timeout tm) {
    if (*ps == SOCKET_INVALID) return PDR_IO_CLOSED;
    for ( ;; ) {
        int err;
        /* try to get client socket */
        if ((*pa = accept(*ps, addr, len)) != SOCKET_INVALID) return PDR_IO_DONE;
        /* find out why we failed */
        err = WSAGetLastError(); 
        /* if we failed because there was no connectoin, keep trying */
        if (err != WSAEWOULDBLOCK && err != WSAECONNABORTED) return err;
        /* call select to avoid busy wait */
        if ((err = ___pdr_socket_waitfd(ps, ___PDR_WAITFD_R, tm)) != PDR_IO_DONE) return err;
    } 
}

/*-------------------------------------------------------------------------*\
* Send with timeout
* On windows, if you try to send 10MB, the OS will buffer EVERYTHING 
* this can take an awful lot of time and we will end up blocked. 
* Therefore, whoever calls this function should not pass a huge buffer.
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
        /* try to send something */
        int put = send(*ps, data, (int) count, 0);
        /* if we sent something, we are done */
        if (put > 0) {
            *sent = put;
            return PDR_IO_DONE;
        }
        /* deal with failure */
        err = WSAGetLastError(); 
        /* we can only proceed if there was no serious error */
        if (err != WSAEWOULDBLOCK) return err;
        /* avoid busy wait */
        if ((err = ___pdr_socket_waitfd(ps, ___PDR_WAITFD_W, tm)) != PDR_IO_DONE) return err;
    } 
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
        int put = sendto(*ps, data, (int) count, 0, addr, len);
        if (put > 0) {
            *sent = put;
            return PDR_IO_DONE;
        }
        err = WSAGetLastError(); 
        if (err != WSAEWOULDBLOCK) return err;
        if ((err = ___pdr_socket_waitfd(ps, ___PDR_WAITFD_W, tm)) != PDR_IO_DONE) return err;
    } 
}

/*-------------------------------------------------------------------------*\
* Receive with timeout
\*-------------------------------------------------------------------------*/
int ___pdr_socket_recv(___pdr_p_socket ps, char *data, size_t count, size_t *got, 
        ___pdr_p_timeout tm) 
{
    int err, prev = PDR_IO_DONE;
    *got = 0;
    if (*ps == SOCKET_INVALID) return PDR_IO_CLOSED;
    for ( ;; ) {
        int taken = recv(*ps, data, (int) count, 0);
        if (taken > 0) {
            *got = taken;
            return PDR_IO_DONE;
        }
        if (taken == 0) return PDR_IO_CLOSED;
        err = WSAGetLastError();
        /* On UDP, a connreset simply means the previous send failed. 
         * So we try again. 
         * On TCP, it means our socket is now useless, so the error passes. 
         * (We will loop again, exiting because the same error will happen) */
        if (err != WSAEWOULDBLOCK) {
            if (err != WSAECONNRESET || prev == WSAECONNRESET) return err;
            prev = err;
        }
        if ((err = ___pdr_socket_waitfd(ps, ___PDR_WAITFD_R, tm)) != PDR_IO_DONE) return err;
    }
}

/*-------------------------------------------------------------------------*\
* Recvfrom with timeout
\*-------------------------------------------------------------------------*/
int ___pdr_socket_recvfrom(___pdr_p_socket ps, char *data, size_t count, size_t *got, 
        ___pdr_SA *addr, ___pdr_socklen_t *len, ___pdr_p_timeout tm) 
{
    int err, prev = PDR_IO_DONE;
    *got = 0;
    if (*ps == SOCKET_INVALID) return PDR_IO_CLOSED;
    for ( ;; ) {
        int taken = recvfrom(*ps, data, (int) count, 0, addr, len);
        if (taken > 0) {
            *got = taken;
            return PDR_IO_DONE;
        }
        if (taken == 0) return PDR_IO_CLOSED;
        err = WSAGetLastError();
        /* On UDP, a connreset simply means the previous send failed. 
         * So we try again. 
         * On TCP, it means our socket is now useless, so the error passes.
         * (We will loop again, exiting because the same error will happen) */
        if (err != WSAEWOULDBLOCK) {
            if (err != WSAECONNRESET || prev == WSAECONNRESET) return err;
            prev = err;
        }
        if ((err = ___pdr_socket_waitfd(ps, ___PDR_WAITFD_R, tm)) != PDR_IO_DONE) return err;
    }
}

/*-------------------------------------------------------------------------*\
* Put socket into blocking mode
\*-------------------------------------------------------------------------*/
void ___pdr_socket_setblocking(___pdr_p_socket ps) {
    u_long argp = 0;
    ioctlsocket(*ps, FIONBIO, &argp);
}

/*-------------------------------------------------------------------------*\
* Put socket into non-blocking mode
\*-------------------------------------------------------------------------*/
void ___pdr_socket_setnonblocking(___pdr_p_socket ps) {
    u_long argp = 1;
    ioctlsocket(*ps, FIONBIO, &argp);
}

/*-------------------------------------------------------------------------*\
* DNS helpers 
\*-------------------------------------------------------------------------*/
int ___pdr_socket_gethostbyaddr(const char *addr, ___pdr_socklen_t len, struct hostent **hp) {
    *hp = gethostbyaddr(addr, len, AF_INET);
    if (*hp) return PDR_IO_DONE;
    else return WSAGetLastError();
}

int ___pdr_socket_gethostbyname(const char *addr, struct hostent **hp) {
    *hp = gethostbyname(addr);
    if (*hp) return PDR_IO_DONE;
    else return  WSAGetLastError();
}

/*-------------------------------------------------------------------------*\
* Error translation functions
\*-------------------------------------------------------------------------*/
const char *___pdr_socket_hoststrerror(int err) {
    if (err <= 0) return ___pdr_io_strerror(err);
    switch (err) {
        case WSAHOST_NOT_FOUND: return "host not found";
        default: return wstrerror(err); 
    }
}

const char *___pdr_socket_strerror(int err) {
    if (err <= 0) return ___pdr_io_strerror(err);
    switch (err) {
        case WSAEADDRINUSE: return "address already in use";
        case WSAECONNREFUSED: return "connection refused";
        case WSAEISCONN: return "already connected";
        case WSAEACCES: return "permission denied";
        case WSAECONNABORTED: return "closed";
        case WSAECONNRESET: return "closed";
        case WSAETIMEDOUT: return "timeout";
        default: return wstrerror(err);
    }
}

const char *___pdr_socket_ioerror(___pdr_p_socket ps, int err) {
    (void) ps;
    return ___pdr_socket_strerror(err);
}

static const char *wstrerror(int err) {
    switch (err) {
        case WSAEINTR: return "Interrupted function call";
        case WSAEACCES: return "Permission denied";
        case WSAEFAULT: return "Bad address";
        case WSAEINVAL: return "Invalid argument";
        case WSAEMFILE: return "Too many open files";
        case WSAEWOULDBLOCK: return "Resource temporarily unavailable";
        case WSAEINPROGRESS: return "Operation now in progress";
        case WSAEALREADY: return "Operation already in progress";
        case WSAENOTSOCK: return "Socket operation on nonsocket";
        case WSAEDESTADDRREQ: return "Destination address required";
        case WSAEMSGSIZE: return "Message too long";
        case WSAEPROTOTYPE: return "Protocol wrong type for socket";
        case WSAENOPROTOOPT: return "Bad protocol option";
        case WSAEPROTONOSUPPORT: return "Protocol not supported";
        case WSAESOCKTNOSUPPORT: return "Socket type not supported";
        case WSAEOPNOTSUPP: return "Operation not supported";
        case WSAEPFNOSUPPORT: return "Protocol family not supported";
        case WSAEAFNOSUPPORT: 
            return "Address family not supported by protocol family"; 
        case WSAEADDRINUSE: return "Address already in use";
        case WSAEADDRNOTAVAIL: return "Cannot assign requested address";
        case WSAENETDOWN: return "Network is down";
        case WSAENETUNREACH: return "Network is unreachable";
        case WSAENETRESET: return "Network dropped connection on reset";
        case WSAECONNABORTED: return "Software caused connection abort";
        case WSAECONNRESET: return "Connection reset by peer";
        case WSAENOBUFS: return "No buffer space available";
        case WSAEISCONN: return "Socket is already connected";
        case WSAENOTCONN: return "Socket is not connected";
        case WSAESHUTDOWN: return "Cannot send after socket shutdown";
        case WSAETIMEDOUT: return "Connection timed out";
        case WSAECONNREFUSED: return "Connection refused";
        case WSAEHOSTDOWN: return "Host is down";
        case WSAEHOSTUNREACH: return "No route to host";
        case WSAEPROCLIM: return "Too many processes";
        case WSASYSNOTREADY: return "Network subsystem is unavailable";
        case WSAVERNOTSUPPORTED: return "Winsock.dll version out of range";
        case WSANOTINITIALISED: 
            return "Successful WSAStartup not yet performed";
        case WSAEDISCON: return "Graceful shutdown in progress";
        case WSAHOST_NOT_FOUND: return "Host not found";
        case WSATRY_AGAIN: return "Nonauthoritative host not found";
        case WSANO_RECOVERY: return "Nonrecoverable name lookup error"; 
        case WSANO_DATA: return "Valid name, no data record of requested type";
        default: return "Unknown error";
    }
}

const char *___pdr_socket_gaistrerror(int err) {
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
#ifdef EAI_OVERFLOW
        case EAI_OVERFLOW: return "argument buffer overflow";
#endif
#ifdef EAI_PROTOCOL
        case EAI_PROTOCOL: return "resolved protocol is unknown";
#endif
        case EAI_SERVICE: return "service not supported for socket type";
        case EAI_SOCKTYPE: return "ai_socktype not supported";
#ifdef EAI_SYSTEM
        case EAI_SYSTEM: return strerror(errno); 
#endif
        default: return gai_strerrorA(err);
    }
}


} // end NS_PDR_SLUA

#endif // _WIN32