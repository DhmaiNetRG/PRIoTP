/**
 * @file clients.h
 * @brief Client connection management for PRIoTP protocol.
 * 
 * Implements client registry and communication primitives for server-side
 * client tracking, message delivery, and connection lifecycle management.
 * 
 * @author Hämäläinen, Harri (harri.hamalainen@aalto.fi)
 * @author Kurnikov, Arseny (arseny.kurnikov@aalto.fi)
 * @author Rahman Mehraj (rahmanmehraj627@gmail.com)
 * @author Hasan Mahmood (hasan.mahmood@ewubd.edu)
 */

#ifndef CLIENTS_H
#define CLIENTS_H

#include <sys/time.h>
#include "transport.h"
#include "messages.h"

/**
 * @struct client_node
 * @brief Represents an active client connection.
 * 
 * Maintains client state including transport context, connection
 * timestamp for timeout detection.
 */
struct client_node {
  struct client_node* next;     /**< Linked list pointer */
  struct transport transport;   /**< Transport layer context */
  struct timeval last_seen;     /**< Last activity timestamp */
};

/**
 * @brief Establish socket connection to remote client.
 * @param hostname Client hostname or IP address
 * @param port Destination port number
 * @return Socket file descriptor on success, negative on error
 */
int client_socket(const char* hostname, in_port_t port);

/**
 * @brief Send PRIoTP message to connected client.
 * @param t_status Transport status context
 * @param msg PRIoTP packet to transmit
 * @param node Target client node
 * @return Number of bytes sent, negative on error
 */
int send_client_message(struct transport_status* t_status, struct PRTP_packet* msg, struct client_node* node);

/**
 * @brief Receive and parse message from client socket.
 * @param sd Socket descriptor
 * @param node Pointer to receive client node reference
 * @param msg Pointer to receive parsed PRIoTP packet
 * @return Message status code
 */
int read_client(int sd, struct client_node** node, struct PRTP_packet** msg);

/**
 * @brief Remove inactive clients from registry.
 * @return Number of clients pruned
 */
int prune_expired_clients();

/**
 * @brief Display client connection details.
 * @param node Client node to display
 */
void print_client(struct client_node* node);

/**
 * @brief Initialize client registry.
 */
void init_clients();

/**
 * @brief Shutdown and cleanup client registry.
 */
void shutdown_clients();

#endif
