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

#include "irframe.h"
#include "crc16.h"
#include <string.h>
#include <Arduino.h>
#include "util.h" //FIXME shouldn't need this
#include <util/atomic.h>

_FrameFactory FrameFactory;

#define NEIGHBOR_MAX  8
#define XMIT_Q_DEPTH  2

#define __uint64_t uint64_t
#define __uint32_t uint32_t
#define __uint16_t uint16_t

/* the following is stolen from freebsd from sys/x86/include/endian.h
 * and then slightly mangled
 */
#define __bswap16(x)        (__uint16_t)((x) << 8 | (x) >> 8)
#define __bswap32(x)                \
    (((__uint32_t)__bswap16((x) & 0xffff) << 16) | __bswap16((x) >> 16))
#define __bswap64(x)                \
    (((__uint64_t)__bswap32((x) & 0xffffffff) << 32) | __bswap32((x) >> 32))

/* avr and pc is little endian */
uint32_t IRFrame::htonl(uint32_t hostlong)
{
    return __bswap32(hostlong);
}

uint16_t IRFrame::htons(uint16_t hostshort)
{
    return __bswap16(hostshort);
}

uint32_t IRFrame::ntohl(uint32_t netlong)
{
    return __bswap32(netlong);
}

uint16_t IRFrame::ntohs(uint16_t netshort)
{
    return __bswap16(netshort);
}

void IRFrame::init()
{
    memset((void*)this, 0, sizeof(IRFrame));
}

void IRFrame::calculate_crc()
{
    crc16_append(blob(), FRAME_SIZE);
}

static void irframe_swap(IRFrame *frame)
{
    frame->source = __bswap16(frame->source);
    frame->destination = __bswap16(frame->destination);
}

void IRFrame::hton()
{
    irframe_swap(this);
}

void IRFrame::ntoh()
{
    irframe_swap(this);
}

bool IRFrame::valid() const
{
    if (!crc16_check(blob(), FRAME_SIZE)) {
        return false;
    }

    if (type >= INVALID_FRAME) {
        return false;
    }

    return true;
}

uint8_t* IRFrame::blob() const
{
    return (uint8_t*)this;
}

void IRFrame::hexdump(IRFrame *frame)
{
    uint8_t *blob = frame->blob();
    for (uint16_t i = 0; i < sizeof(IRFrame); i += 8) {
        debugstart("%03d ", i);
        for (uint8_t j = 0; j < 8; j++) {
            debugcont("%02x ", blob[i+j]);
        }
        debugend();
    }
}

void IRFrame::copy(IRFrame *dest, IRFrame *src)
{
    if (dest && src) {
        memcpy((void*)dest, (void*)src, sizeof(IRFrame));
    }
}

_FrameFactory::_FrameFactory()
{
    high_water_mark = 0;
}

_FrameFactory::~_FrameFactory()
{
}

_FrameFactory::BitArray::BitArray()
{
    b = 0;
}

void _FrameFactory::BitArray::set(uint8_t bit, bool value)
{
    bit = 7 - bit;
    bitWrite(b, bit, value);
}

bool _FrameFactory::BitArray::get(uint8_t bit)
{
    bit = 7 - bit;
    return bitRead(b, bit);
}


IRFrame* _FrameFactory::alloc()
{
    IRFrame *ret = NULL;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        for (uint8_t i = 0; i < NUMFRAMES; i++) {
            if (!allocated.get(i)) {
                allocated.set(i, true);
                ret = &(frames[i]);
                memset((void*)ret, 0, sizeof(IRFrame));
                break;
            }
        }
        uint8_t water_level = 0;
        for (uint8_t i = 0; i < NUMFRAMES; i++) {
            if (allocated.get(i)) {
                water_level++;
            }
        }
        high_water_mark = max(high_water_mark, water_level);
    }

    return ret;
}

void _FrameFactory::free(IRFrame *item)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        if (item == NULL) {
            return;
        }
        memset((void*)item, 0, sizeof(IRFrame));
        for (uint8_t i = 0; i < NUMFRAMES; i++) {
            if (item == &(frames[i])) {
                allocated.set(i, false);
                break;
            }
        }
    }
}

void _FrameFactory::print_high_water_mark() const
{
    debug("high water mark: %d", high_water_mark);
}

uint8_t _FrameFactory::get_high_water_mark() const
{
    return high_water_mark;
}


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
