/*
   This API contains the HTTP methods to be used by
   HTTP/2
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include <unistd.h>

#include "http.h"
#include "http_bridge.h"
#include "logging.h"
#include "sock.h"
#include "http2.h"

int get_receive(hstates_t *hs);

/************************************Server************************************/


int http_init_server(hstates_t *hs, uint16_t port)
{
    hs->socket_state = 0;
    hs->h_lists.header_list_count = 0;
    hs->h_lists.header_list_count_response = 0;
    hs->path_callback_list_count=0;
    hs->connection_state = 0;
    hs->server_socket_state=0;

    if (sock_create(hs->server_socket) < 0) {
        ERROR("Error in server creation");
        return -1;
    }

    hs->server_socket_state=1;

    if (sock_listen(hs->server_socket, port) < 0) {
        ERROR("Partial error in server creation");
        if (sock_destroy(hs->server_socket)==0){
          hs->server_socket_state=0;
        }
        return -1;
    }

    printf("Server waiting for a client\n");


    while (sock_accept(hs->server_socket, hs->socket) >= 0) {

        printf("Client found and connected\n");

        hs->socket_state = 1;
        hs->connection_state = 1;

        if (h2_server_init_connection(hs) < 0) {
            hs->connection_state = 0;
            ERROR("Problems sending server data");
            return -1;
        }

        while (hs->connection_state == 1) {
          if (h2_receive_frame(hs) < 1) {
            break;
          }
          if (hs->keep_receiving == 1){
            continue;
          }
          if (hs->new_headers == 1){
            get_receive(hs);
          }
        }

        if (sock_destroy(hs->socket)==-1){
          WARN("Could not destroy client socket");
        }
        hs->connection_state=0;
        hs->socket_state=0;
    }

    ERROR("Not client found");

    return -1;
}


int http_server_destroy(hstates_t *hs)
{
    if (hs->server_socket_state == 0) {
        WARN("Server not found");
        return -1;
    }

    if (hs->socket_state == 1) {
        if (sock_destroy(hs->socket) < 0) {
            WARN("Client still connected");
        }
    }

    hs->socket_state = 0;
    hs->connection_state = 0;

    if (sock_destroy(hs->server_socket) < 0) {
        ERROR("Error in server disconnection");
        return -1;
    }

    hs->server_socket_state = 0;

    printf("Server destroyed\n");

    return 0;
}


int http_set_function_to_path(hstates_t *hs, callback_type_t callback, char *path)
{
  int i = hs->path_callback_list_count;

  if (i == HTTP_MAX_CALLBACK_LIST_ENTRY) {
      WARN("Path-callback list is full");
      return -1;
  }

  strcpy(hs->path_callback_list[i].name, path);
  hs->path_callback_list[i].ptr=callback.cb;

  hs->path_callback_list_count = i + 1;

  return 0;
}


/************************************Client************************************/


int http_client_connect(hstates_t * hs, uint16_t port, char *ip)
{
    hs->socket_state = 0;
    hs->server_socket_state=0;
    hs->h_lists.header_list_count = 0;
    hs->h_lists.header_list_count_response = 0;
    hs->path_callback_list_count=0;
    hs->connection_state = 0;

    if (sock_create(hs->socket) < 0) {
        ERROR("Error on client creation");
        return -1;
    }

    hs->socket_state = 1;

    if (sock_connect(hs->socket, ip, port) < 0) {
        ERROR("Error on client connection");
        http_client_disconnect(hs);
        return -1;
    }

    printf("Client connected to server\n");

    hs->connection_state = 1;

    if (h2_client_init_connection(hs) < 0) {
        ERROR("Problems sending client data");
        return -1;
    }

    return 0;
}


int http_client_disconnect(hstates_t *hs)
{
    if (hs->socket_state == 1) {
        if (sock_destroy(hs->socket) < 0) {
            ERROR("Error in client disconnection");
            return -1;
        }

        printf("Client disconnected\n");
    }

    hs->socket_state = 0;
    hs->connection_state = 0;

    return 0;
}


/************************************Headers************************************/

int http_set_header(headers_lists_t* h_lists, char *name, char *value)
{
    int i = h_lists->header_list_count;

    if (i == HTTP2_MAX_HEADER_COUNT) {
        WARN("Headers list is full");
        return -1;
    }

    strcpy(h_lists->header_list[i].name, name);
    strcpy(h_lists->header_list[i].value, value);

    h_lists->header_list_count = i + 1;

    return 0;
}


char *http_get_header(headers_lists_t* h_lists, char *header)
{
    int i = h_lists->header_list_count;

    if (i == 0) {
        WARN("Headers list is empty");
        return NULL;
    }

    int k;
    for (k = 0; k <= i; k++) {
        if (strncmp(h_lists->header_list[k].name, header, strlen(header)) == 0) {
            INFO("RETURNING value of '%s' header; '%s'", h_lists->header_list[k].name, h_lists->header_list[k].value);
            return h_lists->header_list[k].value;
        }
    }

    WARN("Header not found in headers list");
    return NULL;
}


int get_receive(hstates_t *hs)
{
    // preparar respuesta
    char *path=http_get_header(&hs->h_lists, "path");
    callback_type_t callback;

    if (hs->path_callback_list_count == 0) {
        WARN("Path-callback list is empty");
        return -1;
    }

    int i;
    for (i = 0; i <= hs->path_callback_list_count; i++) {
        if (strncmp(hs->path_callback_list[i].name, path, strlen(path)) == 0) {
            callback.cb = hs->path_callback_list[i].ptr;
            break;
        }
        if (i == hs->path_callback_list_count) {
            WARN("No function associated with this path");
            return -1;
        }
    }

    callback.cb(&hs->h_lists);

    if(h2_send_headers(hs)<0){
      return -1;
    }

    return 0;
}
