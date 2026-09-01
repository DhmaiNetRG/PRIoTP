#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <sys/time.h>
#include <signal.h>
#include <assert.h>

#include "PRTP_client.h"
#include "../src/utils.h"
#include "../src/client_module.h"
#include "../src/logger.h"
#include "../src/bson_parser.h"
#include "../src/bson_msg.h"
#include "../src/messages.h"
#include "../src/security.h"
#include "../src/subscriptions.h"
#include "../src/sensor_logger.h"
#include "../src/fragment_buffer.h"


#define HOSTNAME_SIZE 255
#define INPUTSIZE 10

static int operation = -1;
static bool all_reliable = false;
static bool ui_enabled = false;
static struct logger* l = NULL;


/* Client tracks seq numbers and reports missing packets with NACKs when
   unreliable connections are used
   */
static bool active_flow_enabled = false;

void usage() {
  printf("iotclient [-l log_dir] [-a|-A] [-q] [-s <server_ip>] [-p <server_port>] [-r sensor_id] [sensor_id_1, sensor_id_2, ...]\n");
  printf("\t-l\t\t  Specifies the directory for output sensor logs\n");
  printf("\t-s <server_ip>\t  The IP address of the IoT server, default localhost\n");
  printf("\t-p <server_port>  The port number of the IoT server, default 5001\n");
  printf("\t-r <sensor_id>\t  Subscribes reliably to the sensor\n");
  printf("\t-H <client_ip>\t  The IP address of the client, default localhost\n");
  printf("\t-a(-A)\t\t  Subscribes (reliably) to all sensors returned by the server\n");
  printf("\t-q\t\t  Queries the list of available sensor IDs from server and prints IDs into stdout and terminates the execution\n");
  printf("\t-g\t\t  Turns on prompting for operation mode\n");
  printf("\t-N\t\t  Turns on NACKs when weakly realible transport is not used\n");
}

void on_subscription_ack_sids(struct iotmsg_node * node) 
{
  for (; node != NULL; node = node->next ) {
     log_print( l, "what is happening\n");
    switch (((struct iotmsg_subscribe_ack_node *) node)->status) {
    case SUBSCRIBE_OK:
      log_print( l, "subscription ok for %s \n", node->id);
      add_sensor_logger( node->id );
      add_fragment_buffer( node->id );
      /* XXX: No need to add flows with reliable subscription here */
      if( active_flow_enabled )
        add_active_flow( node->id );
      if( parse_sensor_type( node->id ) == CAMERA ) add_sensor_dump(node->id);
      break;
    case SUBSCRIBE_ALREADY_EXISTS:
      log_print( l, "subscription already exists for %s \n", node->id);
      break;
    case SUBSCRIBE_NOT_FOUND:
      log_print( l, "subscription failed, no such sensor: %s \n", node->id);
      break;
    default:
      log_print( l, "unknown status code (%d) in subscription ack for %s \n",
          ((struct iotmsg_subscribe_ack_node *) node)->status, node->id);
      break;
    }
    remove_pending_sub_request( node );
  }
}


// void on_update_msg(int sd, struct PRTP_packet* upd_msg)
// {
//   log_error(l,"---before writing in log upd_msg->seq_no= %d\n", upd_msg->seq_no);
//   log_error(l,"---before writing in log upd_msg->type= %d\n", upd_msg->type);
//   log_error(l,"---before writing in log upd_msg->frag_no= %d\n", upd_msg->frag_no);
//   log_error(l,"---before writing in log upd_msg->reliable= %d\n", upd_msg->reliable);
//   log_error(l,"---before writing in log upd_msg->data.sensor_type= %d\n", upd_msg->data.sensor_type);
//   log_error(l,"---before writing in log upd_msg->data.sid= %s\n", upd_msg->data.sid);

//   sensor_log( upd_msg->data.sid, upd_msg->seq_no, upd_msg->data.sensor_type, &(upd_msg->data) );
//   log_error(l,"---after writing in log upd_msg->seq_no = %d\n", upd_msg->seq_no);
//   log_error(l,"---after writing in log upd_msg->data.sensor_type = %d\n", upd_msg->data.sensor_type);
//   log_error(l,"---after writing in log upd_msg->reliable = %d\n", upd_msg->reliable);

//   if( upd_msg->data.sensor_type == CAMERA ) {
//     if( strncmp(((char *)(upd_msg->data.blob)), "NO_MOTION", 9 ) != 0 ) {
//       sensor_dump( upd_msg->data.sid, &(upd_msg->data) );
//     }
//   }
 
//   if( upd_msg->reliable ) {
//     log_debug( l, "Sending acknowledgment.\n");
//     send_update_ack( sd, upd_msg );
//   } else if( active_flow_enabled ) {
//     log_debug( l, "Checking active (unreliable) flows for losses.\n");
//     check_active_flow( upd_msg, sd );
//   }
// }
void on_update_msg(int sd, struct PRTP_packet* upd_msg)
{
  log_error(l,"---before writing in log upd_msg->seq_no= %d\n", upd_msg->seq_no);
  log_error(l,"---before writing in log upd_msg->type= %d\n", upd_msg->type);
  log_error(l,"---before writing in log upd_msg->frag_no= %d\n", upd_msg->frag_no);
  log_error(l,"---before writing in log upd_msg->reliable= %d\n", upd_msg->reliable);
  
  /* Fix: Access through union for UPDATE message */
  log_error(l,"---before writing in log upd_msg->data.sensor_type= %d\n", 
            upd_msg->data.update.sensor_type);
  log_error(l,"---before writing in log upd_msg->data.sid= %s\n", 
            upd_msg->data.update.sid);

  /* Fix: Pass the correct union members */
  sensor_log(upd_msg->data.update.sid, 
             upd_msg->seq_no, 
             upd_msg->data.update.sensor_type, 
             &(upd_msg->data.update.blob));
  
  log_error(l,"---after writing in log upd_msg->seq_no = %d\n", upd_msg->seq_no);
  log_error(l,"---after writing in log upd_msg->data.sensor_type = %d\n", 
            upd_msg->data.update.sensor_type);
  log_error(l,"---after writing in log upd_msg->reliable = %d\n", upd_msg->reliable);

  /* Fix: Access sensor_type through union */
  if(upd_msg->data.update.sensor_type == CAMERA) {
    if(strncmp((char *)(upd_msg->data.update.blob.blob), "NO_MOTION", 9) != 0) {
      /* Fix: Pass the blob struct */
      sensor_dump(upd_msg->data.update.sid, &(upd_msg->data.update.blob));
    }
  }
 
  if(upd_msg->reliable) {
    log_debug(l, "Sending acknowledgment.\n");
    send_update_ack(sd, upd_msg);
  } else if(active_flow_enabled) {
    log_debug(l, "Checking active (unreliable) flows for losses.\n");
    check_active_flow(upd_msg, sd);
  }
}

int client_interface()
{
  int selected;
  printf("Client operation options: \n");
  printf(" 1. Query sensor list \n 2. Subscribe all \n 3. subscribe a particular sensor \n 4. Subscribe with reliability \n 5. Unsubscribe \n 6. Quit\n\n Enter your choice:");
  scanf("%d", &selected);
  return selected;
}


void process_user_input(int sd, int* client_operation)
{
  struct PRTP_packet* sub_msg = NULL;
  char input_sensor[INPUTSIZE];
  switch(*client_operation){
  case CLIENT_OPERATION_QUERY_SENSOR_LIST:
    query_sensor_list(sd);
    break;
  case CLIENT_OPERATION_SUBSCRIBE_ALL_SENSORS:
    /*subscribe to all sensors*/
    query_sensor_list(sd);
    break;
  case CLIENT_OPERATION_SUBSCRIBE_TO_SENSOR_NONRELIABLE:
    /*subscribe to specific sensor*/
    printf("Enter sensor_id =");
    scanf("%s", input_sensor);
    if(!sub_msg ) sub_msg = create_iotmsg(SUBSCRIBE);  
    iotmsg_add_sid(sub_msg, input_sensor);
    break;
  case CLIENT_OPERATION_SUBSCRIBE_TO_SENSOR_RELIABLE:
    /*subscribe to specific sensor with reliability*/
    printf("Enter sensor_id =");
    scanf("%s", input_sensor);
    if(!sub_msg ) sub_msg = create_iotmsg(SUBSCRIBE);
    iotmsg_add_sid(sub_msg, input_sensor );
    iotmsg_set_sid_reliable( sub_msg, true );					
    break;
  case CLIENT_OPERATION_UNSUBSCRIBE: /* 5 */
    /*unsubscribe*/
    send_unsubscribe(sd);
    break;
  case CLIENT_OPERATION_SHUTDOWN:
    /*shut down client*/
    return;
  default:
    break;
  }

  if( sub_msg ) {
    add_pending_subscription((struct PRTP_packet *) sub_msg);
    send_iotmsg(sd, sub_msg);
  }
}

void main_loop(int sd)
{
  struct pollfd fds[2];
  int timeout_msecs = KEEP_ALIVE_INTERVAL;
  struct timeval tv1, tv2, tvelapsed;
  int ret = 0;
  size_t len = 0;
  struct PRTP_packet* msg = NULL;
  int client_operation=-1;
  struct transport aux_tr;
  memset(&aux_tr, 0, sizeof(aux_tr));
  aux_tr.sd = sd;

  fds[0].fd = sd;
  fds[0].events = POLLIN;
  fds[1].fd = 0;
  fds[1].events = POLLIN;

  for(;;) {
    gettimeofday(&tv1, 0);
    if( ui_enabled ) ret = poll(fds, 2, timeout_msecs);
    else ret = poll(fds, 1, timeout_msecs);
    gettimeofday(&tv2, 0);
    if( ret > 0 ) {
      if( fds[1].revents & POLLIN ) {
        if ( ui_enabled )
          if ((client_operation=client_interface())) {
            process_user_input(sd, &client_operation);
          }
      }
      if( fds[0].revents & POLLIN ) {
        if( !timeval_subtract( &tvelapsed, &tv2, &tv1 ) ) {
          timeout_msecs -= tvelapsed.tv_sec*1000 + tvelapsed.tv_usec/1000;
          if( timeout_msecs < 0 ) timeout_msecs = 0;
        }
		
        msg = NULL;
        len = transport_receive(&aux_tr, &msg, NULL, NULL);
        if( len == -1 ) {
          log_error(l, "Couldn't get a message from the server.\n");
          break;
        } else if( len == 0 ) {
          log_error(l, "Got nothing from the server.\n");
          break;
        } else {
          if(msg == NULL) {
            log_error(l, "Got a malformed message or a fragment from the server.\n");
            continue;
          }
          else {
            log_print(l, "Got a message from the server.\n");
            log_print(l, "Message type is %d\n", msg->type);
          }

          if( msg->type == LIST_RESPONSE ) {
            print_list_response( msg);
            if( client_operation == 2||operation==2 ) {
              log_print(l, "Subscribing to all sensors.\n" );
              send_subscribe_all( sd, (msg)->data.blob, all_reliable );
            }
          }
          else if( msg->type == UPDATE ) {
           log_print(l, "---------Update message arrived------------ %d\n", client_operation!=CLIENT_OPERATION_UNSUBSCRIBE );
            if(client_operation!=CLIENT_OPERATION_UNSUBSCRIBE)
              on_update_msg( sd, (msg) );
          }
          else if( msg->type == SUBSCRIBE_ACK ) {
            log_debug( l, "subscribe ack\n" );
            on_subscription_ack_sids( ((struct PRTP_packet *)msg)->data.blob );
            aux_tr.frag_buffer = get_frag_buffer(NULL);
          }
          else {
            log_print(l, "No handler for message.\n");
          }
          free_iotmsg( msg );
        }
      }
    }
    /* Timeout, need to send KEEP_ALIVE. If client unsubscribe it will not send any keep_alive message */
    if( (ret == 0 || timeout_msecs == 0)&&(client_operation!=CLIENT_OPERATION_UNSUBSCRIBE))
    {
      log_debug(l, "Timeout reached, sending keep_alive.\n");
      send_keep_alive(sd);
      update_pending_subscriptions(sd);
      timeout_msecs = KEEP_ALIVE_INTERVAL;
    }
  } 
}

void shutdown_client()
{
  shutdown_logger(l);
  shutdown_bson_parser();
  shutdown_sensor_logger();
  shutdown_transport();

  shutdown_client_module();
}

void signal_handler(int signum)
{
  log_print(l, "Shutdown.\n");
  shutdown_client();
  exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
  char server_host[HOSTNAME_SIZE] = "localhost";
  char log_dir[HOSTNAME_SIZE] = "./client_sensor_log";
  char *localaddr = NULL;
  char *endptr;
  int server_port = 5001;
  int c;
  int sd;
  struct PRTP_packet* submsg = NULL;

  if( signal(SIGINT, signal_handler) == SIG_IGN )
    signal(SIGINT, SIG_IGN);
  
  while ((c = getopt(argc, argv, "hl:s:p:qaAr:gNH:")) != -1) {
    switch(c) {
      case 'l':
        if (strlen(optarg) < HOSTNAME_SIZE) {
          strcpy(log_dir, optarg);
        } else {
          log_error(l, "Error: log directory name too long\n");
        }
        break;

      case 's':
        if (strlen(optarg) < HOSTNAME_SIZE) {
          strcpy(server_host, optarg);
        } else {
          log_error(l, "Error: hostname too long\n");
          exit(EXIT_FAILURE);
        }
        break;

      case 'p':
        errno = 0;
        server_port = strtol(optarg, &endptr, 10);
        if ( server_port < 1 || server_port > 65535 ) {
          log_error(l, "Error: invalid port number (should be between 1...65535)\n");
          exit(EXIT_FAILURE);
        }
        break;

      case 'q':
        operation = CLIENT_OPERATION_QUERY_SENSOR_LIST;
        break;

      case 'a':
        operation = CLIENT_OPERATION_SUBSCRIBE_ALL_SENSORS;
        break;

      case 'A':
        operation = CLIENT_OPERATION_SUBSCRIBE_ALL_SENSORS;
        all_reliable = true;
        break;

      case 'g':
        ui_enabled = true;
        break;

      case 'N':
        active_flow_enabled = true;
        break;
      case 'H':
        localaddr = (char*)xalloc(strlen(optarg) + 1);
        strcpy(localaddr, optarg);
        break;
      case 'r':
        if( !submsg ) submsg = create_iotmsg(SUBSCRIBE);
        iotmsg_add_sid( submsg, optarg );
        iotmsg_set_sid_reliable( submsg, true );
        break;
      case 'h':
      default:
        usage();
        exit(EXIT_SUCCESS);
    }
  }

  l = init_logger(stdout, stderr, stderr, "Client");
  init_bson_parser();
  init_client_module();
  init_transport();
  priotps_security_ctx_t *sctx = get_priotps_security_ctx();
  if (sctx) sctx->is_server = 0;
  init_sensor_logger(log_dir);

  printf("connceting to %s\n", server_host);

  if (localaddr != NULL) {
    sd = init_socket(server_host, server_port, false, localaddr);
  } else {
    sd = init_socket(server_host, server_port, false, NULL);
  }
  if( sd == -1 ) {
    log_error(l, "Socket init failed\n");
    exit(EXIT_FAILURE);
  }

  // Send PRIoTPS Handshake Request
  uint8_t hsk_pkt[1024];
  size_t hsk_len = sizeof(hsk_pkt);
  fprintf(stderr, "PRIoTPS: Checking if we should send Handshake Request... sctx=%p, initialized=%d\n", sctx, sctx ? sctx->initialized : -1);
  if (sctx && sctx->initialized) {
      if (security_core_client_build_subscribe(sctx, hsk_pkt, &hsk_len) == 0) {
          int ret = send(sd, hsk_pkt, hsk_len, 0);
          if (ret < 0) {
              perror("send() Handshake Request failed");
          } else {
              fprintf(stderr, "PRIoTPS: Handshake Request sent. len=%zu, ret=%d\n", hsk_len, ret);
          }
      } else {
          fprintf(stderr, "PRIoTPS: build_subscribe failed!\n");
      }
  }
  
  if( (operation == CLIENT_OPERATION_QUERY_SENSOR_LIST) || (operation == CLIENT_OPERATION_SUBSCRIBE_ALL_SENSORS) ) {
    query_sensor_list(sd);
  } else if (optind < argc) { /* List of sensor IDs given as cmd line parameters */
    uint32_t i;
    /* Might be already created with reliable sids */
    if( !submsg) submsg = create_iotmsg(SUBSCRIBE);
    for (i=optind; i < argc; i++) {
      iotmsg_add_sid(submsg, argv[i]);
    }
  }

  if( submsg ) {
    add_pending_subscription((struct PRTP_packet *) submsg);
    send_iotmsg(sd, submsg);
  }

  if( operation == -1 && !submsg ) {
    usage();
    exit(EXIT_SUCCESS);
  }
  main_loop(sd);

  shutdown_client();
  return EXIT_SUCCESS;
}