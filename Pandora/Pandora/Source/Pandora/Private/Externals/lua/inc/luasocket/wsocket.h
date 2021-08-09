#ifndef ___PDR_WSOCKET_H___
#define ___PDR_WSOCKET_H___
/*=========================================================================*\
* Socket compatibilization module for Win32
* LuaSocket toolkit
\*=========================================================================*/

/*=========================================================================*\
* WinSock include files
\*=========================================================================*/
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>

namespace NS_PDR_SLUA {    

typedef int ___pdr_socklen_t;
typedef SOCKADDR_STORAGE ___pdr_t_sockaddr_storage;
typedef SOCKET ___pdr_t_socket;
typedef ___pdr_t_socket *___pdr_p_socket;

#ifndef IPV6_V6ONLY
#define IPV6_V6ONLY 27
#endif

#define SOCKET_INVALID (INVALID_SOCKET)

#ifndef SO_REUSEPORT
#define SO_REUSEPORT SO_REUSEADDR
#endif

#ifndef AI_NUMERICSERV
#define AI_NUMERICSERV (0)
#endif

} // end NS_PDR_SLUA
#endif
#endif /* WSOCKET_H */
