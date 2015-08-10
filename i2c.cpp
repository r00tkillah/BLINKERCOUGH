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

#include "i2c.h"
#include "util.h"
#include "EventManager.h"
#include "mac.h"

extern EventManager eventManager;

#define I2CTXEVENT 500

i2cRegisters i2cRegs;

void xmit(int event, int param);
GenericCallable<void(int,int)> i2c_xmit(xmit);
MemberFunctionCallable<i2cRegisters> recv_hook(&i2cRegs, &i2cRegisters::recvHook);


void i2cReceiveCallback(int count)
{
    int a = 0;
    uint8_t address = 0;
    uint8_t data = 0;

    while (Wire.available() > 0) {
        uint8_t d = Wire.read();

        if (a == 0) {
            address = d;
        } else if (a == 1) {
            data = d;
        }
        a++;
    }
    if (a == 1) {
        //This is a read and the byte is the address.

        //calling with the c mode seems to cause a request to get issued.
        i2cRegs.address = address;
    }
    if (a >= 2) {
        //This is a write
        i2cRegs.write(address, data);
    }
}

void i2cRequestCallback(void)
{
    Wire.write(i2cRegs.read());
}

i2cRegisters::i2cRegisters() : rxq_depth(0), rxq(0), txq_depth(0), txq(0)
{
}

void i2cRegisters::begin()
{
    Wire.begin(SLAVE_ADDRESS);
    Wire.onReceive(i2cReceiveCallback);
    Wire.onRequest(i2cRequestCallback);

    eventManager.addListener(I2CTXEVENT, &i2c_xmit);
    eventManager.addListener(BlinkerMac::ValidFrameRecievedEvent, &recv_hook);
}

void xmit(int event, int param)
{
    auto frame = (IRFrame*)param;

    debug("sending frame reg set to 0x%04x", frame->destination);

    frame->hops = 0;
    frame->source = mac.get_address();
    frame->type = DATA_FRAME;
    frame->hton();
    frame->calculate_crc();

    IRFrame::hexdump(frame);
    mac.send_frame(*frame);

    FrameFactory.free(frame);
}

void i2cRegisters::write(uint8_t address, uint8_t data)
{
    switch (address) {
    case 3:
        i2cRegs.txenqueue(data);
        break;

    case 4:
        {
            union Splitter {
                uint16_t address;
                uint8_t  bytes[2];
            } splitter;
            splitter.address = mac.get_address();
            splitter.bytes[0] = data;
            mac.set_address(splitter.address);
        }

    case 5:
        {
            union Splitter {
                uint16_t address;
                uint8_t  bytes[2];
            } splitter;
            splitter.address = mac.get_address();
            splitter.bytes[1] = data;
            mac.set_address(splitter.address);
        }
    case 6: //new secret register: send a garbage packet
        {
            debug("sending random packet!");
            i2cRegs.txenqueue(data);
            if (txq) {
                txq_depth = 0;
            }
            for (uint16_t i = 0; i < sizeof(IRFrame); i++) {
                txenqueue(random(256));
            }
        }

    default:
        break;
    }
}

uint8_t i2cRegisters::read(uint8_t addr)
{
    address = addr;
    return read();
}

uint8_t i2cRegisters::read()
{
    uint8_t ret = 0;
    switch (address) {
    case 0: //rxq depth
        ret = rxq_depth;
        break;

    case 1: //rxq data
        if (rxq_depth > 0 && rxq) {
            ret = ((IRFrame*)rxq)->blob()[rxq_depth - 1];
            rxq_depth--;
            if (rxq_depth == 0) {
                FrameFactory.free((IRFrame*)rxq);
                rxq = 0;
            }
        }
        break;

    case 2: //txq depth:
        ret = txq_depth;
        break;

    case 3: //txq data:
        //reading txq is invalid

    case 4: //address lower byte
        ret = lowByte(mac.get_address());
        break;

    case 5: //address upper byte
        ret = highByte(mac.get_address());
        break;

    case 6: //high water mark
        ret = FrameFactory.get_high_water_mark();
        break;

    default: //ruhoh
        //can't print here b/c in interrupt handler
        break;
    }
    return ret;
}

void i2cRegisters::recvHook(int event, int param)
{
    auto frame = (IRFrame *)param;

    if (!rxq) {
        // if rxq points to something, we have a buffer overrun
        rxq = FrameFactory.alloc();
        IRFrame::copy((IRFrame*)rxq, frame);
        rxq_depth = sizeof(IRFrame);
    }
    //we omit the free of param here b/c the serial one should clean
    //up for us
}

void i2cRegisters::txenqueue(uint8_t data)
{
    if (txq == NULL) {
        txq = FrameFactory.alloc();
    }
    if (txq_depth < sizeof(IRFrame) - 1) {
        ((IRFrame*)txq)->blob()[txq_depth] = data;
        txq_depth++;
    }
    if (txq_depth == sizeof(IRFrame) - 1) {
        debug("filled tx queue.. sending");
        //we just filled the tx queue and are ready to xmit
        IRFrame *frame = FrameFactory.alloc();
        IRFrame::copy(frame, (IRFrame*)txq);

        eventManager.queueEvent(I2CTXEVENT, (int)frame);
    
        FrameFactory.free((IRFrame*)txq);
        txq = 0;
        txq_depth = 0;
    }
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
