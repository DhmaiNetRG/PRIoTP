/**
 * @file fragment_buffer.h
 * @brief Packet fragmentation and reassembly buffer.
 * 
 * Manages reception and reassembly of fragmented PRIoTP messages across
 * multiple packets, handling out-of-order and duplicate fragments.
 * 
 * @note Fragment buffer copies payload data for clarity; optimization possible.
 * 
 * @author Hämäläinen, Harri (harri.hamalainen@aalto.fi)
 * @author Kurnikov, Arseny (arseny.kurnikov@aalto.fi)
 * @author Rahman Mehraj (rahmanmehraj627@gmail.com)
 * @author Hasan Mahmood (hasan.mahmood@ewubd.edu)
 */

#ifndef FRAGMENT_BUFFER_G7EES5IC

#define FRAGMENT_BUFFER_G7EES5IC

#include "messages.h"

/**
 * @enum MESSAGE_STATUS
 * @brief Reassembly status of fragmented message.
 */
enum MESSAGE_STATUS {
    MESSAGE_STATUS_READY,    /**< All fragments received, message complete */
    MESSAGE_STATUS_MISSING,  /**< Waiting for additional fragments */
    MESSAGE_STATUS_OLD,      /**< Obsolete message (timeout) */
    MESSAGE_STATUS_ERROR     /**< Reassembly error */
};

/**
 * @enum FRAGMENT_STATUS
 * @brief Status of individual fragment reception.
 */
enum FRAGMENT_STATUS {
    FRAGMENT_STATUS_MISSING,  /**< Fragment not yet received */
    FRAGMENT_STATUS_OK,       /**< Fragment received and buffered */
    FRAGMENT_STATUS_DUP       /**< Duplicate fragment detected */
};

/**
 * @struct fragment_buffer
 * @brief Reassembly buffer for fragmented message.
 */
struct fragment_buffer {
    bool clean;                      /**< Valid buffer state */
    uint32_t seq_no;                 /**< Message sequence number */
    uint32_t frag_received;          /**< Fragments received count */
    uint32_t frag_total;             /**< Total fragments expected */
    struct fragment_buffer_node* first_fragment; /**< Fragment list head */
    char* sid;                       /**< Sensor identifier */
    struct fragment_buffer* next;    /**< Linked list pointer */
};

/**
 * @struct fragment_buffer_node
 * @brief Individual fragment buffer node.
 */
struct fragment_buffer_node {
    enum FRAGMENT_STATUS status;     /**< Reception status */
    uint32_t frag_no;                /**< Fragment number */
    struct void_data data;           /**< Fragment payload */
    struct fragment_buffer_node* next; /**< Linked list pointer */
};

/**
 * @brief Create new fragment buffer for sensor.
 * @param sid Sensor identifier
 * @param first Pointer to buffer list head
 */
void create_fragment_buffer(char* sid, struct fragment_buffer** first);

/**
 * @brief Allocate fragment slots in buffer.
 * @param buffer Target fragment buffer
 * @param size Number of fragments to allocate
 */
void create_fragment_list(struct fragment_buffer* buffer, uint32_t size);

/**
 * @brief Retrieve buffer by sensor identifier.
 * @param buf Buffer list head
 * @param sid Sensor identifier to search
 * @return Pointer to matching buffer, NULL if not found
 */
struct fragment_buffer* get_fragment_buffer(struct fragment_buffer* buf, char* sid);

/**
 * @brief Insert fragment into reassembly buffer.
 * @param buffer Target fragment buffer
 * @param data Fragment payload
 * @param frag_no Fragment number
 * @return Fragment reception status
 */
enum FRAGMENT_STATUS add_fragment_to_buffer(struct fragment_buffer* buffer, struct void_data* data, uint32_t frag_no);

/**
 * @brief Check reassembly completion status.
 * @param buffer Fragment buffer to check
 * @return Message status (ready, missing, error)
 */
enum MESSAGE_STATUS check_buffer(struct fragment_buffer* buffer);

/**
 * @brief Process received fragment and update buffer state.
 * @param buffer Fragment buffer
 * @param msg Received PRIoTP packet containing fragment
 * @return Message status (ready, missing, error)
 */
enum MESSAGE_STATUS update_fragment_buffer(struct fragment_buffer* buffer, struct PRTP_packet* msg);

/**
 * @brief Free fragment node list.
 * @param first Fragment node list head
 */
void free_fragment_list(struct fragment_buffer_node* first);

/**
 * @brief Get total data length in buffer.
 * @param buf Fragment buffer
 * @return Total payload bytes
 */
uint32_t frag_buf_data_length(struct fragment_buffer* buf);

/**
 * @brief Divide sensor data into fragments for transmission.
 * @param buf Fragment buffer
 * @param msg PRIoTP packet template
 * @param max_data Maximum data bytes per fragment
 * @return Number of fragments created
 */
int fragment_update_message(struct fragment_buffer* buf, struct PRTP_packet* msg, uint32_t max_data);

/**
 * @brief Generate next fragment PRIoTP packet.
 * @param buf Fragment buffer
 * @param msg PRIoTP packet to populate
 * @return Bytes in next fragment, 0 if complete
 */
int generate_update_messages(struct fragment_buffer* buf, struct PRTP_packet* msg);

/**
 * @brief Free all fragment buffers.
 * @param first Buffer list head
 */
void free_fragment_buffers(struct fragment_buffer* first);

#endif /* end of include guard: FRAGMENT_BUFFER_G7EES5IC */
