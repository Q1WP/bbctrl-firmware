################################################################################
#                                                                              #
#                 This file is part of the Buildbotics firmware.               #
#                                                                              #
#        Copyright (c) 2015 - 2023, Buildbotics LLC, All rights reserved.      #
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
import time
import errno

try:
    try:
        import smbus
    except:
        import smbus2 as smbus
except:
    smbus = None

__all__ = ['OLED']

fontTable = {" ": [0x00, 0x00, 0x00, 0x00, 0x00],
             "!": [0x00, 0x00, 0x2f, 0x00, 0x00],
             '"': [0x00, 0x07, 0x00, 0x07, 0x00],
             "#": [0x14, 0x7f, 0x14, 0x7f, 0x14],
             "$": [0x24, 0x2a, 0x7f, 0x2a, 0x12],
             "%": [0x23, 0x13, 0x08, 0x64, 0x62],
             "&": [0x36, 0x49, 0x55, 0x22, 0x50],
             "'": [0x00, 0x05, 0x03, 0x00, 0x00],
             "(": [0x00, 0x1c, 0x22, 0x41, 0x00],
             ")": [0x00, 0x41, 0x22, 0x1c, 0x00],
             "*": [0x14, 0x08, 0x3E, 0x08, 0x14],
             "+": [0x08, 0x08, 0x3E, 0x08, 0x08],
             ",": [0x00, 0x00, 0xA0, 0x60, 0x00],
             "-": [0x08, 0x08, 0x08, 0x08, 0x08],
             ".": [0x00, 0x60, 0x60, 0x00, 0x00],
             "/": [0x20, 0x10, 0x08, 0x04, 0x02],
             "0": [0x3E, 0x51, 0x49, 0x45, 0x3E],
             "1": [0x00, 0x42, 0x7F, 0x40, 0x00],
             "2": [0x42, 0x61, 0x51, 0x49, 0x46],
             "3": [0x21, 0x41, 0x45, 0x4B, 0x31],
             "4": [0x18, 0x14, 0x12, 0x7F, 0x10],
             "5": [0x27, 0x45, 0x45, 0x45, 0x39],
             "6": [0x3C, 0x4A, 0x49, 0x49, 0x30],
             "7": [0x01, 0x71, 0x09, 0x05, 0x03],
             "8": [0x36, 0x49, 0x49, 0x49, 0x36],
             "9": [0x06, 0x49, 0x49, 0x29, 0x1E],
             ":": [0x00, 0x36, 0x36, 0x00, 0x00],
             ";": [0x00, 0x56, 0x36, 0x00, 0x00],
             "<": [0x08, 0x14, 0x22, 0x41, 0x00],
             "=": [0x14, 0x14, 0x14, 0x14, 0x14],
             ">": [0x00, 0x41, 0x22, 0x14, 0x08],
             "?": [0x02, 0x01, 0x51, 0x09, 0x06],
             "@": [0x32, 0x49, 0x59, 0x51, 0x3E],
             "A": [0x7C, 0x12, 0x11, 0x12, 0x7C],
             "B": [0x7F, 0x49, 0x49, 0x49, 0x36],
             "C": [0x3E, 0x41, 0x41, 0x41, 0x22],
             "D": [0x7F, 0x41, 0x41, 0x22, 0x1C],
             "E": [0x7F, 0x49, 0x49, 0x49, 0x41],
             "F": [0x7F, 0x09, 0x09, 0x09, 0x01],
             "G": [0x3E, 0x41, 0x49, 0x49, 0x7A],
             "H": [0x7F, 0x08, 0x08, 0x08, 0x7F],
             "I": [0x00, 0x41, 0x7F, 0x41, 0x00],
             "J": [0x20, 0x40, 0x41, 0x3F, 0x01],
             "K": [0x7F, 0x08, 0x14, 0x22, 0x41],
             "L": [0x7F, 0x40, 0x40, 0x40, 0x40],
             "M": [0x7F, 0x02, 0x0C, 0x02, 0x7F],
             "N": [0x7F, 0x04, 0x08, 0x10, 0x7F],
             "O": [0x3E, 0x41, 0x41, 0x41, 0x3E],
             "P": [0x7F, 0x09, 0x09, 0x09, 0x06],
             "Q": [0x3E, 0x41, 0x51, 0x21, 0x5E],
             "R": [0x7F, 0x09, 0x19, 0x29, 0x46],
             "S": [0x46, 0x49, 0x49, 0x49, 0x31],
             "T": [0x01, 0x01, 0x7F, 0x01, 0x01],
             "U": [0x3F, 0x40, 0x40, 0x40, 0x3F],
             "V": [0x1F, 0x20, 0x40, 0x20, 0x1F],
             "W": [0x3F, 0x40, 0x38, 0x40, 0x3F],
             "X": [0x63, 0x14, 0x08, 0x14, 0x63],
             "Y": [0x07, 0x08, 0x70, 0x08, 0x07],
             "Z": [0x61, 0x51, 0x49, 0x45, 0x43],
             "[": [0x00, 0x7F, 0x41, 0x41, 0x00],
             "\\": [0x55, 0xAA, 0x55, 0xAA, 0x55],
             "]": [0x00, 0x41, 0x41, 0x7F, 0x00],
             "^": [0x04, 0x02, 0x01, 0x02, 0x04],
             "_": [0x40, 0x40, 0x40, 0x40, 0x40],
             "`": [0x00, 0x03, 0x05, 0x00, 0x00],
             "a": [0x20, 0x54, 0x54, 0x54, 0x78],
             "b": [0x7F, 0x48, 0x44, 0x44, 0x38],
             "c": [0x38, 0x44, 0x44, 0x44, 0x20],
             "d": [0x38, 0x44, 0x44, 0x48, 0x7F],
             "e": [0x38, 0x54, 0x54, 0x54, 0x18],
             "f": [0x08, 0x7E, 0x09, 0x01, 0x02],
             "g": [0x18, 0xA4, 0xA4, 0xA4, 0x7C],
             "h": [0x7F, 0x08, 0x04, 0x04, 0x78],
             "i": [0x00, 0x44, 0x7D, 0x40, 0x00],
             "j": [0x40, 0x80, 0x84, 0x7D, 0x00],
             "k": [0x7F, 0x10, 0x28, 0x44, 0x00],
             "l": [0x00, 0x41, 0x7F, 0x40, 0x00],
             "m": [0x7C, 0x04, 0x18, 0x04, 0x78],
             "n": [0x7C, 0x08, 0x04, 0x04, 0x78],
             "o": [0x38, 0x44, 0x44, 0x44, 0x38],
             "p": [0xFC, 0x24, 0x24, 0x24, 0x18],
             "q": [0x18, 0x24, 0x24, 0x18, 0xFC],
             "r": [0x7C, 0x08, 0x04, 0x04, 0x08],
             "s": [0x48, 0x54, 0x54, 0x54, 0x20],
             "t": [0x04, 0x3F, 0x44, 0x40, 0x20],
             "u": [0x3C, 0x40, 0x40, 0x20, 0x7C],
             "v": [0x1C, 0x20, 0x40, 0x20, 0x1C],
             "w": [0x3C, 0x40, 0x30, 0x40, 0x3C],
             "x": [0x44, 0x28, 0x10, 0x28, 0x44],
             "y": [0x1C, 0xA0, 0xA0, 0xA0, 0x7C],
             "z": [0x44, 0x64, 0x54, 0x4C, 0x44],
             "{": [0x00, 0x10, 0x7C, 0x82, 0x00],
             "|": [0x00, 0x00, 0xFF, 0x00, 0x00],
             "}": [0x00, 0x82, 0x7C, 0x10, 0x00],
             "~": [0x00, 0x06, 0x09, 0x09, 0x06]  #degrees symbol
            }

class OLED:
    def __init__(self,ctrl):
        self.ctrl = ctrl
        self.log = ctrl.log.get('oled')
        self.i2c_bus = None
        self.ft = fontTable
        block = [0xae, 0xd5, 0x80, 0xa8, 0x3f, 0xd3, 0x00, 0x40 | 0,
                 0x8d, 0x14, 0x20, 0x00, 0xa0 | 1, 0xc8, 0xda, 0x12, 0x81,
                 0x80, 0xd9, 0xf1, 0xdb, 0x20, 0xa4, 0xa6, 0xaf]
        self.connect()
        self.i2c_bus.write_i2c_block_data(0x3c, 0, block)
        time.sleep(.0001)
        self.clear_block = []
        i = 0
        while i < 32:
            self.clear_block.append(0x00)
            i = i + 1
        self.oled_clear();

    def connect(self):
        if self.i2c_bus is None:
            self.i2c_bus = smbus.SMBus(1)

    def gotorowcol(self,row,col):
        block = [0x21,col,0x7f,0x22,row,0x7]
        self.i2c_bus.write_i2c_block_data(0x3c, 0, block)
        time.sleep(.0001)

    def oled_clear(self):
        self.connect()
        self.gotorowcol(0,0)
        i = 0
        while i < 32:
            self.i2c_bus.write_i2c_block_data(0x3c, 0x40, self.clear_block)
            time.sleep(0.0001)
            i = i + 1

    def writeStringAt(self,s,row,col):
        self.connect()
        self.gotorowcol(row,col)
        i = 0
        while (i < len(s)):
            d = self.ft[s[i]]
            block = []
            j = 0
            while j < 5:
                block.append(d[j])
                j = j + 1
            i = i + 1
            self.i2c_bus.write_i2c_block_data(0x3c, 0x40, block)

    def writePage(self,data):
        self.connect()
        i = 0
        while i < len(data[i]):
            s = ""
            j = 0
            while j < len(data):
                s = s + data[j][i]
                j = j + 1
            self.writeStringAt(s,2*i + 1,5)
            i = i + 1

    def get(self,c): return fontTable[c]

