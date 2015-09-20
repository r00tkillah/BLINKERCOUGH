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

#ifndef _CODEC_H
#define _CODEC_H

#include "util.h"
#include <stdint.h>
#include <Arduino.h>
#include <SoftwareSerial.h>

#define BYTESTUFF 3

struct Writer
{
    virtual void operator() (uint8_t input)  = 0;
};

class Codec
{
public:
    virtual ~Codec() {}
    virtual void encode_byte(uint8_t input, Writer &writer) = 0;
    virtual bool decode_byte(uint8_t input, uint8_t &output) = 0;
    virtual void reset_codec_state() = 0;
};

class ByteStuffCodec
{
public:
    //FIXME: this will not work because stream has no write method
    // we need to refactor this (again)
    // possibly taking it from Stream to SoftwareSerial
    void encode_byte(uint8_t input, Writer &writer)
    {
        encoder_cnt = (encoder_cnt + 1) % BYTESTUFF;
        if (encoder_cnt % BYTESTUFF == 0) {
            const uint8_t ff = 0xff;
            writer(ff);
            encoder_cnt++;
        }
        writer(input);
        delay(1);
    }

    bool decode_byte(uint8_t input, uint8_t &output)
    {
        debugcont(" [%02X", input);
        decoder_cnt = (decoder_cnt + 1) % BYTESTUFF;
        if (decoder_cnt % BYTESTUFF == 0) {
            if (input != 0xFF) {
                debugcont("?");
            }
            debugcont("!] ");
            //input should be 0xFF here
            return false;
        }
        debugcont("] ");
        output = input;
        return true;
    }

    void reset_codec_state()
    {
        encoder_cnt = 1;
        decoder_cnt = 1;
    }

private:
    uint8_t encoder_cnt;
    uint8_t decoder_cnt;
};

#if 0
class BitStuffCodec : Codec
{
public:
    void encode_byte(uint8_t input, void (write)(uint8_t))
    {
        //we will ship out 7 bits this cycle
        // that means we will ship out 7-qlen bits of our input
        // which means our leftover bits (or new qlen) is going to be
        // 8-(7-qlen)

        uint8_t output = ebitq | (input << eqlen) | 0x80;
        //we need to shift the input (where the leftovers come from) over by
        //8-(7-qlen)
        eqlen = 8 - (7 - eqlen);
        ebitq = input >> (8 - eqlen);
        write(output);

        if (eqlen == 7) {
            output = ebitq;
            output |= 0x80;
            ebitq = 0x0;
            eqlen = 0;
            write(output);
        }
    }

    bool decode_byte(uint8_t input, uint8_t &output)
    {
        if (dqlen == 0) {
            dbitq = input & 0x7f;
            dqlen = 7;
            return false;
        }

        output = dbitq | ((input << dqlen) & ~(0xff >> (8 - dqlen)));
        dbitq = (input & 0x7f) >> (8 - dqlen);
        dqlen--;
        return true;
    }

    void reset_codec_state()
    {
        ebitq = 0x00;
        eqlen = 0;
        dbitq = 0x00;
        dqlen = 0;
    }

private:
    uint8_t ebitq;
    uint8_t eqlen;
    uint8_t dbitq;
    uint8_t dqlen;
};
#endif


#endif /* _CODEC_H */
/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
