#ifndef TWO_H
#define TWO_H

#include <stdint.h>

#include "resource_handler.h"



/*
 * Given a port number, this function start a server
 *
 * @param    port       Port number
 *
 * @return   0          Server was successfully performance
 * @return   -1         An error occurred while the server was running
 */
int two_server_start(unsigned int port);

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
 * @param   method      HTTP method for the resource
 * @param   path        Path string, as described above
 * @param   handler     Callback handler
 *
 * @return  0           if ok 
 * @return  -1          if error
 */
int two_register_resource(char *method, char *path, http_resource_handler_t handler);


#endif /* TWO_H */
