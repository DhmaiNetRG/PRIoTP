#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "utils.h"
#include "logger.h"

/* return 1 if x<y, -1 if x>y, 0 if x==y */
int timeval_compare(const struct timeval *x, const struct timeval *y)
{
  if(x->tv_sec < y->tv_sec) return 1;
  if(x->tv_sec > y->tv_sec) return -1;

  if(x->tv_usec < y->tv_sec) return 1;
  if(x->tv_usec > y->tv_sec) return -1;

  return 0;
}

int timeval_add(struct timeval* result, const struct timeval *x, const struct timeval *y)
{
  result->tv_sec = x->tv_sec + y->tv_sec;
  result->tv_usec = x->tv_usec + y->tv_usec;
  if(result->tv_usec > 1000000) {
		int nsec = result->tv_usec / 1000000;
		result->tv_usec -= 1000000 * nsec;
		result->tv_sec += nsec;
  }

  return 0;
}

/* result = x-y */
int timeval_subtract(struct timeval *result, const struct timeval *x, struct timeval *y)
{
	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_usec < y->tv_usec) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000) {
		int nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}

	/* Compute the time remaining to wait.
	 *			tv_usec is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;

	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}

/* Return 0 if two addresses are the same */
int compare_sockaddr(struct sockaddr_storage* a1, struct sockaddr_storage* a2)
{
  if( a1->ss_family != a2->ss_family ) return 1;
  if( a1->ss_family == AF_INET ) {
    if( ((struct sockaddr_in*)a1)->sin_port != ((struct sockaddr_in*)a2)->sin_port ) return 2;
    if( ((struct sockaddr_in*)a1)->sin_addr.s_addr != ((struct sockaddr_in*)a2)->sin_addr.s_addr ) return 3;
    return 0;
  }
  if( a1->ss_family == AF_INET6 ) {
    if( ((struct sockaddr_in6*)a1)->sin6_port != ((struct sockaddr_in6*)a2)->sin6_port ) return 4;
    /* TODO Compare IPv6 addresses */
    return 0;
  }
  return 6;
}

/* Get address information, create socket and bind to the port */
int init_socket(const char* hostname, in_port_t port, bool server_side, const char* localaddr)
{
  char str_port[6] = {0};
  struct addrinfo hints;
  struct sockaddr_in sin;
  struct addrinfo *res, *cur;
  int sd;
  int err = 0;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_flags = AI_NUMERICSERV | AI_PASSIVE;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;

  sprintf(str_port, "%u", port);
  err = getaddrinfo(hostname, str_port, &hints, &res);
  if( err != 0 ) {
    fprintf(stderr, "Socket init, getaddrinfo(): %s\n", gai_strerror(err));
    return -1;
  }

  for( cur = res; cur != NULL; cur = cur->ai_next )
  {
    if( (sd = socket( cur->ai_family, cur->ai_socktype, cur->ai_protocol )) == -1 )
    {
      perror("Socket init, socket():");
      continue;
    }
    if( server_side && (bind(sd, cur->ai_addr, cur->ai_addrlen) == -1) )
    {
      perror("Socket init, bind():");
      close(sd);
      continue;
    }
    if( !server_side ) {
      if (localaddr != NULL) {
        memset(&sin, 0, sizeof(struct sockaddr_in));
        sin.sin_family = AF_INET;
        sin.sin_port = 0;
        sin.sin_addr.s_addr = inet_addr(localaddr);
        printf("binding");
        if ( bind(sd, (struct sockaddr *)&sin, sizeof(struct sockaddr)) == -1) {
          perror("Local socket binding, bind():");
          continue;
        }
      }
      if ( connect(sd, cur->ai_addr, cur->ai_addrlen) == -1) {
        perror("Socket init, connect():");
        continue;
      }
    }
    break;
  }

  if( cur == NULL ) sd = -1;

  freeaddrinfo(res);
  return sd;
}

void * xalloc(size_t sz) {
    void * ptr = malloc(sz);
    if (!ptr) {
        perror("error");
        exit(errno);
    }
    memset(ptr, 0, sz);
    return ptr;
}

