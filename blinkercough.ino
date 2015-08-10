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
#include "codec.h"
#include "mac.h"
#include "i2c.h"
#include "util.h"
#include <EventManager.h>
#include <SoftwareSerial.h>
#include <stdint.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <Wire.h>


#define READ_CMD "READ REGISTER "
#define WRITE_CMD "WRITE REGISTER "

//SERIAL PROTOCOL
// gives register interface like i2cregs
//
// PC says: READ_REGISTER <register address>
// BC replies: <register value>
//
// PC says: WRITE_REGISTER <register address> <register value>
// BC replies <nothing>
//
// BC may give debug information with prefix
// DBG: 


EventManager eventManager;


char commandbuf[20];
uint8_t pos = 0;

typedef void (*CB)(int eventCode, int eventParam);

#define IREVENT EventManager::kEventUser0

static void process_command(char i)
{
    if ((i != '\n' && i != '\r') && pos < sizeof(commandbuf)-1) {
        commandbuf[pos] = i;
        pos++;
        return;
    }
    // a whole command is in buffer
    //clear out any remaining input
    while(Serial.available()) {
        (void)Serial.read();
    }

     //debug("cmd buf \"%s\"", commandbuf);
     if (strncmp_P(commandbuf, PSTR(READ_CMD), sizeof(READ_CMD)-1) == 0) {
        int address;

        sscanf_P(commandbuf, PSTR(READ_CMD "%x"), &address);
        auto data = i2cRegs.read(address);
        printf("%02x\n", data);
    } else if (strncmp_P(commandbuf, PSTR(WRITE_CMD), sizeof(WRITE_CMD)-1) == 0) {
        int address;
        int data;
        sscanf_P(commandbuf, PSTR(WRITE_CMD "%x %x"), &address, &data);
        i2cRegs.write(address, data);
    }

    pos = 0;
    memset((void*)commandbuf, 0, sizeof(commandbuf));
    return;
}

static void serialCallback(__attribute__((unused)) int event, char c)
{
    process_command(c);
}
GenericCallable<void(int,int)> serialCB((CB)serialCallback);

static void irSerialCallback(__attribute__((unused)) int event, char c)
{
    mac.recv(c);
}
GenericCallable<void(int,int)> irSerialCB((CB)irSerialCallback);

static void FrameCallback(int event, IRFrame *frame)
{
    if (event == BlinkerMac::ValidFrameRecievedEvent) {
        debugstart("packet from: 0x%04x to: ", frame->source);
        if (frame->destination == mac.get_address()) {
            debugcont("ME");
        } else {
            debugcont("0x4x", frame->destination);
        }
        debugend();

        FrameFactory.free(frame);
    } else if (event == BlinkerMac::InvalidFrameRecievedEvent) {
            debug("Invalid packet recieved!");
            IRFrame::hexdump(frame);

            FrameFactory.free(frame);
    }
}
GenericCallable<void(int,int)> frameCB((CB)FrameCallback);




void setup()
{
    pinMode(13, OUTPUT);


    Serial.begin(115200);
    init_debug();

    randomSeed(analogRead(0));
    mac.set_address(random(4096));
    debug("This station's address is 0x%04x", mac.get_address());

    mac.set_baud_invert(IR_BAUD, false);

    eventManager.addListener(EventManager::kEventSerial,
                             &serialCB);
    eventManager.addListener(IREVENT,
                             &irSerialCB);

    eventManager.addListener(BlinkerMac::ValidFrameRecievedEvent,
                             &frameCB);
 
    eventManager.addListener(BlinkerMac::InvalidFrameRecievedEvent,
                             &frameCB);


    FrameFactory.print_high_water_mark();

    i2cRegs.begin();
    mac.begin();


    debug("Starting up...\n");
}

void loop()
{
    static auto last = millis();
    auto now = millis();
    if (now - last > 30000) {
        last = now;
        FrameFactory.print_high_water_mark();
    }

    if (Serial.available()) {
        char c = Serial.read();
        eventManager.queueEvent(EventManager::kEventSerial, c);
    }

    //FIXME: encapsulate this in RX
    // and abstract with method
    if (mac.serial.available()) {
        const uint8_t c = mac.serial.read();
        eventManager.queueEvent(IREVENT, c);
    }
    eventManager.processEvent();
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
