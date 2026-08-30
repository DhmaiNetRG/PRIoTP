#ifndef VALIDATOR_H
#define VALIDATOR_H

#include "bson_parser.h"

struct validate_description
{
  char* parent;
  char* key;
  enum BSON_TYPE type;
  const struct validate_description* prev_required;
};

struct validate_node
{
  struct validate_description desc;
  int occurence;
  struct validate_node* next;
};

#endif
