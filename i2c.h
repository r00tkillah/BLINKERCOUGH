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

#ifndef _I2C_H
#define _I2C_H

#include "irframe.h"
#include <Wire.h>

#define SLAVE_ADDRESS 0x25

class i2cRegisters
{
public:
    i2cRegisters();

    void begin();  //to be called in setup()

    //to be called when a packet is received
    //public b/c of declaration of hook in global space
    void recvHook(int event, int param);

    //write a "register" to be called from i2cReceiveCallback
    void write(uint8_t address, uint8_t data);

    //read a register specified at address
    uint8_t read(uint8_t addr);

private:
    //read a "register" specified in address to be called form
    //i2cRequestCallback
    uint8_t read();

    //enqueue a byte into the transmit buffer
    void txenqueue(uint8_t data);


    volatile uint8_t address; //stores address of read operation

    //rx is for packets to PC
    volatile uint16_t rxq_depth;
    volatile IRFrame *rxq;

    //tx is for packets from PC
    volatile uint16_t txq_depth;
    volatile IRFrame *txq;


    friend void xmit(int event, int param); //internal event handler to xmit packet
    friend void i2cReceiveCallback(int count);
    friend void i2cRequestCallback(void);
};

extern i2cRegisters i2cRegs;


#endif /* _I2C_H */

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
