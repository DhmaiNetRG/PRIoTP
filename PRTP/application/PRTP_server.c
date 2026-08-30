#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <signal.h>

#include "../src/subscriptions.h"
#include "../src/bson_parser.h"
#include "../src/sensor_parser.h"
#include "../src/logger.h"
#include "../src/utils.h"
#include "../src/sensor_logger.h"
#include "../src/clients_config.h"
#include "../src/q_agent.h"
#include "../src/telemetry.h"

#define HOSTNAME_SIZE 255
#define FILENAME_SIZE 255

q_agent_t server_q_agent;
bool q_learning_enabled = false;
static struct logger* l = NULL;
static bool nack_support = true;

void usage()
{
  printf("\n Usage: iotserver -i <ip address> -p <sensor publish port> -s <client subscribe port> -l <sensor list> -c <clients config> -q <q_table_path> -t <telemetry_path>\n\n");
}

int parse_parameters(int argc, char *argv[],
                     char* hostname, in_port_t* sensor_port, in_port_t* client_port,
                     char* sensor_list, char* clients_config, char* q_table_path,
                     char* telemetry_path)
{
  int c = 0;

  while((c = getopt(argc,argv,"hi:p:s:l:c:q:t:")) != -1){
    switch(c){
    case 'i':
      if( strlen(optarg) < HOSTNAME_SIZE )
        strcpy(hostname, optarg);
      else
      {
        log_error(l, "Error: given hostname/ip_address is too long.\n");
        return -1;
      }
      break;
    case 'p':
      if( (*sensor_port=atoi(optarg)) == 0 )
      {
        log_error(l, "Error: invalid sensor publish port. Give a number between 1 and 65535.\n");
        return -1;
      }
      break;
    case 's':
      if( (*client_port=atoi(optarg)) == 0 )
      {
        log_error(l, "Error: invalid client subscribe port. Give a number between 1 and 65535.\n");
        return -1;
      }
      break;

    case 'l':
      if( strlen(optarg) < FILENAME_SIZE )
        strcpy(sensor_list, optarg);
      else
      {
        log_error(l, "Error: given sensor list filename is too long.\n");
        return -1;
      }
      break;

    case 'c':
      if( strlen(optarg) < FILENAME_SIZE )
        strcpy(clients_config, optarg);
      else
      {
        log_error(l, "Error: given clients config filename is too long.\n");
        return -1;
      }
      break;

    case 'q':
      if( strlen(optarg) < FILENAME_SIZE )
        strcpy(q_table_path, optarg);
      else
      {
        log_error(l, "Error: given Q-table path is too long.\n");
        return -1;
      }
      break;

    case 't':
      if( strlen(optarg) < FILENAME_SIZE )
        strcpy(telemetry_path, optarg);
      else
      {
        log_error(l, "Error: given telemetry path is too long.\n");
        return -1;
      }
      break;

    case 'h':
    default:
      usage();
      return -1;
      break;
    }
  }

  return 0;
}

int on_client_msg(struct client_node* node, const struct PRTP_packet *msg, struct PRTP_packet** response, int len)
{
  struct iotmsg_node* sid;
  struct sensor_node* sensor;
  struct PRTP_packet* sack_msg;
  struct PRTP_packet* sub_msg;
  struct PRTP_packet* list_resp_msg;
  *response = NULL;

  if( msg->type == LIST ) {
    list_resp_msg = create_iotmsg(LIST_RESPONSE);
    write_sensor_list( list_resp_msg );
    *response = list_resp_msg;
    return 0;
  }

  if( msg->type == SUBSCRIBE ) {
    sub_msg = msg;
    sack_msg = create_iotmsg(SUBSCRIBE_ACK);
    for( sid = sub_msg->data.blob; sid != NULL;
         sid = sid->next ) {
      iotmsg_add_sid( sack_msg, sid->id );
      sensor = get_sensor( sid->id );
      if( sensor != NULL ) {
        if( !subscription_exists( node, sensor ) ) {
          add_subscription( node, sensor, ((struct iotmsg_subscribe_node*)sid)->reliable );
          iotmsg_set_status( sack_msg, SUBSCRIBE_OK );
        }
        else
          iotmsg_set_status( sack_msg, SUBSCRIBE_ALREADY_EXISTS );
      }
      else {
        iotmsg_set_status( sack_msg, SUBSCRIBE_NOT_FOUND );
      }
    }
    *response = sack_msg;
    return 0;
  }

  if( msg->type == KEEP_ALIVE ) {
    log_debug(l, "Got a keep-alive from client.\n");
    gettimeofday(&node->last_seen, 0);
    return 0;
  }

  if( msg->type == UPDATE_ACK ) {
    log_debug(l, "Got update acknowledgement from client.\n");
    sensor = get_sensor( (msg)->data.sid );
    if( sensor ) subscription_on_ack(node, sensor, (msg)->seq_no, len);
    return 0;
  }

  if ( msg->type == UPDATE_NACK && nack_support ) {
    log_debug(l, "Got negative update acknowledgement from client.\n");
    sensor = get_sensor( (msg)->data.sid );
    if ( sensor ) subscription_on_nack(node, sensor);
    return 0;
  }

  return -1;
}

int server_loop(int sensor_sd, int client_sd)
{
  struct pollfd fds[2];
  int timeout_msecs = RETRANSMIT_TIMEOUT;
  int ret = 0, res;
  struct PRTP_packet* recv_msg;
  struct PRTP_packet* send_msg;
  struct client_node* client = NULL;
  struct sensor_node* sensor = NULL;

  fds[0].fd = sensor_sd;
  fds[1].fd = client_sd;
  fds[0].events = POLLIN;
  fds[1].events = POLLIN;

  for(;;)
  {
    ret = poll( fds, 2, timeout_msecs );
    if( ret > 0 ) {
      if( fds[0].revents & POLLIN ) {
        if( (res = read_sensor(sensor_sd, &sensor)) != -1 ) {
          mark_updated_subscriptions(sensor);
          if( res == 1 ) {
            /* Loggers are now added in sensors.c for consistency */
          }

          sensor_log(sensor->id, sensor->seq_no, sensor->type, &(sensor->data));
          if( sensor->type == CAMERA ) {
            if( strncmp((char*)(sensor->data.blob), "NO_MOTION", 9 ) != 0 )
              sensor_dump(sensor->id, &(sensor->data));
          }
        }
      }
      if( fds[1].revents & POLLIN ) {
        recv_msg = NULL;
        if( (res = read_client(client_sd, &client, &recv_msg)) != -1 ) {
          send_msg = NULL;
          if( on_client_msg(client, recv_msg, &send_msg, res) == -1 )
            log_error(l, "Error constructing a response for the client.\n");
          else if( send_msg != NULL ) {
            send_client_message( NULL, send_msg, client );
            free_iotmsg(send_msg);
          }
          free_iotmsg(recv_msg);
        }
      }
    }
    if( ret == 0 ) {
      /* prune_expired_clients(); */
    }
    prune_expired_clients();
    send_updates(&timeout_msecs);

    if( ret == -1 ) {
      log_error(l, "Poll errors.\n");
      exit(EXIT_FAILURE);
    }
  }
}

void init_systems()
{
  l = init_logger(stdout, stderr, stderr, "Server");
  init_clients();
  init_transport();
  init_subscriptions();
  init_messages();
  init_sensor_types();
  init_sensors();
  init_bson_parser();
  init_sensor_parser();
  init_sensor_logger("../PRTP/src/server_sensor_log");
}

void shutdown_systems()
{
  shutdown_logger(l);
  shutdown_clients();
  shutdown_transport();
  shutdown_subscriptions();
  shutdown_messages();
  shutdown_sensor_types();
  shutdown_sensors();
  shutdown_bson_parser();
  shutdown_sensor_parser();
  shutdown_sensor_logger();
  telemetry_shutdown();
}

void signal_handler( int signum )
{
  log_print(l, "Shutdown.\n");
  shutdown_systems();
  exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
  char hostname[HOSTNAME_SIZE] = "localhost";
  char sensor_list[FILENAME_SIZE] = "./sensor.list";
  char clients_config[FILENAME_SIZE] = "clients.conf";
  char q_table_path[FILENAME_SIZE] = "";  // Empty by default
  char telemetry_path[FILENAME_SIZE] = "";
  in_port_t sensor_port = 5000, client_port = 5001;
  int sensor_sd, client_sd;
  
  init_systems();
  
  if( signal(SIGINT, signal_handler) == SIG_IGN )
    signal(SIGINT, SIG_IGN);

  if( parse_parameters(argc, argv, hostname, &sensor_port, &client_port,
                       sensor_list, clients_config, q_table_path, telemetry_path) < 0 ) 
    exit(EXIT_FAILURE);

  telemetry_init(strlen(telemetry_path) > 0 ? telemetry_path : NULL);

  log_print(l, "Will bind to: %s.\n", hostname);
  log_print(l, "Port for sensor data: %u.\n", sensor_port);
  log_print(l, "Port for client subscriptions: %u.\n", client_port);

  /* Initialize Q-learning agent */
  q_learning_enabled = true;
  q_agent_init(&server_q_agent, 0.2, 0.9, 0.1);
  
  /* Try multiple paths for Q-table */
  const char* q_table_paths[] = {
    q_table_path,  // Command line argument (if provided)
    "./q_agent_trained.csv",
    "../launcher/q_agent_trained.csv",
    "../PRTP/application/q_agent_trained.csv",
    "q_agent_trained.csv",
    NULL
  };
  
  int q_table_loaded = 0;
  for (int i = 0; q_table_paths[i] != NULL && !q_table_loaded; i++) {
    if (strlen(q_table_paths[i]) == 0) continue;  // Skip empty paths
    
    if (q_agent_load(&server_q_agent, q_table_paths[i]) == 0) {
      log_print(l, "✓ Q-table loaded successfully from %s\n", q_table_paths[i]);
      q_agent_print_table(&server_q_agent, stdout);
      q_table_loaded = 1;
      break;
    }
  }
  
  if (!q_table_loaded) {
    log_error(l, "✗ Failed to load Q-table from any location. Tried:\n");
    for (int i = 0; q_table_paths[i] != NULL; i++) {
      if (strlen(q_table_paths[i]) > 0)
        log_error(l, "  - %s\n", q_table_paths[i]);
    }
    log_error(l, "Q-learning will use untrained (zero) values!\n");
    log_error(l, "To train: compile and run q_agent_test.c, then copy q_agent_trained.csv\n");
  }
  
  log_print(l, "Q-learning enabled: %s\n", q_learning_enabled ? "YES" : "NO");

  populate_sensor_list(sensor_list);
  read_clients_config(clients_config);

  sensor_sd = sensor_socket(hostname, sensor_port);
  if( sensor_sd == -1 ) exit(EXIT_FAILURE);

  client_sd = client_socket(hostname, client_port);
  if( client_sd == -1 ) exit(EXIT_FAILURE);

  server_loop(sensor_sd, client_sd);

  shutdown_systems();
  return EXIT_SUCCESS;
}