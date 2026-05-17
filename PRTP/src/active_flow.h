#ifndef ACTIVE_FLOW_ISGD573
#define ACTIVE_FLOW_ISGD573

#include <stdlib.h>
#include <stdint.h>

enum FLOW_STATUS {
  FLOW_STATUS_OK,
  FLOW_STATUS_RECOVERING
};

enum FLOW_STATUS_REPORT {
  FLOW_STATUS_REPORT_OK,
  FLOW_STATUS_REPORT_FRESH_MISS
};

struct active_flow
{
  struct active_flow* next;
  char* sid;
  uint32_t latest_seq_no;
  enum FLOW_STATUS status; 
};

void init_active_flow();
void create_active_flow(char* sid, struct active_flow** first);
struct active_flow* get_active_flow_from_list(struct active_flow* flow, char* sid);
enum FLOW_STATUS_REPORT update_active_flow(struct active_flow* flow, uint32_t new_seq_no);

#endif /* end of include guard: ACTIVE_FLOW_ISGD573 */
