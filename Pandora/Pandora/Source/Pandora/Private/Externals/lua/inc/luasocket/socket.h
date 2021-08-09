#ifndef ___PDR_SOCKET_H___
#define ___PDR_SOCKET_H___
/*=========================================================================*\
* Socket compatibilization module
* LuaSocket toolkit
*
* BSD Sockets and WinSock are similar, but there are a few irritating
* differences. Also, not all *nix platforms behave the same. This module
* (and the associated usocket.h and wsocket.h) factor these differences and
* creates a interface compatible with the io.h module.
\*=========================================================================*/
#include "io.h"

/*=========================================================================*\
* Platform specific compatibilization
\*=========================================================================*/
#ifdef _WIN32
#include "wsocket.h"
#else
#include "usocket.h"
#endif

/*=========================================================================*\
* The connect and accept functions accept a timeout and their
* implementations are somewhat complicated. We chose to move
* the timeout control into this module for these functions in
* order to simplify the modules that use them. 
\*=========================================================================*/
#include "timeout.h"

namespace NS_PDR_SLUA {    

/* we are lazy... */
typedef struct sockaddr ___pdr_SA;

/*=========================================================================*\
* Functions bellow implement a comfortable platform independent 
* interface to sockets
\*=========================================================================*/
int ___pdr_socket_open(void);
int ___pdr_socket_close(void);
void ___pdr_socket_destroy(___pdr_p_socket ps);
void ___pdr_socket_shutdown(___pdr_p_socket ps, int how); 
int ___pdr_socket_sendto(___pdr_p_socket ps, const char *data, size_t count, 
        size_t *sent, ___pdr_SA *addr, ___pdr_socklen_t addr_len, ___pdr_p_timeout tm);
int ___pdr_socket_recvfrom(___pdr_p_socket ps, char *data, size_t count, 
        size_t *got, ___pdr_SA *addr, ___pdr_socklen_t *addr_len, ___pdr_p_timeout tm);

void ___pdr_socket_setnonblocking(___pdr_p_socket ps);
void ___pdr_socket_setblocking(___pdr_p_socket ps);

int ___pdr_socket_waitfd(___pdr_p_socket ps, int sw, ___pdr_p_timeout tm);
int ___pdr_socket_select(___pdr_t_socket n, fd_set *rfds, fd_set *wfds, fd_set *efds, 
        ___pdr_p_timeout tm);

int ___pdr_socket_connect(___pdr_p_socket ps, ___pdr_SA *addr, ___pdr_socklen_t addr_len, ___pdr_p_timeout tm); 
int ___pdr_socket_create(___pdr_p_socket ps, int domain, int type, int protocol);
int ___pdr_socket_bind(___pdr_p_socket ps, ___pdr_SA *addr, ___pdr_socklen_t addr_len); 
int ___pdr_socket_listen(___pdr_p_socket ps, int backlog);
int ___pdr_socket_accept(___pdr_p_socket ps, ___pdr_p_socket pa, ___pdr_SA *addr, 
        ___pdr_socklen_t *addr_len, ___pdr_p_timeout tm);

const char *___pdr_socket_hoststrerror(int err);
const char *___pdr_socket_gaistrerror(int err);
const char *___pdr_socket_strerror(int err);

/* these are perfect to use with the io abstraction module 
   and the buffered input module */
int ___pdr_socket_send(___pdr_p_socket ps, const char *data, size_t count, 
        size_t *sent, ___pdr_p_timeout tm);
int ___pdr_socket_recv(___pdr_p_socket ps, char *data, size_t count, size_t *got, ___pdr_p_timeout tm);
int ___pdr_socket_write(___pdr_p_socket ps, const char *data, size_t count, 
        size_t *sent, ___pdr_p_timeout tm);
int ___pdr_socket_read(___pdr_p_socket ps, char *data, size_t count, size_t *got, ___pdr_p_timeout tm);
const char *___pdr_socket_ioerror(___pdr_p_socket ps, int err);

int ___pdr_socket_gethostbyaddr(const char *addr, ___pdr_socklen_t len, struct hostent **hp);
int ___pdr_socket_gethostbyname(const char *addr, struct hostent **hp);

} // end NS_PDR_SLUA

#endif /* SOCKET_H */
