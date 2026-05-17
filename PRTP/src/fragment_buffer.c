#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "messages.h"
#include "fragment_buffer.h"
#include "utils.h"

void create_fragment_buffer(char* sid, struct fragment_buffer** first)
{
  struct fragment_buffer* last;
  last = *first;
  if (last != NULL) {
    while (last->next != NULL) {
      last = last->next;
    }
    last->next = (struct fragment_buffer*)xalloc(sizeof(struct fragment_buffer));
    last->next->sid = strdup(sid);
    last->clean = false;
  } else {
    *first = (struct fragment_buffer*)xalloc(sizeof(struct fragment_buffer));
    (*first)->sid = strdup(sid);
    (*first)->clean = false;
  }
}

void free_fragment_buffers(struct fragment_buffer* first)
{
  struct fragment_buffer* next = NULL;
  struct fragment_buffer* cur = first;
  while (cur != NULL) {
    free(cur->sid);
    free_fragment_list(cur->first_fragment);
    next = cur->next;
    free(cur);
    cur = next;
  }
}

// enum MESSAGE_STATUS update_fragment_buffer(struct fragment_buffer* buffer, struct PRTP_packet* msg)
// {
//   assert(msg != NULL);
//   assert(buffer != NULL);
//   assert(strcmp(buffer->sid, msg->data.sid) == 0);

//   if (msg->seq_no > buffer->seq_no) {
//     /* Discard current, start fresh buffer with new seq_no*/
//     free_fragment_list(buffer->first_fragment);
//     buffer->first_fragment = NULL;
//     buffer->seq_no = msg->seq_no;
//     buffer->frag_received = 0;
//     create_fragment_list(buffer, msg->frag_total);
//   } else if (msg->seq_no < buffer->seq_no) {
//     /* Discard msg */
//     return MESSAGE_STATUS_OLD;
//   }

//   if ( buffer ) {
//     enum MESSAGE_STATUS status;
//     // enum FRAGMENT_STATUS frag_status;
//     if (buffer->first_fragment == NULL) {
//         create_fragment_list(buffer, msg->frag_total);
//     }
//     // frag_status = add_fragment_to_buffer(buffer, &(msg->data), msg->frag_no);
//     status = check_buffer(buffer);
//     return status;
//   } else {
//     return MESSAGE_STATUS_ERROR;
//   }
// }
enum MESSAGE_STATUS update_fragment_buffer(struct fragment_buffer* buffer, struct PRTP_packet* msg)
{
  assert(msg != NULL);
  assert(buffer != NULL);
  /* Fix: Access sid through union for UPDATE message */
  assert(strcmp(buffer->sid, msg->data.update.sid) == 0);

  if (msg->seq_no > buffer->seq_no) {
    /* Discard current, start fresh buffer with new seq_no*/
    free_fragment_list(buffer->first_fragment);
    buffer->first_fragment = NULL;
    buffer->seq_no = msg->seq_no;
    buffer->frag_received = 0;
    create_fragment_list(buffer, msg->frag_total);
  } else if (msg->seq_no < buffer->seq_no) {
    /* Discard msg */
    return MESSAGE_STATUS_OLD;
  }

  if ( buffer ) {
    enum MESSAGE_STATUS status;
    if (buffer->first_fragment == NULL) {
        create_fragment_list(buffer, msg->frag_total);
    }
    /* Fix: Pass the blob part of the union */
    add_fragment_to_buffer(buffer, &(msg->data.update.blob), msg->frag_no);
    status = check_buffer(buffer);
    return status;
  } else {
    return MESSAGE_STATUS_ERROR;
  }
}
void create_fragment_list(struct fragment_buffer* buffer, uint32_t size) 
{
  uint32_t i;
  struct fragment_buffer_node* prev = NULL;
  struct fragment_buffer_node* tmp = NULL;

  assert(buffer != NULL);
  assert(buffer->first_fragment == NULL);
  buffer->frag_total = size;
  for (i=0; i < size; i++) {
    tmp = (struct fragment_buffer_node*)xalloc(sizeof(struct fragment_buffer_node));
    tmp->frag_no = i;
    if (prev == NULL) {
      buffer->first_fragment = prev = tmp;
    } else {
      prev->next = tmp;
      prev = tmp;
    }
  }
  tmp = buffer->first_fragment;
  while(tmp != NULL) {
    tmp = tmp->next;
  }
  buffer->clean = true;
}

struct fragment_buffer* get_fragment_buffer(struct fragment_buffer* buf, char* sid)
{
  struct fragment_buffer* node = buf;
  while (node != NULL) {
    if (strcmp(node->sid, sid) == 0) {
      return node;
    }
    node = node->next;
  }
  return NULL;
}

enum FRAGMENT_STATUS add_fragment_to_buffer(struct fragment_buffer* buffer, struct void_data* data, uint32_t frag_no)
{
  struct fragment_buffer_node* node = buffer->first_fragment;
  uint32_t i = 0;
  uint32_t data_length = data->len;
  assert(buffer->clean == true);
  for (i=0; i < frag_no; i++) {
    node = node->next;
  }
  assert(node != NULL);
  if (node->data.blob != NULL) {
    return FRAGMENT_STATUS_DUP;
  }
  buffer->frag_received = buffer->frag_received + 1;
  node->data.blob = (void*)xalloc(data_length);
  node->status = FRAGMENT_STATUS_OK;
  node->data.len = data_length;
  memcpy(node->data.blob, data->blob, data_length);
  return FRAGMENT_STATUS_OK;
}

enum MESSAGE_STATUS check_buffer(struct fragment_buffer* buffer) 
{
  struct fragment_buffer_node* node;
  if (buffer->frag_received == buffer->frag_total) {
    node = buffer->first_fragment;
    
    while(node != NULL) {
      assert(node->status == FRAGMENT_STATUS_OK);
      node = node->next;
    }
    return MESSAGE_STATUS_READY;
  } else if (buffer->frag_received < buffer->frag_total) {
    return MESSAGE_STATUS_MISSING;
  } else {
    assert(false);
  }
}

void free_fragment_list(struct fragment_buffer_node* first) 
{
  struct fragment_buffer_node* node = first;
  struct fragment_buffer_node* temp = NULL;
  while( node != NULL ) {
    free(node->data.blob);
    temp = node;
    
    node = node->next;
    
    free(temp);
  }
}

uint32_t frag_buf_data_length(struct fragment_buffer* buf) 
{
    struct fragment_buffer_node* node = buf->first_fragment;
    uint32_t total_bytes = 0;
    while(node != NULL) {
      if (node->status == FRAGMENT_STATUS_OK)
        total_bytes = total_bytes + node->data.len;
      node = node->next;
    }
    return total_bytes;
}

// int fragment_update_message(struct fragment_buffer* buf, struct PRTP_packet* upd_msg, uint32_t max_data)
// {
//   uint32_t data_length_total = upd_msg->data.len;
//   uint32_t frag_no = 0;
//   uint32_t frag_total;
//   uint32_t data_offset = 0;
//   struct void_data frag_data = {0, NULL};

//   max_data = max_data ? max_data : MAX_SENSOR_DATA_PER_DGRAM;
//   upd_msg->frag_total = frag_total = data_length_total / max_data + 1;

//   if( frag_total == 1 ) return 1; /* Shortcut to avoid unnecessary copying */

//   create_fragment_list( buf, frag_total );

//   while (data_offset < data_length_total) {
//     printf("data_offset / data_length_total: (%d/%d) frag_no / frag_total (%d/%d)\n", data_offset, data_length_total, frag_no, frag_total);
//     frag_data.len = (data_offset + max_data < data_length_total) ?
//                      max_data : data_length_total - data_offset;
//     frag_data.blob = (char*)upd_msg->data.blob + data_offset;
      
//     add_fragment_to_buffer( buf, &frag_data, frag_no );
//     data_offset += frag_data.len;
//     frag_no++;
//   }
//   return frag_total;
// }
int fragment_update_message(struct fragment_buffer* buf, struct PRTP_packet* upd_msg, uint32_t max_data)
{
  /* Fix: Access through union for UPDATE message */
  uint32_t data_length_total = upd_msg->data.update.blob.len;
  uint32_t frag_no = 0;
  uint32_t frag_total;
  uint32_t data_offset = 0;
  struct void_data frag_data = {0, NULL};

  max_data = max_data ? max_data : MAX_SENSOR_DATA_PER_DGRAM;
  upd_msg->frag_total = frag_total = data_length_total / max_data + 1;

  if( frag_total == 1 ) return 1; /* Shortcut to avoid unnecessary copying */

  create_fragment_list( buf, frag_total );

  while (data_offset < data_length_total) {
    printf("data_offset / data_length_total: (%d/%d) frag_no / frag_total (%d/%d)\n", 
           data_offset, data_length_total, frag_no, frag_total);
    frag_data.len = (data_offset + max_data < data_length_total) ?
                     max_data : data_length_total - data_offset;
    /* Fix: Access blob through union */
    frag_data.blob = (char*)upd_msg->data.update.blob.blob + data_offset;
      
    add_fragment_to_buffer( buf, &frag_data, frag_no );
    data_offset += frag_data.len;
    frag_no++;
  }
  return frag_total;
}

int generate_update_messages(struct fragment_buffer* buf, struct PRTP_packet* upd_msg)
{
  int ret = -1;
  struct fragment_buffer_node* node = buf->first_fragment;
  
  if( (upd_msg->frag_total == 1) && (upd_msg->frag_no == 0) ) {
    return 0;
  }

  while( node != NULL ) {
    if( node->frag_no == upd_msg->frag_no ) break;
    node = node->next;
  }
  if( node != NULL )
    ret = iotmsg_set_data(upd_msg, &(node->data) );

  return ret;
}
