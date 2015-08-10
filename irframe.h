/*
 * BLINKERCOUGH: NSA Playset implant for IR bridging of airgap
 *
 * Copyright (C) 2015  Hacker, J.R. <r00tkillah@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _IRFRAME_H
#define _IRFRAME_H

#include <stdint.h>

#define FRAME_SIZE        128
#define HEADER_SIZE       (3*sizeof(uint16_t)+1)
#define BLOB_SIZE         (FRAME_SIZE - HEADER_SIZE)
#define BROADCAST_ADDRESS 0xFFFF
#define MAX_HOPS          15

#define NUMFRAMES         4

typedef enum FrameType_t {
    NEIGHBOR_DISCOVER_FRAME,
    NEIGHBOR_DISCOVER_ACK_FRAME,
    DATA_FRAME,
    INVALID_FRAME
} FrameType;

struct __attribute__ ((__packed__)) IRFrame
{
    uint16_t source;
    uint16_t destination;
    unsigned int  type: 4;
    unsigned int  hops: 4;
    uint8_t  payload[BLOB_SIZE];
    uint16_t crc;

    void init();
    void calculate_crc();
    void hton();
    void ntoh();
    bool valid() const;
    uint8_t* blob() const;

    static uint32_t htonl(uint32_t hostlong);
    static uint16_t htons(uint16_t hostshort);
    static uint32_t ntohl(uint32_t netlong);
    static uint16_t ntohs(uint16_t netshort);
    static void hexdump(IRFrame *frame);
    static void copy(IRFrame *dest, IRFrame *src);
};

// The idea here is to provide a pool of frames to avoid memory fragmentation.
class _FrameFactory
{
public:
    _FrameFactory();
    ~_FrameFactory();
    IRFrame *alloc();
    void free(IRFrame *item);
    uint8_t get_high_water_mark() const;
    void print_high_water_mark() const;

private:
    struct BitArray
    {
        BitArray();
        void set(uint8_t bit, bool value);
        bool get(uint8_t bit);
        uint8_t b;
    };
    IRFrame frames[NUMFRAMES];
    BitArray allocated;
    uint8_t high_water_mark;
};

extern _FrameFactory FrameFactory;

#endif /* _IRFRAME_H */

/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
