#ifndef ___PDR_USOCKET_H___
#define ___PDR_USOCKET_H___
/*=========================================================================*\
* Socket compatibilization module for Unix
* LuaSocket toolkit
\*=========================================================================*/

/*=========================================================================*\
* BSD include files
\*=========================================================================*/
/* error codes */
#include <errno.h>
/* close function */
#include <unistd.h>
/* fnctnl function and associated constants */
#include <fcntl.h>
/* struct sockaddr */
#include <sys/types.h>
/* socket function */
#include <sys/socket.h>
/* struct timeval */
#include <sys/time.h>
/* gethostbyname and gethostbyaddr functions */
#include <netdb.h>
/* sigpipe handling */
#include <signal.h>
/* IP stuff*/
#include <netinet/in.h>
#include <arpa/inet.h>
/* TCP options (nagle algorithm disable) */
#include <netinet/tcp.h>
#include <net/if.h>

#ifndef SO_REUSEPORT
#define SO_REUSEPORT SO_REUSEADDR
#endif

/* Some platforms use IPV6_JOIN_GROUP instead if
 * IPV6_ADD_MEMBERSHIP. The semantics are same, though. */
#ifndef IPV6_ADD_MEMBERSHIP
#ifdef IPV6_JOIN_GROUP
#define IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#endif /* IPV6_JOIN_GROUP */
#endif /* !IPV6_ADD_MEMBERSHIP */

/* Same with IPV6_DROP_MEMBERSHIP / IPV6_LEAVE_GROUP. */
#ifndef IPV6_DROP_MEMBERSHIP
#ifdef IPV6_LEAVE_GROUP
#define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP
#endif /* IPV6_LEAVE_GROUP */
#endif /* !IPV6_DROP_MEMBERSHIP */

namespace NS_PDR_SLUA {    

#if PLATFORM_WINDOWS
#else
typedef socklen_t ___pdr_socklen_t;
#endif
typedef int ___pdr_t_socket;
typedef ___pdr_t_socket *___pdr_p_socket;
typedef struct sockaddr_storage ___pdr_t_sockaddr_storage;

#define SOCKET_INVALID (-1)

} // end NS_PDR_SLUA

#endif /* USOCKET_H */
