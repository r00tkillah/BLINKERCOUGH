#
# BLINKERCOUGH: NSA Playset implant for IR bridging of airgap
#
# Copyright (C) 2015  Hacker, J.R. <r00tkillah@gmail.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

ARDUINO_DIR    = /usr/share/arduino
ARDMK_DIR      = Arduino-Makefile
AVR_TOOLS_DIR  = /usr

USER_LIB_PATH  = libraries

TARGET         = blinkercough
ARDUINO_LIBS   = SoftwareSerial EventManager/EventManager Wire EEPROM

ISP_PROG = usbasp
BOARD_TAG = nano328
ARDUINO_PORT = /dev/ttyUSB0
MONITOR_BAUDRATE = 115200
CXXFLAGS_STD = -std=c++11

include Arduino-Makefile/Arduino.mk
