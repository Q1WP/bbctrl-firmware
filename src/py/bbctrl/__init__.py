#!/usr/bin/env python3

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


def _parse_args():
    import argparse

    parser = argparse.ArgumentParser(
        description = 'Buildbotics Machine Controller')

    parser.add_argument('-p', '--port', default = 80,
                        type = int, help = 'HTTP port')
    parser.add_argument('-a', '--addr', metavar = 'IP', default = '0.0.0.0',
                        help = 'HTTP address to bind')
    parser.add_argument('-s', '--serial', default = '/dev/ttyAMA0',
                        help = 'Serial device')
    parser.add_argument('-b', '--baud', default = 230400, type = int,
                        help = 'Serial baud rate')
    parser.add_argument('--i2c-port', default = 1, type = int,
                        help = 'I2C port')
    parser.add_argument('--lcd-addr', default = [0x27, 0x3f], type = int,
                        help = 'LCD I2C address')
    parser.add_argument('--avr-addr', default = 0x2b, type = int,
                        help = 'AVR I2C address')
    parser.add_argument('-v', '--verbose', action = 'store_true',
                        help = 'Verbose output')
    parser.add_argument('-l', '--log', metavar = "FILE",
                        help = 'Set a log file')
    parser.add_argument('--disable-camera', action = 'store_true',
                        help = 'Disable the camera')
    parser.add_argument('--width', default = 640, type = int,
                        help = 'Camera width')
    parser.add_argument('--height', default = 480, type = int,
                        help = 'Camera height')
    parser.add_argument('--fps', default = 15, type = int,
                        help = 'Camera frames per second')
    parser.add_argument('--camera-clients', default = 4,
                        help = 'Maximum simultaneous camera clients')
    parser.add_argument('--demo', action = 'store_true',
                        help = 'Enter demo mode')
    parser.add_argument('--debug', default = 0, type = int,
                        help = 'Enable debug mode and set frequency in seconds')
    parser.add_argument('--fast-emu', action = 'store_true',
                        help = 'Enter demo mode')
    parser.add_argument('--client-timeout', default = 5 * 60, type = int,
                        help = 'Demo client timeout in seconds')

    return parser.parse_args()


def run():
    import os
    import tornado
    from . import util
    from .Debugger import Debugger
    from .Web import Web

    args = _parse_args()

    # Create ioloop
    ioloop = tornado.ioloop.IOLoop.current()

    # Debug handler
    if args.debug: Debugger(ioloop, args.debug)

    # Set TPL path
    os.environ['TPL_PATH'] = \
        '/var/lib/bbctrl/upload/lib/:' + util.get_resource('tpl_lib/')

    # Start server
    Web(args, ioloop)
    ioloop.start()


if __name__ == '__main__': run()
