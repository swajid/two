#include "http2/flowcontrol.h"

// Specify to which module this file belongs
#include "config.h"
#define LOG_MODULE LOG_MODULE_HTTP2
#include "logging.h"

/*
 * Function get_window_available_size
 * Returns the available window size of the given window manager. It indicates
 * the octets available in the corresponding endpoint to process data.
 * Input: ->window_manager: h2_window_manager_t struct where window info is stored
 * Output: The available window size
 */
int32_t get_window_available_size(h2_flow_control_window_t flow_control_window)
{
    return flow_control_window.stream_window;
}

/*
 * Function update_window_size
 * Update window size value and window used value if necessary.
 * @param     window_manager     h2_window_manager_t struct where window info is stored
 * @param     new_window_size    new value of window size
 *
 * @returns   1
 */
uint32_t update_window_size(h2states_t *h2s, uint32_t initial_window_size, uint8_t place)
{
    int id = INITIAL_WINDOW_SIZE;
    if (place == LOCAL) {
        h2s->local_window.stream_window += initial_window_size - h2s->local_settings[--id];
        // An endpoint MUST treat a change to SETTINGS_INITIAL_WINDOW_SIZE that
        // causes any flow-control window to exceed the maximum size as a
        // connection error of type FLOW_CONTROL_ERROR.
        if (h2s->local_window.stream_window > 2147483647) {
            return HTTP2_RC_ERROR;
        }
    }
    else if (place == REMOTE) {
        h2s->remote_window.stream_window += initial_window_size - h2s->remote_settings[--id];
        if (h2s->remote_window.stream_window > 2147483647) {
            return HTTP2_RC_ERROR;
        }
    }
    return HTTP2_RC_NO_ERROR;
}

/*
 * Function: increase_window_used
 * Increases the window_used value on a given window manager.
 * Input: ->flow_control_window: h2_flow_control_window_t struct pointer where window info is stored
 *        ->data_size: the corresponding increment on the window used
 * Output: 0
 */
int decrease_window_available(h2_flow_control_window_t *flow_control_window, uint32_t data_size)
{
    flow_control_window->stream_window -= data_size;
    return HTTP2_RC_NO_ERROR;
}

/*
 * Function: decrease_window_used
 * Decreases the window_used value on a given window manager.
 * Input: ->flow_control_window: h2_flow_control_window_t struct where window info is stored
 *        ->data_size: the corresponding decrement on the window used
 * Output: 0
 */
int increase_window_available(h2_flow_control_window_t *flow_control_window, uint32_t window_size_increment)
{
    flow_control_window->stream_window += window_size_increment;
    return HTTP2_RC_NO_ERROR;
}

/*
 * Function: flow_control_receive_data
 * Checks if the incoming data fits on the available window.
 * Input: -> st: hstates_t struct pointer where connection windows are stored
 *        -> length: the size of the incoming data received
 * Output: 0 if data can be processed. -1 if not
 */
int flow_control_receive_data(h2states_t *h2s, uint32_t length)
{
    uint32_t window_available = get_window_available_size(h2s->remote_window);

    if (length > window_available) {
        ERROR("Available window is smaller than data received. FLOW_CONTROL_ERROR");
        return HTTP2_RC_ERROR;
    }
    decrease_window_available(&h2s->local_window, length);
    return HTTP2_RC_NO_ERROR;
}

/*
 * Function: flow_control_send_data
 * Updates the outgoing window when certain amount of data is sent.
 * Input: -> st: hstates_t struct pointer where connection windows are stored
 *        -> data_sent: the size of the outgoing data.
 * Output: 0 if window increment was successfull. -1 if not
 */
int flow_control_send_data(h2states_t *h2s, uint32_t data_sent)
{
    if ((int)data_sent > get_window_available_size(h2s->remote_window)) {
        ERROR("Trying to send more data than allowed by window.");
        return HTTP2_RC_ERROR;
    }
    decrease_window_available(&h2s->remote_window, data_sent);
    return HTTP2_RC_NO_ERROR;
}

int flow_control_send_window_update(h2states_t *h2s, uint32_t window_size_increment)
{
    //TODO: Check the validity of this error
    /*
    if (window_size_increment > h2s->local_window.window_used) {
        ERROR("Increment to big. PROTOCOL_ERROR");
        return HTTP2_RC_ERROR;
    }
    */
    increase_window_available(&h2s->local_window, window_size_increment);
    return HTTP2_RC_NO_ERROR;
}

int flow_control_receive_window_update(h2states_t *h2s, uint32_t window_size_increment)
{
    if (h2s->header.stream_id == 0) {
        h2s->remote_window.connection_window += window_size_increment;
        if (h2s->remote_window.connection_window < 0) {
            DEBUG("flow_control_receive_window_update: Remote connection window overflow!");
            return HTTP2_RC_ERROR;
        }
    }
    else if (h2s->header.stream_id == h2s->current_stream.stream_id) {
        h2s->remote_window.stream_window += window_size_increment;
        if (h2s->remote_window.stream_window < 0) {
            DEBUG("flow_control_receive_window_update: Remote stream window overflow!");
            return HTTP2_RC_ERROR;
        }
    }
    return HTTP2_RC_NO_ERROR;
}


uint32_t get_size_data_to_send(h2states_t *h2s)
{
    uint32_t available_window = get_window_available_size(h2s->remote_window);

    if (available_window <= h2s->data.size - h2s->data.processed) {
        return available_window;
    }
    else {
        return h2s->data.size - h2s->data.processed;
    }
    return HTTP2_RC_NO_ERROR;
}
