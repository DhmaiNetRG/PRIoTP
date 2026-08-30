#ifndef UTILS_H
#define UTILS_H

#include <netinet/in.h>
#include <stdbool.h>
#include <sys/time.h>

/* Put it here for the time being, maybe separate spec_config.h would be good */
/* 1000 msec = 1 second */
#define KEEP_ALIVE_INTERVAL 5000
#define KEEP_ALIVE_TIMEOUT 15000
#define RETRANSMIT_TIMEOUT 200
#define MAX_SENSOR_DATA_PER_DGRAM 512

int timeval_compare(const struct timeval* x, const struct timeval* y);
int timeval_add(struct timeval* result, const struct timeval* x, const struct timeval* y);
int timeval_subtract (struct timeval *result, const struct timeval *x, struct timeval *y);
int compare_sockaddr(struct sockaddr_storage* a1, struct sockaddr_storage* a2);
int init_socket(const char* hostname, in_port_t port, bool server_side, const char* localaddr);
void * xalloc(size_t sz);

#endif
