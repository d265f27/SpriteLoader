#ifndef CRC32_H
#define CRC32_H

#include <cstdint>
#include <cstdlib>

class crc32
{
public:
    crc32();
    static uint32_t calc_crc_32(const unsigned char *input_str, size_t num_bytes);
    static uint32_t update_crc_32(uint32_t crc, unsigned char c);
};

#endif // CRC32_H
