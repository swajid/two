/*
   This API contains the HTTP methods to be used by
   HTTP/2
 */
#ifndef HTTP_BRIDGE_H
#define HTTP_BRIDGE_H

#include <stdlib.h>
#include <stdint.h>
#include "sock.h"
#include "table.h"

#define HTTP2_MAX_HEADER_COUNT 32
#define HTTP2_MAX_HBF_BUFFER 128

#define HTTP_MAX_CALLBACK_LIST_ENTRY 32

/*-----HTTP2 structures-----*/

typedef enum {
    STREAM_IDLE,
    STREAM_OPEN,
    STREAM_HALF_CLOSED_REMOTE,
    STREAM_HALF_CLOSED_LOCAL,
    STREAM_CLOSED
} h2_stream_state_t;

typedef struct HTTP2_STREAM {
    uint32_t stream_id;
    h2_stream_state_t state;
} h2_stream_t;

/*Struct for storing HTTP2 states*/
typedef struct HTTP2_STATES {
    uint32_t remote_settings[6];
    uint32_t local_settings[6];
    /*uint32_t local_cache[6]; Could be implemented*/
    uint8_t wait_setting_ack;
    h2_stream_t current_stream;
    uint8_t header_block_fragments[HTTP2_MAX_HBF_BUFFER];
    uint8_t header_block_fragments_pointer; //points to the next byte to write in
    uint8_t waiting_for_end_headers_flag;   //bool
    uint8_t received_end_stream;
} h2states_t;

/*-----HTTP structures-----*/

typedef struct HTTP_STATES {
    uint8_t connection_state;
    uint8_t socket_state;
    sock_t *socket; // client socket
    uint8_t server_socket_state;
    sock_t *server_socket;
    h2states_t h2s;
    uint8_t header_list_count;
    table_pair_t header_list[HTTP2_MAX_HEADER_COUNT];
    uint8_t path_callback_list_count;
    table_pair_t path_callback_list[HTTP_MAX_CALLBACK_LIST_ENTRY];
    uint8_t new_headers; //boolean. Notifies HTTP if new headers were written
    uint8_t keep_receiving; //boolean. Tells HTTP to keep receiving frames

} hstates_t;




/*
 * Write in the socket with the client
 *
 * @param    hs    http states struct
 * @param    buf   buffer with the data to writte
 * @param    len   buffer length
 *
 * @return >0   number of bytes written
 * @return 0    if connection was closed on the other side
 * @return -1   on error
 */
int http_write(hstates_t *hs, uint8_t *buf, int len);

/*
 * Read the data from the socket with the client
 *
 * @param    hs    http states struct
 * @param    buf   buffer where the data will be write
 * @param    len   buffer length
 *
 * @return   >0    number of bytes read
 * @return   0     if connection was closed on the other side
 * @return   -1    on error
 */
int http_read(hstates_t *hs, uint8_t *buf, int len);

/*
 * Given the content of the request made by the client, this function calls
 * the functions necessary to respond to the request
 *
 * @param     hs    struct with headers information
 *
 * @return    0     the action was successful
 * @return    -1    the action fail
 */
int http_receive(hstates_t *hs);

/*
 * Empty the list of headers in hstates_t struct
 *
 * @param    hs        struct with headers information
 * @param    index     header index in the headers list to be maintained,
 *                     invalid index to delete the entire table
 *
 * @return    0        The list was emptied
 * @return    1        There was an error
 */
int http_clear_header_list(hstates_t *hs, int index);

#endif /* HTTP_BRIDGE_H */
