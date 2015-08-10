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

#ifndef _MAC_H
#define _MAC_H

#include "irframe.h"
#include "codec.h"
#include <SoftwareSerial.h>
#include <stdint.h>
#include <EventManager.h>

extern EventManager eventManager;

#define IR_RX_PIN A3
#define IR_TX_PIN A2 /* this is actually not really used */
#define IR_BAUD 4800

/* notes about encoding:
 *
 * byte stuffing doesn't appear to really work
 *
 * 0x80 transmits fine
 * 0x01 does not
 */
class BlinkerMac : public ByteStuffCodec, public Writer
{
public:
    BlinkerMac();
    void operator() (const uint8_t input)
    {
        serial.write(&input, 1);
    }

    void begin(); //to be called in setup()

    //YOU MUST HANDLE FRAME RECIEVED EVENTS
    //IF YOU DO NOT, MEMORY LEAKS
    enum Events {
        //for frame recv'd events, param is the frame pointer
        //the handler is responsible for freeing with FrameFactory.free()
        ValidFrameRecievedEvent = 400,
        InvalidFrameRecievedEvent,
        PacketNeedsRelayEvent,
        };
    SoftwareSerial serial;

private:
    bool rx_blink;
    uint16_t buf_pos;
    bool packet_in_progress;
    uint8_t prefix_state;
    unsigned long last_rx_time;
    IRFrame* buf;
    uint16_t address;

    uint16_t baud;
    bool invert;

    const uint8_t packet_prefix[4] = { 0xDE, 0xAD, 0xBE, 0xEF };
    const uint8_t preamble_byte = 0xA;
    const uint8_t preamble_count = 8;
    const uint8_t modulation_khz = 38;

    void write_prefix();
    bool prefix_hit(uint8_t c);
    bool blink();
    bool stalled();

public:
    inline void set_address(uint16_t _address)
    {
        address = _address;
    }

    inline uint16_t get_address() const
    {
        return address;
    }

    inline void set_baud_invert(uint16_t _baud, bool _invert)
    {
        baud = _baud;
        invert = invert;
    }

    void reset();
    void send_frame(IRFrame &frame);

    // fires recv'd events
    void recv(const uint8_t input);

    void relayCallback(int event, int param);
};

extern BlinkerMac mac;
extern MemberFunctionCallable<BlinkerMac> RXRelayCB;

#endif /* _RX_H */
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
