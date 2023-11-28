/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2023, Buildbotics LLC, All rights reserved.

          This Source describes Open Hardware and is licensed under the
                                  CERN-OHL-S v2.

          You may redistribute and modify this Source and make products
     using it under the terms of the CERN-OHL-S v2 (https:/cern.ch/cern-ohl).
            This Source is distributed WITHOUT ANY EXPRESS OR IMPLIED
     WARRANTY, INCLUDING OF MERCHANTABILITY, SATISFACTORY QUALITY AND FITNESS
      FOR A PARTICULAR PURPOSE. Please see the CERN-OHL-S v2 for applicable
                                   conditions.

                 Source location: https://github.com/buildbotics

       As per CERN-OHL-S v2 section 4, should You produce hardware based on
     these sources, You must maintain the Source Location clearly visible on
     the external case of the CNC Controller or other product you make using
                                   this Source.

                 For more information, email info@buildbotics.com

\******************************************************************************/

#pragma once

#include "pgmspace.h"

#include <stdint.h>


// Control flags
enum {
  REG_SELECT_BIT          = 1 << 0,
  READ_BIT                = 1 << 1,
  ENABLE_BIT              = 1 << 2,
  BACKLIGHT_BIT           = 1 << 3,
};


// Commands
enum {
  LCD_CLEAR_DISPLAY       = 1 << 0,
  LCD_RETURN_HOME         = 1 << 1,
  LCD_ENTRY_MODE_SET      = 1 << 2,
  LCD_DISPLAY_CONTROL     = 1 << 3,
  LCD_CURSOR_SHIFT        = 1 << 4,
  LCD_FUNCTION_SET        = 1 << 5,
  LCD_SET_CGRAM_ADDR      = 1 << 6,
  LCD_SET_DDRAM_ADDR      = 1 << 7,
};


// Entry Mode Set flags
#define LCD_ENTRY_SHIFT_DISPLAY (1 << 0)
#define LCD_ENTRY_SHIFT_INC     (1 << 1)
#define LCD_ENTRY_SHIFT_DEC     (0 << 1)


// Display Control flags
#define LCD_BLINK_ON            (1 << 0)
#define LCD_BLINK_OFF           (0 << 0)
#define LCD_CURSOR_ON           (1 << 1)
#define LCD_CURSOR_OFF          (0 << 1)
#define LCD_DISPLAY_ON          (1 << 2)
#define LCD_DISPLAY_OFF         (0 << 2)


// Cursor Shift flags
#define LCD_SHIFT_RIGHT         (1 << 2)
#define LCD_SHIFT_LEFT          (0 << 2)
#define LCD_SHIFT_DISPLAY       (1 << 3)
#define LCD_SHIFT_CURSOR        (0 << 3)


// Function Set flags
#define LCD_5x11_DOTS           (1 << 2)
#define LCD_5x8_DOTS            (0 << 2)
#define LCD_2_LINE              (1 << 3)
#define LCD_1_LINE              (0 << 3)
#define LCD_8_BIT_MODE          (1 << 4)
#define LCD_4_BIT_MODE          (0 << 4)


// Text justification flags
enum {
  JUSTIFY_LEFT            = 0,
  JUSTIFY_RIGHT           = 1,
  JUSTIFY_CENTER          = 2,
};


void lcd_init(uint8_t addr);
void lcd_nibble(uint8_t addr, uint8_t data);
void lcd_write(uint8_t addr, uint8_t cmd, uint8_t flags);
void lcd_goto(uint8_t addr, uint8_t x, uint8_t y);
void lcd_putchar(uint8_t addr, uint8_t c);
void lcd_pgmstr(uint8_t addr, const char *s);
void lcd_splash();
void lcd_rtc_callback();
