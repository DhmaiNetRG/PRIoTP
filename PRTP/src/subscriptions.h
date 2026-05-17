#ifndef SUBSCRIPTIONS_H
#define SUBSCRIPTIONS_H

#include "sensors.h"
#include "clients.h"
#include "transport.h"
#include <stdbool.h>
#include <sys/time.h>

enum SUBSCRIBE_STATUS {
  SUBSCRIBE_OK,
  SUBSCRIBE_NOT_FOUND,
  SUBSCRIBE_ALREADY_EXISTS
};

/* Holds information about a subscription
 * matches sensor with client
 *   updated is true if the sensor is updated and the update should be sent
 *   ack_pending is true if the update is sent but ack is not received
 *     now handled by transport
 *   reliable is true if acks should be received
 *   last_sent - used to be time when the last update was sent,
 *     now can get it from transport
 *   seq_no - last sent sequence number
 */
struct subscription_node {
  struct subscription_node* next;
  const struct sensor_node* sensor;
  /* Used to be const, now client contains transport that has
   * fragmentation buffers that change when fragmentation happens */
  struct client_node* client;
  bool updated;
  /* bool ack_pending; */
  bool reliable;
  /* struct timeval last_sent; */
  int seq_no;

  struct transport_status t_status;
    int q_state;      /* Store (rtt, importance, action) for feedback */
  int q_action;
};

/* Scan through subscriptions, construct and send necessary updates */
int send_updates(int* timeout_msecs);

/* Mark sensor subscriptions updated */
void mark_updated_subscriptions(const struct sensor_node* sensor);
  
void add_subscription(struct client_node* client, const struct sensor_node* sensor, bool reliable);

/* Remove all subscriptions of the client from the list */
void remove_client_subscriptions(const struct client_node* client);

/* Reset pending acknowldgment flag, notify transport about ack */
void subscription_on_ack(const struct client_node* client, const struct sensor_node* sensor, int seq_no, int len);

void subscription_on_nack(struct client_node* client, const struct sensor_node* sensor);

bool subscription_exists(const struct client_node* client, const struct sensor_node* sensor);

void init_subscriptions();
void shutdown_subscriptions();

#endif
