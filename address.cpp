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

#include "address.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>

const PROGMEM char* const magic = "NSA PLAYSET";

AddressStorageClass AddressStorage;

AddressStorageClass::AddressStorageClass()
{
}

void AddressStorageClass::begin()
{
    randomSeed(analogRead(0));
}

void AddressStorageClass::store(uint16_t addresss)
{
    /* if there is no header, put one there */
    if (!present()) {
        write_header();
    }
    const auto offset = strlen_P(magic);
    EEPROM.write(offset+1, lowByte(addresss));
    EEPROM.write(offset+2, highByte(addresss));
}

bool AddressStorageClass::present()
{
    for (size_t i = 0; i < strlen_P(magic); i++) {
        auto b = EEPROM.read(i);
        if (b != pgm_read_byte_near(magic+i)) {
            return false;
        }
    }
    return true;
}

uint16_t AddressStorageClass::load()
{
    union Splitter {
        uint16_t address;
        uint8_t  bytes[2];
    } splitter;
    const auto offset = strlen_P(magic);
    splitter.bytes[0] = EEPROM.read(offset+1);
    splitter.bytes[1] = EEPROM.read(offset+2);
    return splitter.address;
}

uint16_t AddressStorageClass::generate()
{
    return random(4096);
}

void AddressStorageClass::write_header()
{
    for (size_t i = 0; i < strlen_P(magic); i++) {
        EEPROM.write(i, pgm_read_byte_near(magic+i));
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
