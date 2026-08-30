#include "clients_config.h"
#include "utils.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct clients_config_node* clients_config = NULL;

static struct logger* l = NULL;

void init_clients_config()
{
  l = init_logger(stdout, stderr, stderr, "Clients config");
}

int read_clients_config(char* filename)
{
  FILE* config_file = NULL;
  char str[100];
  char* ch;
  struct clients_config_node *new_config;
  log_error(l, filename);

  config_file = fopen(filename, "r");
  if( !config_file )
  {
    log_error(l, "Couldn't read clients configuration file.\n");
    return -1;
  }

  while( !feof( config_file ) ) {
    ch = fgets(str, 100, config_file);
    if( !ch ) break;
    ch = strchr( str, '\n' );
    if( ch ) *ch = '\0'; 
    if( str[0] == '#' ) continue;

    new_config = xalloc(sizeof(struct clients_config_node));
    sscanf(str, "%d %f %f", &(new_config->sched.delay_msecs),
           &(new_config->sched.p_prob),&(new_config->sched.q_prob));

    new_config->next = clients_config;
    clients_config = new_config;
    log_debug(l, "Added a new clients configuration.\n");
  }

  return 0;
}

void shutdown_clients_config()
{
  struct clients_config_node *config = clients_config, *tmp = NULL;

  while( config != NULL )
  {
    tmp = config->next;
    free(config);
    config = tmp;
  }
}

int get_scheduler(struct scheduler* sched, struct client_node* client)
{
  struct clients_config_node* config;

  for( config = clients_config; config != NULL; config = config->next )
  {
    if( config->client == client ) {
      *sched = config->sched;
      return 0;
    }
  }
  return -1;
}

int assign_scheduler(struct client_node* client)
{
  struct clients_config_node* config;

  for( config = clients_config; config != NULL; config = config->next )
  {
    if( config->client == NULL ) {
      config->client = client;
      return 0;
    }
  }
  return -1;
}

int remove_client_config(struct client_node* client)
{
  struct clients_config_node* config;

  for( config = clients_config; config != NULL; config = config->next )
  {
    if( config->client == client ) {
      config->client = NULL;
      return 0;
    }
  }
  return -1;
}
