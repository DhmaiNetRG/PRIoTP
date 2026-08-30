#include "validator.h"
#include <stdlib.h>
#include <string.h>

static const struct validate_description message_start = {
  NULL,
  NULL,
  BSON_UNKNOWN__,
  NULL
};

static const struct validate_description list_validator[1] = {
  { NULL,
    "type",
    BSON_INT32,
    &message_start
  }
};

static const struct validate_description* validators[] = {
  &list_validator[0]
};

void copy_validator(struct validate_node** context, const struct validate_description* validator)
{
  /*struct validate_node* node;*/

}

int validate_bson_iotmsg(const char* parent, const char* key, enum BSON_TYPE type,
                         struct validate_node** context)
{
  struct validate_node* node;
  /*const struct validate_description *desc;*/
  int i, j;

  /* Here comes the first field of the message, need to select the validator
   * The selection is based on the value*/
  if( *context == NULL ) {
    for(i = 0; i < sizeof(validators); i++)
    {
      for( j = 0; j < sizeof(validators[i]); j++ )
      {
/*        desc = validators[i][j];
        if( desc->prev_required == &message_start ) {
          if (is_description_matching(desc, parent, key, type) == 0) {
            copy_validator(context, validators[i]);
            return 0;
          }
        } */
      }
    }


  }
  for(node = *context; node != NULL; node = node->next)
  {
    
  }
  return -1;
}

int is_description_matching(const struct validate_description* description, const char* parent,
                            const char* key, enum BSON_TYPE type, void* value)
{
  if( description->type != type ) return -1;
  if( description->parent ) {
    if( parent ) {
      if( strcmp( description->parent, parent ) != 0 ) return -1;
    }
    else return -1;
  }
  else {
    if( parent ) return -1;
  }

  if( description->key ) {
    if( key ) {
      if( strcmp( description->key, key ) != 0 ) return -1;
    }
    else return -1;
  }
  else {
    if( key ) return -1;
  }

/*  if( description->value ) {
    if( value ) { */
      /* So far only check INT32 values */
/*      if( type == BSON_INT32 ) {
        if( *(int*)value != *((int*)description->value) ) return -1;
      }
      else return -1;
    }
    else return -1;
  }
  else {
    if( value ) return -1;
  }
*/
  return 0;
}
