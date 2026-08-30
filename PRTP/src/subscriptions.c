// #include "subscriptions.h"
// #include <stdlib.h>
// #include <string.h>
// #include <math.h>
// #include "utils.h"
// #include "logger.h"
// #include "clients_config.h"

// static struct subscription_node* subscriptions_list = NULL;
// static struct logger* l = NULL;

// void init_subscriptions()
// {
//   l = init_logger(stdout, stderr, stderr, "Subscriptions");
//   init_clients_config();
// }

// void shutdown_subscriptions()
// {
//   shutdown_logger(l);
//   shutdown_clients_config();
// }

// int send_update( struct subscription_node* node )
// {
//   int ret = -1;
//   struct PRTP_packet* msg = create_iotmsg(UPDATE);
//   struct PRTP_packet* upd_msg;
  
//   if (msg == NULL) {
//     return ret;
//   }

//   upd_msg  = msg;

//   upd_msg->seq_no = node->seq_no;
//   upd_msg->reliable = node->reliable;
//   iotmsg_set_sid( msg, node->sensor->id );  //--fix
//   upd_msg->data.sensor_type = node->sensor->type; //--fix
//   upd_msg->data.sid = node->sensor->id; //--fix


//   /* Set zeros here, the fragmentation is taken care of on the lower layers */
//   upd_msg->frag_no = 0;
//   upd_msg->frag_total = 0;
//   log_debug( l, "sending update. %s %d\n", msg->data.sid ,  msg->data.sensor_type );
//   if( iotmsg_set_data( msg, &(node->sensor->data) ) != -1 ) {
//   log_debug( l, "checking:: sending update. %s %d\n", msg->data.sid ,  msg->data.sensor_type );
//     ret = send_client_message( &(node->t_status), msg, node->client );
//   }

//   free_iotmsg(msg);
//   return ret;
// }

// int send_updates(int* timeout_msecs)
// {
//   struct subscription_node* node;
//   bool node_update;
//   bool retransmit;
//   struct timeval no_time = {0, 0};
//   struct timeval tv;
//   struct timeval next_transmission = no_time;
//   struct timeval next_event = no_time;
//   struct timeval timeout_val;
//   *timeout_msecs = RETRANSMIT_TIMEOUT;

//   for( node = subscriptions_list; node != NULL; node = node->next ) {
//     node_update = node->updated;
//     if( node->reliable )
//     {
//       retransmit = transport_retransmit( &(node->t_status) );
//       node_update |= retransmit;
//       if( retransmit ) log_debug( l, "Retransmitting the update.\n" );
//     }

//     if( node_update ) {
//       if( send_update( node ) >= 0 ) {
//         node->updated = false;
//       }
//     }
//     transmit_from_queue( &(node->client->transport), &(node->t_status), &next_transmission );
//     if( timeval_compare(&next_transmission, &no_time) ) {
//       if( !timeval_compare(&next_event, &no_time) || 
//           (timeval_compare(&next_transmission, &next_event) == -1) ) next_event = next_transmission;
//     }
//   }

//   /* Calculate the poll timeout till the next scheduled packet */
//   gettimeofday(&tv, 0);
//   if( !timeval_subtract(&timeout_val, &next_event, &tv) )
//     *timeout_msecs = timeout_val.tv_sec*1000 + timeout_val.tv_usec/1000;

//   if(*timeout_msecs > RETRANSMIT_TIMEOUT) *timeout_msecs = RETRANSMIT_TIMEOUT;

//   return 0;
// }

// void subscription_on_ack(const struct client_node* client, const struct sensor_node* sensor, int seq_no, int len)
// {
//   struct subscription_node* node;
//   struct transport_packet pkt;

//   /* Inform transport that the ack is received */
//   pkt.seq_no = seq_no;
//   pkt.len = len;
//   pkt.frag_no = 0;
//   pkt.frag_total = 1;

//   for( node = subscriptions_list; node != NULL; node = node->next ) {
//     if( node->client == client && node->sensor == sensor ) {
//       transport_on_received(&(node->t_status), &pkt);
//     }
//   }
// }


// /* XXX: Ugly, should not modify client datastructure (client should be const) */
// void subscription_on_nack(struct client_node* client, const struct sensor_node* sensor)
// {
//   struct subscription_node* node;
//   for ( node = subscriptions_list; node != NULL; node = node->next ) {
//     if ( node->client == client && node->sensor == sensor ) {
//         client->transport.skip_next = true;
//     }
//   }
// }

// void mark_updated_subscriptions(const struct sensor_node* sensor)
// {
//   struct subscription_node* node;

//   for( node = subscriptions_list; node != NULL; node = node->next ) {
//     if( node->sensor == sensor ) {
//       node->updated = true;
//       node->seq_no++;
//     }
//   }
// }

// bool subscription_exists(const struct client_node* client, const struct sensor_node* sensor)
// {
//   struct subscription_node* node;

//   for( node = subscriptions_list; node != NULL; node = node->next ) {
//     if( (node->sensor == sensor) && (node->client == client) )
//       return true;
//   }
//   return false;
// }

// void add_subscription(struct client_node* client, const struct sensor_node* sensor, bool reliable)
// {
//   struct subscription_node* node = xalloc(sizeof(struct subscription_node));

//   node->client = client;
//   node->sensor = sensor;
//   node->updated = true;
//   node->reliable = reliable;
//   node->seq_no = 0;

//   get_scheduler(&(node->t_status.sched), client);

//   node->next = subscriptions_list;
//   subscriptions_list = node;
//   if( reliable ) log_debug(l, "Reliable subscription added. ");
//   else log_debug(l, "Unreliable subscription added. ");
//   log_debug(l, "%d delay, %f p_prob, %f q_prob.\n", node->t_status.sched.delay_msecs,
//             node->t_status.sched.p_prob, node->t_status.sched.q_prob);
// }

// void remove_client_subscriptions(const struct client_node* client)
// {
//   struct subscription_node* node = subscriptions_list, *prev = NULL;

//   while( node != NULL ) {
//     if( node->client == client ) {
//       if( prev ) prev->next = node->next;
//       else subscriptions_list = node->next;
//       free_transport_status(&(node->t_status));
//       free(node);
//       /* This is to free the node safely. List routines in utils would be nice to have */
//       /* There was a bug here when the head gets free'd  */
//       if( prev ) node = prev->next; else node = subscriptions_list;
//       if( node == NULL ) break;
//     }
//     prev = node;
//     node = node->next;
//   }
// }


#include "subscriptions.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "utils.h"
#include "logger.h"
#include "clients_config.h"
#include "q_agent.h"

static struct subscription_node* subscriptions_list = NULL;
static struct logger* l = NULL;

void init_subscriptions()
{
  l = init_logger(stdout, stderr, stderr, "Subscriptions");
  init_clients_config();
}

void shutdown_subscriptions()
{
  shutdown_logger(l);
  shutdown_clients_config();
}

int send_update( struct subscription_node* node )
{
  int ret = -1;
  struct PRTP_packet* msg = create_iotmsg(UPDATE);
  
  if (msg == NULL) {
    return ret;
  }

  /* Set PRTP packet fields using bitfield */
  msg->seq_no = node->seq_no;
  msg->reliable = node->reliable;
  
  /* Set sensor ID using the proper accessor function */
  iotmsg_set_sid(msg, node->sensor->id);
  
  /* Set sensor type using the proper accessor function */
  iotmsg_set_sensor_type(msg, node->sensor->type);

  /* Set zeros here, the fragmentation is taken care of on the lower layers */
  msg->frag_no = 0;
  msg->frag_total = 0;
  
  log_debug(l, "sending update. %s %d\n", 
            msg->data.update.sid, msg->data.update.sensor_type);
  
  // if (iotmsg_set_data(msg, &(node->sensor->data)) != -1) {
  //   log_debug(l, "checking:: sending update. %s %d\n", 
  //             msg->data.update.sid, msg->data.update.sensor_type);
  //   ret = send_client_message(&(node->t_status), msg, node->client);
  // }

    if (iotmsg_set_data(msg, &(node->sensor->data)) != -1) {
    log_debug(l, "checking:: sending update. %s %d\n",
              msg->data.update.sid, msg->data.update.sensor_type);

    /* --- Q-agent decision --- */
    extern q_agent_t server_q_agent;
    extern bool q_learning_enabled;

    /* Enhanced Q-learning section in subscriptions.c send_update() 
 * Add this code right after the importance mapping
 */

    if (q_learning_enabled) {
      /* Estimate RTT / network state */
      int estimated_rtt = node->t_status.sched.delay_msecs;
      int rtt_level = (estimated_rtt > 150) ? 1 : 0;

      /* Map semantic -> importance */
      int importance = 1; /* default NORMAL */
      const char* sensor_type_str = "UNKNOWN";
      
      if (node->sensor->type == CAMERA) {
        sensor_type_str = "CAMERA";
        if (node->sensor->data.blob && strncmp((char*)node->sensor->data.blob, "NO_MOTION", 9) == 0) {
          importance = 0;  // LOW
        } else {
          importance = 2;  // HIGH (motion detected)
        }
      } else if (node->sensor->type == TEMP) {
        sensor_type_str = "TEMP";
        importance = 1;  // NORMAL
      } else if (node->sensor->type == DEVICE) {
        sensor_type_str = "DEVICE";
        importance = 1;  // NORMAL (or 2 for STATUS_CHANGE)
      } else if (node->sensor->type == GPS) {
        sensor_type_str = "GPS";
        importance = 1;  // NORMAL
      }

      int action = q_agent_choose_action(&server_q_agent, rtt_level, importance);

      /* ENHANCED LOGGING - Use log_print to always show decisions */
      const char* action_str[] = {"UNRELIABLE", "RELIABLE", "DROP"};
      const char* imp_str[] = {"LOW", "NORMAL", "HIGH"};
      
      log_print(l, "🤖 Q-Decision: %-10s | RTT=%dms(lvl=%d) | IMP=%-6s | ACTION=%-10s | Q=%.2f\n",
                node->sensor->id,
                estimated_rtt, rtt_level,
                imp_str[importance],
                action_str[action],
                server_q_agent.Q[rtt_level][importance][action]);

      if (action == 2) {
        /* Drop packet */
        log_print(l, "   ❌ DROPPED\n");
        free_iotmsg(msg);
        return -1;
      } else if (action == 1) {
        msg->reliable = 1;
      } else {
        msg->reliable = 0;
      }

      /* Store state/action for feedback */
      node->q_state = (rtt_level << 4) | (importance << 2) | action;
      node->q_action = action;
    }

    ret = send_client_message(&(node->t_status), msg, node->client);
  }

  free_iotmsg(msg);
  return ret;
}

int send_updates(int* timeout_msecs)
{
  struct subscription_node* node;
  bool node_update;
  bool retransmit;
  struct timeval no_time = {0, 0};
  struct timeval tv;
  struct timeval next_transmission = no_time;
  struct timeval next_event = no_time;
  struct timeval timeout_val;
  *timeout_msecs = RETRANSMIT_TIMEOUT;

  for( node = subscriptions_list; node != NULL; node = node->next ) {
    node_update = node->updated;
    if( node->reliable )
    {
      retransmit = transport_retransmit( &(node->t_status) );
      node_update |= retransmit;
      if( retransmit ) log_debug( l, "Retransmitting the update.\n" );
    }

    if( node_update ) {
      if( send_update( node ) >= 0 ) {
        node->updated = false;
      }
    }
    transmit_from_queue( &(node->client->transport), &(node->t_status), &next_transmission );
    if( timeval_compare(&next_transmission, &no_time) ) {
      if( !timeval_compare(&next_event, &no_time) || 
          (timeval_compare(&next_transmission, &next_event) == -1) ) next_event = next_transmission;
    }
  }

  /* Calculate the poll timeout till the next scheduled packet */
  gettimeofday(&tv, 0);
  if( !timeval_subtract(&timeout_val, &next_event, &tv) )
    *timeout_msecs = timeout_val.tv_sec*1000 + timeout_val.tv_usec/1000;

  if(*timeout_msecs > RETRANSMIT_TIMEOUT) *timeout_msecs = RETRANSMIT_TIMEOUT;

  return 0;
}

void subscription_on_ack(const struct client_node* client, const struct sensor_node* sensor, int seq_no, int len)
{
  struct subscription_node* node;
  struct transport_packet pkt;

  /* Inform transport that the ack is received */
  pkt.seq_no = seq_no;
  pkt.len = len;
  pkt.frag_no = 0;
  pkt.frag_total = 1;

  for( node = subscriptions_list; node != NULL; node = node->next ) {
    if( node->client == client && node->sensor == sensor ) {
      extern q_agent_t server_q_agent;
      extern bool q_learning_enabled;
      
      if (q_learning_enabled && node->q_action >= 0) {
          float reward = 20.0 - (len / 100.0);  /* +20 for HIGH importance, -latency cost */
          int next_rtt = 0;
          int next_importance = 1;
          
          int rtt_state = (node->q_state >> 4) & 1;
          int importance = (node->q_state >> 2) & 3;
          int action = node->q_state & 3;
          
          q_agent_update(&server_q_agent, rtt_state, importance, action, reward, next_rtt, next_importance);
          log_debug(l, "Q-Agent ACK reward: %.2f\n", reward);
      }
      transport_on_received(&(node->t_status), &pkt);
    }
  }
}

/* XXX: Ugly, should not modify client datastructure (client should be const) */
void subscription_on_nack(struct client_node* client, const struct sensor_node* sensor)
{
  extern q_agent_t server_q_agent;
  extern bool q_learning_enabled;
  struct subscription_node* node;
  for ( node = subscriptions_list; node != NULL; node = node->next ) {
    if ( node->client == client && node->sensor == sensor ) {
        if (q_learning_enabled && node->q_action >= 0) {
          float reward = -200.0;  /* Harsh penalty for losing an update */
          int rtt_state = (node->q_state >> 4) & 1;
          int importance = (node->q_state >> 2) & 3;
          int action = node->q_state & 3;
          
          q_agent_update(&server_q_agent, rtt_state, importance, action, reward, rtt_state, importance);
          log_debug(l, "Q-Agent NACK penalty: -200\n");
      }
      client->transport.skip_next = true;
    }
  }
}

void mark_updated_subscriptions(const struct sensor_node* sensor)
{
  struct subscription_node* node;

  for( node = subscriptions_list; node != NULL; node = node->next ) {
    if( node->sensor == sensor ) {
      node->updated = true;
      node->seq_no++;
    }
  }
}

bool subscription_exists(const struct client_node* client, const struct sensor_node* sensor)
{
  struct subscription_node* node;

  for( node = subscriptions_list; node != NULL; node = node->next ) {
    if( (node->sensor == sensor) && (node->client == client) )
      return true;
  }
  return false;
}

void add_subscription(struct client_node* client, const struct sensor_node* sensor, bool reliable)
{
  struct subscription_node* node = xalloc(sizeof(struct subscription_node));

  node->client = client;
  node->sensor = sensor;
  node->updated = true;
  node->reliable = reliable;
  node->seq_no = 0;

  get_scheduler(&(node->t_status.sched), client);

  node->next = subscriptions_list;
  subscriptions_list = node;
  if( reliable ) log_debug(l, "Reliable subscription added. ");
  else log_debug(l, "Unreliable subscription added. ");
  log_debug(l, "%d delay, %f p_prob, %f q_prob.\n", node->t_status.sched.delay_msecs,
            node->t_status.sched.p_prob, node->t_status.sched.q_prob);
}

void remove_client_subscriptions(const struct client_node* client)
{
  struct subscription_node* node = subscriptions_list, *prev = NULL;

  while( node != NULL ) {
    if( node->client == client ) {
      if( prev ) prev->next = node->next;
      else subscriptions_list = node->next;
      free_transport_status(&(node->t_status));
      free(node);
      /* This is to free the node safely. List routines in utils would be nice to have */
      /* There was a bug here when the head gets free'd  */
      if( prev ) node = prev->next; else node = subscriptions_list;
      if( node == NULL ) break;
    }
    prev = node;
    node = node->next;
  }
}
