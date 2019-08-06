#include "hpack_utils.h"
#include <stdint.h>   /* for int8_t */
#include "logging.h"  /* for ERROR  */


/*
 * Function: hpack_utils_read_bits_from_bytes
 * Reads bits from a buffer of bytes (max number of bits it can read is 32).
 * Before calling this function, the caller has to check if the number of bits to read from the buffer
 * don't exceed the size of the buffer, use hpack_utils_can_read_buffer to check this condition
 * Input:
 *      -> current_bit_pointer: The bit from where to start reading (inclusive)
 *      -> number_of_bits_to_read: The number of bits to read from the buffer
 *      -> *buffer: The buffer containing the bits to read
 *      -> buffer_size: Size of the buffer
 * output: returns the bits read from
 */
uint32_t hpack_utils_read_bits_from_bytes(uint16_t current_bit_pointer, uint8_t number_of_bits_to_read, uint8_t *buffer)
{
    uint32_t byte_offset = current_bit_pointer / 8;
    uint8_t bit_offset = current_bit_pointer - 8 * byte_offset;
    uint8_t num_bytes = ((number_of_bits_to_read + current_bit_pointer - 1) / 8) - (current_bit_pointer / 8) + 1;

    uint32_t mask = 1 << (8 * num_bytes - number_of_bits_to_read);
    mask -= 1;
    mask = ~mask;
    mask >>= bit_offset;
    mask &= ((1 << (8 * num_bytes - bit_offset)) - 1);
    uint32_t code = buffer[byte_offset];
    for (int i = 1; i < num_bytes; i++) {
        code <<= 8;
        code |= buffer[i + byte_offset];
    }
    code &= mask;
    code >>= (8 * num_bytes - number_of_bits_to_read - bit_offset);

    return code;
}

/*
 * Function: hpack_utils_check_if_can_read_buffer
 * Checks if the number of bits to read from the buffer from the current_bit_pointer doesn't exceed the size of the buffer
 * This function is meant to be used to check if hpack_utils_read_bits_from_bytes can read succesfully from the buffer
 * Input:
 *      -> current_bit_pointer: Bit number from where to start reading in the buffer(this is not a byte pointer!)
 *      -> number_of_bits_to_read: Number of bits to read from the buffer starting from the current_bit_pointer (inclusive)
 *      -> buffer_size: Size of the buffer to read in bytes.
 * Output:
 *      Returns 0 if the required amount of bits can be read from the buffer, or -1 otherwise
 */
int8_t hpack_utils_check_can_read_buffer(uint16_t current_bit_pointer, uint8_t number_of_bits_to_read, uint8_t buffer_size)
{
    if (current_bit_pointer + number_of_bits_to_read > 8 * buffer_size) {
        return -1;
    }
    return 0;
}

/*
 * Function: hpack_utils_log128
 * Compute the log128 of the input
 * Input:
 *      -> x: variable to apply log128
 * Output:
 *      returns log128(x)
 */
int hpack_utils_log128(uint32_t x)
{
    uint32_t n = 0;
    uint32_t m = 1;

    while (m < x) {
        m = 1 << (7 * (++n));
    }

    if (m == x) {
        return n;
    }
    return n - 1;
}

/*
 * Function: hpack_utils_get_preamble
 * Matches a numeric preamble to a hpack_preamble_t
 * Input:
 *      -> preamble: Number representing the preamble of the integer to encode
 * Output:
 *      An hpack_preamble_t if it can parse the given preamble or -1 if it fails
 */
hpack_preamble_t hpack_utils_get_preamble(uint8_t preamble)
{
    if (preamble & INDEXED_HEADER_FIELD) {
        return INDEXED_HEADER_FIELD;
    }
    if (preamble & LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING) {
        return LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING;
    }
    if (preamble & DYNAMIC_TABLE_SIZE_UPDATE) {
        return DYNAMIC_TABLE_SIZE_UPDATE;
    }
    if (preamble & LITERAL_HEADER_FIELD_NEVER_INDEXED) {
        return LITERAL_HEADER_FIELD_NEVER_INDEXED;
    }
    if (preamble < 16) {
        return LITERAL_HEADER_FIELD_WITHOUT_INDEXING; // preamble = 0000
    }
    ERROR("wrong preamble");
    return -1;
}

/*
 * Function: find_prefix_size
 * Given the preamble octet returns the size of the prefix
 * Input:
 *      -> octet: Preamble of encoding
 * Output:
 *      returns the size in bits of the prefix.
 */
uint8_t hpack_utils_find_prefix_size(hpack_preamble_t octet)
{
    if ((INDEXED_HEADER_FIELD & octet) == INDEXED_HEADER_FIELD) {
        return (uint8_t)7;
    }
    if ((LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING & octet) == LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING) {
        return (uint8_t)6;
    }
    if ((DYNAMIC_TABLE_SIZE_UPDATE & octet) == DYNAMIC_TABLE_SIZE_UPDATE) {
        return (uint8_t)5;
    }
    return (uint8_t)4; /*LITERAL_HEADER_FIELD_WITHOUT_INDEXING and LITERAL_HEADER_FIELD_NEVER_INDEXED*/
}

/* Function: encoded_integer_size
 * Input:
 *      -> num: Number to encode
 *      -> prefix: Size of prefix
 * Output:
 *      returns the amount of octets used to encode num
 */
uint32_t hpack_utils_encoded_integer_size(uint32_t num, uint8_t prefix)
{
    uint8_t p = (1 << prefix) - 1;

    if (num < p) {
        return 1;
    }
    else if (num == p) {
        return 2;
    }
    else {
        uint32_t k = hpack_utils_log128(num - p);//log(num - p) / log(128);
        return k + 2;
    }
}