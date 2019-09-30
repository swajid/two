/*
   This API contains the methods in HTTP layer
*/
#ifndef HTTP_H
#define HTTP_H

#include "http_bridge.h"
#include "headers.h"

/***********************************************
 * Server API methods
 *
 * TODO: make server/client methods toggable
 * by use of a macro (reduce binary size)
 ***********************************************/

/*
 * Given a port number and a struct for server information, this function
 * initialize a server
 *
 * @param    hs         Struct for server information
 * @param    port       Port number
 *
 * @return   0          Server was successfully initialized
 * @return   -1         Server wasn't initialized
 */
int http_server_create(hstates_t *hs, uint16_t port);


/*
 * Given a struct with server information, this function starts the server
 *
 * @param    hs         Struct with server information
 *
 * @return   0          Server was successfully started
 * @return   -1         There was an error in the process
 */
int http_server_start(hstates_t *hs);


/*
 * Stop and destroy server if it is running
 *
 * @param    hs         Struct with server information
 *
 * @return   0          Server was successfully destroyed
 * @return   -1         Server wasn't destroyed
 */
int http_server_destroy(hstates_t *hs);

/**
 * Set callback to handle an http resource
 *
 * A resource is composed by a method and a path
 *
 * A path consists of a sequence of path segments separated by a slash
 * ("/") character.  A path is always defined for a URI, though the
 * defined path may be empty (zero length)
 * (More info in https://tools.ietf.org/html/rfc3986#section-3.3)
 *
 * For this function, the path must start with a '/'.
 *
 * Attempting to define a malformed path, or a path for an unsupported method
 * will result in an error return
 *
 * @return 0 if ok, -1 if error
 */
int http_server_register_resource(hstates_t * hs, char * method, char * path, http_resource_handler_t_1 fn);


/*************************************************************
 * Client API methods
 *************************************************************/


/*
 * Initialize a connection from client to server
 *
 * @param    hs         Struct with client information
 * @param    addr       Server address
 * @param    port       Port number
 *
 * @return   0 if connection was succesful or -1 if an error ocurred
 */
int http_client_connect(hstates_t *hs, char *address, uint16_t port);


/*
 * Send a GET request to server and wait for a reply
 *
 * @param    hs                         Struct with client information
 * @param    uri                        Request URI
 * @param    response                   Pointer to a char array where to store the server response
 * @param    size                       Pointer to size_t storing the maximum response size expected, the value will be updated upon server response
 * @return http status or -1 if an unexpected error ocurred
 */
int http_get(hstates_t *hs, char *uri, uint8_t * response, size_t * size);


/*
 * Send a HEAD request to server and wait for a reply
 *
 * @param    hs                         Struct with client information
 * @param    uri                        Request URI
 * @param    response                   Pointer to a char array where to store the server response
 * @param    size                       Pointer to size_t storing the maximum response size expected, the value will be updated upon server response
 * @return http status or -1 if an unexpected error ocurred
 */
int http_head(hstates_t *hs, char *uri, uint8_t *response, size_t *size);


/*
 * Stop the connection between client and server
 *
 * @param    hs         Struct with client information
 *
 * @return    0         Connection was stopped
 * @return    -1        Failed to stop connection
 */
int http_client_disconnect(hstates_t *hs);


#endif /* HTTP_H */
