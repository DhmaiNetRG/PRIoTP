/**
 * @file active_flow.h
 * @brief Active flow management for multi-source sensor aggregation.
 * 
 * Tracks active data flows by sensor identifier with sequence number
 * monitoring for detecting missing or out-of-order packets.
 * 
 * @author Hämäläinen, Harri (harri.hamalainen@aalto.fi)
 * @author Kurnikov, Arseny (arseny.kurnikov@aalto.fi)
 * @author Rahman Mehraj (rahmanmehraj627@gmail.com)
 * @author Hasan Mahmood (hasan.mahmood@ewubd.edu)
 */

#ifndef ACTIVE_FLOW_ISGD573
#define ACTIVE_FLOW_ISGD573

#include <stdlib.h>
#include <stdint.h>

/**
 * @enum FLOW_STATUS
 * @brief Operational status of active flow.
 */
enum FLOW_STATUS {
  FLOW_STATUS_OK,         /**< Flow operating normally */
  FLOW_STATUS_RECOVERING  /**< Flow in recovery from packet loss */
};

/**
 * @enum FLOW_STATUS_REPORT
 * @brief Diagnostic status for flow update.
 */
enum FLOW_STATUS_REPORT {
  FLOW_STATUS_REPORT_OK,           /**< Packet received in sequence */
  FLOW_STATUS_REPORT_FRESH_MISS    /**< Missing packet detected */
};

/**
 * @struct active_flow
 * @brief Active data flow state.
 * 
 * Maintains per-sensor flow context including latest received
 * sequence number and recovery status.
 */
struct active_flow
{
  struct active_flow* next;        /**< Linked list pointer */
  char* sid;                       /**< Sensor identifier */
  uint32_t latest_seq_no;          /**< Latest received sequence number */
  enum FLOW_STATUS status;         /**< Current flow status */
};

/**
 * @brief Initialize active flow registry.
 */
void init_active_flow();

/**
 * @brief Create new active flow for sensor.
 * @param sid Sensor identifier
 * @param first Pointer to flow list head
 */
void create_active_flow(char* sid, struct active_flow** first);

/**
 * @brief Retrieve active flow by sensor identifier.
 * @param flow Flow list head
 * @param sid Sensor identifier to search
 * @return Pointer to matching flow, NULL if not found
 */
struct active_flow* get_active_flow_from_list(struct active_flow* flow, char* sid);

/**
 * @brief Update flow with new sequence number.
 * @param flow Flow to update
 * @param new_seq_no Received sequence number
 * @return Status report (sequence continuity, missing packets)
 */
enum FLOW_STATUS_REPORT update_active_flow(struct active_flow* flow, uint32_t new_seq_no);

#endif /* end of include guard: ACTIVE_FLOW_ISGD573 */
