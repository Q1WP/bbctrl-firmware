################################################################################
#                                                                              #
#                 This file is part of the Buildbotics firmware.               #
#                                                                              #
#        Copyright (c) 2015 - 2021, Buildbotics LLC, All rights reserved.      #
#                                                                              #
#         This Source describes Open Hardware and is licensed under the        #
#                                 CERN-OHL-S v2.                               #
#                                                                              #
#         You may redistribute and modify this Source and make products        #
#    using it under the terms of the CERN-OHL-S v2 (https:/cern.ch/cern-ohl).  #
#           This Source is distributed WITHOUT ANY EXPRESS OR IMPLIED          #
#    WARRANTY, INCLUDING OF MERCHANTABILITY, SATISFACTORY QUALITY AND FITNESS  #
#     FOR A PARTICULAR PURPOSE. Please see the CERN-OHL-S v2 for applicable    #
#                                  conditions.                                 #
#                                                                              #
#                Source location: https://github.com/buildbotics               #
#                                                                              #
#      As per CERN-OHL-S v2 section 4, should You produce hardware based on    #
#    these sources, You must maintain the Source Location clearly visible on   #
#    the external case of the CNC Controller or other product you make using   #
#                                  this Source.                                #
#                                                                              #
#                For more information, email info@buildbotics.com              #
#                                                                              #
################################################################################

import serial
import time
import traceback
import ctypes
import math

__all__ = ['AVR']


class _serial_struct(ctypes.Structure):
    _fields_ = [
        ('type',            ctypes.c_int),
        ('line',            ctypes.c_int),
        ('port',            ctypes.c_uint),
        ('irq',             ctypes.c_int),
        ('flags',           ctypes.c_int),
        ('xmit_fifo_size',  ctypes.c_int),
        ('custom_divisor',  ctypes.c_int),
        ('baud_base',       ctypes.c_int),
        ('close_delay',     ctypes.c_ushort),
        ('io_type',         ctypes.c_byte),
        ('reserved',        ctypes.c_byte),
        ('hub6',            ctypes.c_int),
        ('closing_wait',    ctypes.c_ushort),
        ('closing_wait2',   ctypes.c_ushort),
        ('iomem_base',      ctypes.c_char_p),
        ('iomem_reg_shift', ctypes.c_ushort),
        ('port_high',       ctypes.c_uint),
        ('iomap_base',      ctypes.c_ulong),
    ]


def _serial_set_low_latency(sp):
    import fcntl
    import termios

    ASYNCB_LOW_LATENCY = 13

    ss = _serial_struct()
    fcntl.ioctl(sp, termios.TIOCGSERIAL, ss)
    ss.flags |= 1 << ASYNCB_LOW_LATENCY # pylint: disable=no-member
    fcntl.ioctl(sp, termios.TIOCSSERIAL, ss)



class AVR(object):
    def __init__(self, ctrl):
        self.ctrl     = ctrl
        self.log      = ctrl.log.get('AVR')
        self.sp       = None
        self.i2c_addr = ctrl.args.avr_addr
        self.read_cb  = None
        self.write_cb = None
        self.events   = 0
        self.errors   = 0


    def close(self): pass


    def flush_output(self): self.sp.reset_output_buffer()


    def _start(self):
        try:
            self.sp = serial.Serial(self.ctrl.args.serial, self.ctrl.args.baud,
                                    rtscts = 1, timeout = 0, write_timeout = 0)
            self.sp.nonblocking()
            #_serial_set_low_latency(self.sp)

            self.ctrl.ioloop.add_handler(self.sp, self._serial_handler, 0)
            self.enable_read(True)

        except Exception as e:
            self.sp = None
            self.log.warning('Failed to open serial port: %s', e)


    def set_handlers(self, read_cb, write_cb):
        if self.read_cb is not None or self.write_cb is not None:
            raise Exception('Handlers already set')

        self.read_cb  = read_cb
        self.write_cb = write_cb
        self._start()


    def update_events(self, events, enable):
        if self.sp is None: return

        if enable: self.events |= events
        else: self.events &= ~events

        self.ctrl.ioloop.update_handler(self.sp, self.events)


    def enable_write(self, enable):
        self.update_events(self.ctrl.ioloop.WRITE, enable)


    def enable_read(self, enable):
        self.update_events(self.ctrl.ioloop.READ, enable)


    def _serial_handler(self, fd, events):
        try:
            if self.ctrl.ioloop.READ & events:
                self.read_cb(self.sp.read(self.sp.in_waiting))

            if self.ctrl.ioloop.WRITE & events:
                self.write_cb(lambda data: self.sp.write(data))

            self.errors = 0

        except Exception as e:
            self.log.warning('Serial: %s', e)

            # Delay next IO
            self.errors += 1
            delay = 0.1 * math.pow(2, max(6, self.errors))

            events = self.events
            self.update_events(events, False)

            self.ctrl.ioloop.call_later(delay, self.update_events, events, True)


    def i2c_command(self, cmd, byte = None, word = None, block = None):
        self.log.info('I2C: %s b=%s w=%s d=%s' % (cmd, byte, word, block))
        retry = 10
        cmd = ord(cmd[0])

        while True:
            try:
                self.ctrl.i2c.write(self.i2c_addr, cmd, byte, word, block)
                break

            except Exception as e:
                retry -= 1

                if retry:
                    self.log.warning('I2C failed, retrying: %s' % e)
                    time.sleep(0.25)
                    continue

                else:
                    self.log.error('I2C failed: %s' % e)
                    raise
