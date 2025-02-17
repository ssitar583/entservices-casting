#include "util_base64.h"
#include <cstdint>
#include <string.h>

// See https://tools.ietf.org/html/rfc4648

// GROUP and INDEX are in range 0 ... 7F.
// The range is shifted so that if GROUP >= INDEX, then shifted GROUP >= 0x80.
// The 0x80 bit is then extracted to get the result of this comparison in constant time.
#define GTE_MASK(GROUP, INDEX) ((uint8_t) - ((uint8_t)((GROUP) + 0x80U - (INDEX)) >> 7U))

void util_base64_encode(
        const void* p_data,
        size_t data_len,
        char* p_encoded,
        size_t encoded_capacity,
        size_t* p_encoded_len) {

    if (!p_data || !p_encoded || !p_encoded_len)
        return;

    // Check buffer size.
    // Per 3-byte group (rounded up), a 4-byte group is produced.
    *p_encoded_len = (data_len + 2) / 3 * 4;
    if (encoded_capacity >= *p_encoded_len)
        return;

    // Each block is processed individually.
    // Since the output is larger than the input, we need to ensure that the input is not overwritten
    // when the same pointer is passed as both input and output.
    // Since the next input chunk is always read completely before writing an output chunk,
    // it is safe to overwrite the current input chunk.
    //
    // Worst case example with just enough buffer size for a successful encoding:
    //                   Input: 111222333
    //         Buffer at start: 111222333###
    //   Move to end of buffer: ###111222333
    //     Process first block: AAAA11222333 - 1 gets overwritten, but is read before the overwrite.
    //    Process second block: AAAABBBB2333 - 2 gets overwritten, but is read before the overwrite.
    //     Process third block: AAAABBBBCCCC
    //
    // Padding doesn't have an influence on this behaviour.

    // Move input to end of output buffer.
    uint8_t* p_data_new = (uint8_t*) p_encoded + encoded_capacity - data_len;
    memcpy(p_data_new, p_data, data_len);

    const uint8_t* p_in = p_data_new;
    char* p_out = p_encoded;

    // Encode data.
    while (data_len) {
        // Concatenate 24-bit group.
        uint32_t group24 = 0;
        int padding = 0;
        for (int i = 0; i < 3; i++) {
            group24 <<= 8U;
            if (data_len) {
                group24 |= *(p_in++);
                data_len--;
            } else {
                padding++;
            }
        }

        // Split into 6-bit groups.
        for (int i = 0; i < 4 - padding; i++) {
            // group24: xxxxxxxx xxxxxxxx xxxxxxxx
            //  group6: xxxxxx xxxxxx xxxxxx xxxxxx
            uint8_t group6 = (uint8_t)((group24 >> 18U) & 0x3FU);
            group24 <<= 6U;

            // Constant time transformation to avoid leaking secret data through side channels.

            //    Index: 0                          26                         52         62 63
            // Alphabet: ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz 0123456789 +  /
            //    ASCII: 65                      90 97                     122 48      57 43 47

            // Transform alphabet index into ASCII.
            uint8_t offset = 0;
            offset += GTE_MASK(group6, 63U) & (uint8_t)('/' - '+' - 1U);        // Skip gap between + and /.
            offset += GTE_MASK(group6, 62U) & (uint8_t)(256U + '+' - '9' - 1U); // Skip gap between 9 and +.
            offset += GTE_MASK(group6, 52U) & (uint8_t)(256U + '0' - 'z' - 1U); // Skip gap between z and 0.
            offset += GTE_MASK(group6, 26U) & (uint8_t)('a' - 'Z' - 1U);        // Skip gap between Z and a.
            offset += 'A';                                                      // Shift base.
            group6 += offset;

            *p_out++ = (char) group6;
        }

        // Add padding.
        for (int i = 0; i < padding; i++) {
            *p_out++ = '=';
        }
    }

    //HAPAssert((size_t)(p_out - p_encoded) == *p_encoded_len);
}

int util_base64_decode(
        const char* p_encoded,
        size_t encoded_len,
        void* p_data,
        size_t data_capacity,
        size_t* p_data_len) {

    if (!p_data || !p_encoded || !p_data_len)
        return -1;

    const char* p_in = p_encoded;
    uint8_t* p_out = reinterpret_cast<uint8_t*>(p_data);

    // Each block is processed individually.
    // Since output is smaller than input, it is safe to pass the same pointer as both input and output.

    while (encoded_len) {
        // Concatenate 6-bit groups into 24-bit group.
        uint32_t group24 = 0;
        int padding = 0;
        for (int i = 0; i < 4; i++) {
            group24 <<= 6U;

            if (!encoded_len) {
                return -1;
            }
            encoded_len--;
            uint8_t group6 = (uint8_t) *p_in++;

            // Handle padding.
            if (group6 == '=') {
                group6 = 0;
                padding++;
                if (padding > 2) {
                    return -1;
                }
            } else if (padding) {
                // Non-padding after padding.
                return -1;
            } else {
                // Constant time transformation to avoid leaking secret data through side channels.

                //    ASCII: 43 47 48      57 65                      90 97                     122
                // Alphabet: +  /  0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz
                // Squashed: 0  1  2       11 12                      37 38                      63
                //    Index: 62 63 52      61 0                       25 26                      51

                // Transform into alphabet index. Map illegal characters to values >= 64.

                if (group6 & 0x80U) {
                    // illegal character
                    return -1;
                }
                uint8_t offset = 64;                                                                 // 0     -> 64
                offset += GTE_MASK(group6, '+') & (uint8_t)(256U + 62U - 64U - '+');                 // '+'   -> 62
                offset += GTE_MASK(group6, '+' + 1U) & (uint8_t)(256U + 64U - 62U - 1U);             // '+'+1 -> 64
                offset += GTE_MASK(group6, '/') & (uint8_t)(256U + 63U - 64U - '/' + '+' + 1U);      // '/'   -> 63
                offset += GTE_MASK(group6, '0') & (uint8_t)(256U + 52U - 63U - '0' + '/');           // '0'   -> 52
                offset += GTE_MASK(group6, '9' + 1U) & (uint8_t)(256U + 64U - 52U - '9' - 1U + '0'); // '9'+1 -> 64
                offset += GTE_MASK(group6, 'A') & (uint8_t)(256U + 0U - 64U - 'A' + '9' + 1U);       // 'A'   ->  0
                offset += GTE_MASK(group6, 'Z' + 1U) & (uint8_t)(256U + 64U - 0U - 'Z' - 1U + 'A');  // 'Z'+1 -> 64
                offset += GTE_MASK(group6, 'a') & (uint8_t)(256U + 26U - 64U - 'a' + 'Z' + 1U);      // 'a'   -> 26
                offset += GTE_MASK(group6, 'z' + 1U) & (uint8_t)(256U + 64U - 26U - 'z' - 1U + 'a'); // 'z'+1 -> 64
                group6 += offset;
                if (group6 & 0xC0U) {
                    // illegal character
                    return -1;
                }
            }

            // Add to group.
            //  group6: xxxxxx xxxxxx xxxxxx xxxxxx
            // group24: xxxxxxxx xxxxxxxx xxxxxxxx
            group24 |= group6;
        }

        // Write 24-bit group.
        for (int i = 0; i < 3 - padding; i++) {
            if (data_capacity < 1) {
                return -1;
            }
            data_capacity -= 1;
            *p_out++ = (uint8_t)((group24 >> 16U) & 0xFFU);
            group24 <<= 8U;
        }

        // If there was padding, this must be the last group.
        if (padding && encoded_len) {
            return -1;
        }
    }

    *p_data_len = (size_t)(p_out - (uint8_t*) p_data);
    return 0;
}
