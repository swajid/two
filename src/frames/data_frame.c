//
// Created by gabriel on 04-11-19.
//

#include "data_frame.h"
#include "http2/utils.h"
#include "logging.h"

/*
 * Function: data_payload_to_bytes
 * Passes a data payload to a byte array
 * Input: frame_header, data_payload pointer, array to save the bytes
 * Output: size of the array of bytes, -1 if error
 */
int data_payload_to_bytes(frame_header_t *frame_header, void *payload, uint8_t *byte_array)
{
    data_payload_t *data_payload = (data_payload_t *) payload;
    int length = frame_header->length;
    uint8_t flags = frame_header->flags;

    if (is_flag_set(flags, DATA_PADDED_FLAG)) {
        //TODO handle padding
        ERROR("Padding not implemented yet");
        return -1;
    }
    int rc = buffer_copy(byte_array, data_payload->data, length);
    if (rc < 0) {
        ERROR("error copying buffer");
        return -1;
    }
    return rc;
}

/*
 * Function: create_data_frame
 * Creates a data frame
 * Input: frame_header, data_payload, data array for data payload, data_to_send(data to save in the data_frame), size of the data to send, stream_id
 * Output: 0 if no error
 */
int create_data_frame(frame_header_t *frame_header, data_payload_t *data_payload, uint8_t *data, uint8_t *data_to_send, int length, uint32_t stream_id)
{
    uint8_t type = DATA_TYPE;
    uint8_t flags = 0x0;

    //uint32_t length = length; //no padding, no dependency. fix if this is implemented

    frame_header->length = length;
    frame_header->type = type;
    frame_header->flags = flags;
    frame_header->stream_id = stream_id;
    frame_header->reserved = 0;
    frame_header->callback_to_bytes = data_payload_to_bytes;

    buffer_copy(data, data_to_send, length);
    data_payload->data = data; //not duplicating info
    return 0;
}


/*
 * Function: read_data_payload
 * given a byte array, get the data payload encoded in it
 * Input: byte_array, frame_header, data_payload, data array
 * Output: byteget_header_block_fragment_sizes read or -1 if error
 */
int read_data_payload(frame_header_t *frame_header, void *payload, uint8_t *bytes)
{
    data_payload_t *data_payload = (data_payload_t *) payload;
    uint8_t flags = frame_header->flags;
    int length = frame_header->length;

    if (is_flag_set(flags, DATA_PADDED_FLAG)) {
        //TODO handle padding
        ERROR("Padding not implemented yet");
        return -1;
    }
    buffer_copy(data_payload->data, bytes, length);
    return length;
}