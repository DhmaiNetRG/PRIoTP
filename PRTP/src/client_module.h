/**
 * @file client_module.h
 * @brief Client-side protocol state and message handling.
 * 
 * Manages client subscription lifecycle, pending requests, fragmentation,
 * and active flow tracking for client-side PRIoTP implementation.
 * 
 * @author Hämäläinen, Harri (harri.hamalainen@aalto.fi)
 * @author Kurnikov, Arseny (arseny.kurnikov@aalto.fi)
 * @author Rahman Mehraj (rahmanmehraj627@gmail.com)
 * @author Hasan Mahmood (hasan.mahmood@ewubd.edu)
 */

#ifndef CLIENT_CLIENT_H
#define CLIENT_CLIENT_H

#include <sys/time.h>
#include "messages.h"
#include "logger.h"

#define BUFSIZE 1600  /**< Maximum message buffer size */

/**
 * @struct pending_sub_request
 * @brief Pending subscription request awaiting acknowledgment.
 */
struct pending_sub_request
{
  struct pending_sub_request* next;  /**< Linked list pointer */
  struct PRTP_packet* submsg;        /**< Original subscription message */
  struct timeval time_sent;          /**< Request transmission timestamp */
};

/**
 * @brief Initialize client protocol module.
 */
void init_client_module();

/**
 * @brief Shutdown client protocol module.
 */
void shutdown_client_module();

/**
 * @brief Transmit PRIoTP message to server.
 * @param sd Socket descriptor
 * @param msg Message to send
 * @return Bytes transmitted, negative on error
 */
int send_iotmsg(int sd, const struct PRTP_packet* msg);

/**
 * @brief Send keep-alive heartbeat.
 * @param sd Socket descriptor
 * @return Bytes transmitted, negative on error
 */
int send_keep_alive(int sd);

/**
 * @brief Subscribe to sensors with reliability option.
 * @param sd Socket descriptor
 * @param sids Sensor identifier list
 * @param reliable Request reliable delivery
 * @return Status code
 */
int send_subscribe_all(int sd, struct iotmsg_node* sids, bool reliable);

/**
 * @brief Unsubscribe from all subscriptions.
 * @param sd Socket descriptor
 * @return Status code
 */
int send_unsubscribe(int sd);

/**
 * @brief Acknowledge received update message.
 * @param sd Socket descriptor
 * @param upd_msg Update message to acknowledge
 * @return Status code
 */
int send_update_ack(int sd, struct PRTP_packet* upd_msg);

/**
 * @brief Query available sensor list from server.
 * @param sd Socket descriptor
 * @return Status code
 */
int query_sensor_list(int sd);

/**
 * @brief Display received sensor list.
 * @param msg List response message
 */
void print_list_response(struct PRTP_packet* msg);

/**
 * @brief Free pending subscription request.
 * @param subreq Request to free
 */
void free_pending_sub_request(struct pending_sub_request * subreq);

/**
 * @brief Remove pending subscription by sensor ID.
 * @param node Sensor identifier node
 */
void remove_pending_sub_request(struct iotmsg_node * node);

/**
 * @brief Record pending subscription request.
 * @param msg Subscription message
 */
void add_pending_subscription(struct PRTP_packet * msg);

/**
 * @brief Process pending subscriptions with timeout.
 * @param sd Socket descriptor for retry
 */
void update_pending_subscriptions(int sd);

/**
 * @brief Register fragment buffer for sensor.
 * @param sid Sensor identifier
 */
void add_fragment_buffer( char* sid );

/**
 * @brief Retrieve fragment buffer by sensor.
 * @param sid Sensor identifier
 * @return Pointer to fragment buffer, NULL if not found
 */
struct fragment_buffer* get_frag_buffer( char* sid );

/**
 * @brief Register active flow for sensor.
 * @param sid Sensor identifier
 */
void add_active_flow( char* sid );

/**
 * @brief Retrieve active flow by sensor.
 * @param sid Sensor identifier
 * @return Pointer to active flow, NULL if not found
 */
struct active_flow* get_active_flow( char* sid );

/**
 * @brief Validate packet flow sequence and send recovery ACK if needed.
 * @param upd_msg Update message to validate
 * @param sd Socket descriptor for ACK transmission
 */
void check_active_flow(struct PRTP_packet* upd_msg, int sd);

#endif
