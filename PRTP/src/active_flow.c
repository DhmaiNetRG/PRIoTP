#include <string.h>
#include "active_flow.h"
#include "logger.h"
#include "utils.h"

static struct logger* l = NULL;

void init_active_flow()
{
  l = init_logger(stdout, stderr, stderr, "Active Flow");
}

void create_active_flow(char* sid, struct active_flow** first)
{
  struct active_flow* last;
  last = *first;
  if (last != NULL) {
    while (last->next != NULL) {
      last = last->next;
    }
    last->next = (struct active_flow*)xalloc(sizeof(struct active_flow));
    last->next->sid = strdup(sid);
  } else {
    *first = (struct active_flow*)xalloc(sizeof(struct active_flow));
    (*first)->sid = strdup(sid);
  }
}

struct active_flow* get_active_flow_from_list(struct active_flow* flow, char* sid)
{
  struct active_flow* node = flow;
  while (node != NULL) {
    if (strcmp(node->sid, sid) == 0) {
      return node;
    }
    node = node->next;
  }
  return NULL;
}

enum FLOW_STATUS_REPORT update_active_flow(struct active_flow* flow, uint32_t new_seq_no)
{
  enum FLOW_STATUS_REPORT rval = FLOW_STATUS_REPORT_OK;
  if( (flow->latest_seq_no + 1) < new_seq_no) {
    if (flow->status == FLOW_STATUS_OK) {
      flow->status = FLOW_STATUS_RECOVERING;
      rval = FLOW_STATUS_REPORT_FRESH_MISS;
      log_debug(l, "detected missing message\n");
    } else {
      log_debug(l, "detected missing message, but already in recovery state\n");
    }
  } else {
    flow->status = FLOW_STATUS_OK;
  }
  flow->latest_seq_no = new_seq_no;
  return rval;
}
