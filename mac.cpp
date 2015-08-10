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

#include "mac.h"
#include "util.h"
#include <Arduino.h>

BlinkerMac mac;
MemberFunctionCallable<BlinkerMac> MACRelayCB(&mac, &BlinkerMac::relayCallback);

BlinkerMac::BlinkerMac() : serial(SoftwareSerial(IR_RX_PIN, IR_TX_PIN, false))
{
}

void BlinkerMac::begin()
{
    serial.begin(IR_BAUD);
    reset();
    buf = FrameFactory.alloc();
    eventManager.addListener(BlinkerMac::PacketNeedsRelayEvent,
                             &MACRelayCB);
}

void BlinkerMac::reset()
{
    rx_blink = false;
    packet_in_progress = false;
    buf_pos = 0;
    prefix_state = 0;
    reset_codec_state();
}

void BlinkerMac::write_prefix()
{
    for (uint8_t i = 0; i < preamble_count; i++) {
        (*this)(preamble_byte);
    }
    for (uint8_t i = 0; i < sizeof(packet_prefix); i++) {
        (*this)(packet_prefix[i]);
    }
}

bool BlinkerMac::prefix_hit(uint8_t input)
{
    debug("prefix state: %d; input=%02x", prefix_state, input);
    if (prefix_state == 0) {
        if (input == packet_prefix[0]) {
            prefix_state = 1;
        }
    } else if (prefix_state == 1) {
        if (input == packet_prefix[1]) {
            prefix_state = 2;
        } else {
            prefix_state = 0;
        }
    } else if (prefix_state == 2) {
        if (input == packet_prefix[2]) {
            prefix_state = 3;
        } else {
            prefix_state = 0;
        }
    } else if (prefix_state == 3) {
            prefix_state = 0;
        if (input == packet_prefix[3]) {
            return true;
        }
    }
    return false;
}

bool BlinkerMac::blink()
{
    rx_blink = rx_blink ? false : true;
    return rx_blink;
}

bool BlinkerMac::stalled()
{
    const unsigned long now = millis();

    if (now - last_rx_time > 250) {
        return true;
    }
    last_rx_time = now;
    return false;
}

//expectation here is that hton is done
//and crc is calculated
void BlinkerMac::send_frame(IRFrame &frame)
{
    debug("Sending packet");

    write_prefix();
    for (uint16_t i = 0; i < sizeof(IRFrame); i++) {
        encode_byte(frame.blob()[i], *this);
    }
    serial.flush();

    // while this is happening, you'll get crap that is your own packet.
    // clear out any remaining input
    while(serial.available()) {
        (void)serial.read();
        delay(1);
    }
}

void BlinkerMac::recv(const uint8_t c)
{
    digitalWrite(13, blink());

    // if you haven't gotten a byte in a while, then what you have
    // is probably garbage.
    if (packet_in_progress && stalled()) {
        Serial.println("aborting rcv");
        IRFrame::hexdump(buf);
        reset();
        return;
    }
    if (!packet_in_progress) {
        packet_in_progress = prefix_hit(c);
        last_rx_time = millis();
        return; //the next byte should be the first byte of a packet
    }

    debugstart("%d", buf_pos);
    debugcont(" ");
    //geting a packet
    uint8_t input;
    if (decode_byte(c, input)) {
        buf->blob()[buf_pos] = input;
        buf_pos++;
    } else {
        return; // was a stuffed byte
    }

    if (buf_pos >= sizeof(IRFrame)) {
        //whole packet recieved!
        buf_pos = 0;
        packet_in_progress = false;

        if (buf->valid()) {
            buf->ntoh();

            if (buf->destination != address) {
                if (buf->hops < MAX_HOPS) {
                    auto newbuf = FrameFactory.alloc();
                    IRFrame::copy(newbuf, buf);
                    eventManager.queueEvent(PacketNeedsRelayEvent, (int)newbuf);
                    return;
                } else {
                    debugcont("too many hops.. eating");
                    debugend();
                    reset();
                    return;
                }
            }

            auto newbuf = FrameFactory.alloc();
            IRFrame::copy(newbuf, buf);
            eventManager.queueEvent(ValidFrameRecievedEvent, (int)newbuf);
            return;
        } else {
            auto newbuf = FrameFactory.alloc();
            IRFrame::copy(newbuf, buf);
            eventManager.queueEvent(InvalidFrameRecievedEvent, (int)newbuf);
            reset();
        }
    }
    return;
}

void BlinkerMac::relayCallback(int event, int param)
{
    //niave approach.. just send it back out this is kind of wrong we
    //should wait a random time before retransmit and as we get
    //packets, we should cancel retransmit if we saw a neighbor do it
    //first.  but when we see that neighbor do it, we schedule a new
    //one to retransmit.

    IRFrame *buf = (IRFrame*)param;
    debug("Packet from 0x%04x to 0x%04x hop %d",
          buf->source, buf->destination,
          buf->hops);

    buf->hops++;
    buf->hton();
    buf->calculate_crc();
    debug("Retransmitting packet...");
    send_frame(*buf);
    debug("done");
    reset();

    FrameFactory.free(buf);
}


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
