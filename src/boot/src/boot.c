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

#include "boot.h"
#include "sp_driver.h"

#include <util/delay.h>
#include <util/crc16.h>

#include <avr/eeprom.h>
#include <avr/io.h>

#include <stdbool.h>


uint8_t buffer[SPM_PAGESIZE];
uint16_t block_crc = 0;


void clock_init() {
  // External 16Mhz Xtal w/ 2x PLL = 32 Mhz
  // 12-16 MHz crystal; 0.4-16 MHz XTAL w/ 16K CLK startup
  OSC.XOSCCTRL = OSC_FRQRANGE_12TO16_gc | OSC_XOSCSEL_XTAL_16KCLK_gc;
  OSC.CTRL = OSC_XOSCEN_bm;                // enable external crystal oscillator
  while (!(OSC.STATUS & OSC_XOSCRDY_bm));  // wait for oscillator ready

  OSC.PLLCTRL = OSC_PLLSRC_XOSC_gc | 2;    // PLL source, 2x (32 MHz sys clock)
  OSC.CTRL = OSC_PLLEN_bm | OSC_XOSCEN_bm; // Enable PLL & External Oscillator
  while (!(OSC.STATUS & OSC_PLLRDY_bm));   // wait for PLL ready

  CCP = CCP_IOREG_gc;
  CLK.CTRL = CLK_SCLKSEL_PLL_gc;           // switch to PLL clock
  OSC.CTRL &= ~OSC_RC2MEN_bm;              // disable internal 2 MHz clock
}


bool uart_has_char() {return UART_DEVICE.STATUS & USART_RXCIF_bm;}
uint8_t uart_recv_char() {return UART_DEVICE.DATA;}


void uart_send_char_blocking(uint8_t c) {
  UART_DEVICE.DATA = c;
  while (!(UART_DEVICE.STATUS & USART_TXCIF_bm)) continue;
  UART_DEVICE.STATUS |= USART_TXCIF_bm;
}


void uart_init() {
  UART_PORT.DIRSET = 1 << UART_TX_PIN;
  UART_DEVICE.BAUDCTRLA = UART_BSEL_VALUE & USART_BSEL_gm;
  UART_DEVICE.BAUDCTRLB =
    ((UART_BSCALE_VALUE << USART_BSCALE_gp) & USART_BSCALE_gm) |
    ((UART_BSEL_VALUE >> 8) & ~USART_BSCALE_gm);
  UART_DEVICE.CTRLB = USART_RXEN_bm | USART_TXEN_bm;

  PORTC.OUTCLR = 1 << 4; // CTS Lo (enable)
  PORTC.DIRSET = 1 << 4; // CTS Output
}


void uart_deinit() {
  UART_DEVICE.CTRLB = 0;
  UART_DEVICE.BAUDCTRLA = 0;
  UART_DEVICE.BAUDCTRLB = 0;
  UART_PORT.DIRCLR = 1 << UART_TX_PIN;
}


void watchdog_init() {
  uint8_t temp = WDT_ENABLE_bm | WDT_CEN_bm | WDT_PER_4KCLK_gc;
  CCP = CCP_IOREG_gc;
  WDT.CTRL = temp;
  while (WDT.STATUS & WDT_SYNCBUSY_bm) continue;
}


void watchdog_reset() {asm("wdr");}


void watchdog_disable() {
  uint8_t temp = (WDT.CTRL & ~WDT_ENABLE_bm) | WDT_CEN_bm;
  CCP = CCP_IOREG_gc;
  WDT.CTRL = temp;
}


void nvm_wait() {while (NVM_STATUS & NVM_NVMBUSY_bp) watchdog_reset();}


void nvm_exec() {
  void *z = (void *)&NVM_CTRLA;

  __asm__ volatile
    ("out %[ccp], %[ioreg]\n"
     "st z, %[cmdex]" ::
     [ccp] "I" (_SFR_IO_ADDR(CCP)),
     [ioreg] "d" (CCP_IOREG_gc),
     [cmdex] "r" (NVM_CMDEX_bm),
     [z] "z" (z));
}


uint8_t get_char() {
  while (!uart_has_char()) continue;
  return uart_recv_char();
}


void send_char(uint8_t c) {uart_send_char_blocking(c);}


uint16_t get_word() {
  uint8_t hi = get_char();
  uint8_t lo = get_char();
  return ((uint16_t)hi << 8) | lo;
}


uint8_t BlockLoad(unsigned size, uint8_t mem, uint32_t *address) {
  watchdog_reset();

  // fill up buffer
  block_crc = 0xffff;
  for (int i = 0; i < SPM_PAGESIZE; i++)
    if (i < size) {
      buffer[i] = get_char();
      block_crc = _crc16_update(block_crc, buffer[i]);

    } else buffer[i] = 0xff;

  switch (mem) {
  case MEM_EEPROM:
    eeprom_write_block(buffer, (uint8_t *)(uint16_t)*address, size);
    *address += size;
    break;

  case MEM_FLASH:
    // NOTE: Flash programming, address is given in words.
    SP_LoadFlashPage(buffer);
    SP_EraseWriteApplicationPage(*address << 1);
    *address += size >> 1;
    nvm_wait();
    break;

  case MEM_USERSIG:
    SP_LoadFlashPage(buffer);
    SP_EraseUserSignatureRow();
    nvm_wait();
    SP_WriteUserSignatureRow();
    nvm_wait();
    break;

  default: return REPLY_ERROR;
  }

  return REPLY_ACK;
}



void BlockRead(unsigned size, uint8_t mem, uint32_t *address) {
  switch (mem) {
  case MEM_EEPROM:
    eeprom_read_block(buffer, (uint8_t *)(uint16_t)*address, size);
    *address += size;

    // send bytes
    for (int i = 0; i < size; i++)
      send_char(buffer[i]);
    break;

  case MEM_FLASH: case MEM_USERSIG: case MEM_PRODSIG: {
    *address <<= 1; // Convert address to bytes temporarily

    do {
      switch (mem) {
      case MEM_FLASH: send_char(SP_ReadByte(*address)); break;
      case MEM_USERSIG: send_char(SP_ReadUserSignatureByte(*address)); break;
      case MEM_PRODSIG: send_char(SP_ReadCalibrationByte(*address)); break;
      }

      nvm_wait();

      (*address)++;    // Select next word in memory.
      size--;          // Subtract two bytes from number of bytes to read
    } while (size);    // Repeat until all block has been read

    *address >>= 1;    // Convert address back to Flash words again.
    break;
  }

  default: break;
  }
}


void bootloader() {
  uint32_t address = 0;
  uint16_t i = 0;

  while (true) {
    uint8_t val = get_char();
    watchdog_reset();

    // Main bootloader parser
    switch (val) {
    case CMD_CHECK_AUTOINCREMENT: send_char(REPLY_YES); break;

    case CMD_SET_ADDRESS:
      address = get_word();
      send_char(REPLY_ACK);
      break;

    case CMD_SET_EXT_ADDRESS: {
      uint8_t hi = get_char();
      address = ((uint32_t)hi << 16) | get_word();
      send_char(REPLY_ACK);
      break;
    }

    case CMD_FLASH_ERASE:
      SP_EraseApplicationSection();
      nvm_wait();
      send_char(REPLY_ACK);
      break;

    case CMD_EEPROM_ERASE:
      NVM.CMD = NVM_CMD_ERASE_EEPROM_gc;
      nvm_exec();
      send_char(REPLY_ACK);
      break;

    case CMD_CHECK_BLOCK_SUPPORT:
      send_char(REPLY_YES);
      // Send block size (page size)
      send_char(SPM_PAGESIZE >> 8);
      send_char((uint8_t)SPM_PAGESIZE);
      break;

    case CMD_BLOCK_LOAD:
      i = get_word(); // Block size
      val = get_char(); // Memory type
      send_char(BlockLoad(i, val, &address)); // Load it
      break;

    case CMD_BLOCK_READ:
      i = get_word(); // Block size
      val = get_char(); // Memory type
      BlockRead(i, val, &address); // Read it
      break;

    case CMD_READ_BYTE: {
      unsigned w = SP_ReadWord(address << 1);

      send_char(w >> 8);
      send_char(w);

      address++;
      break;
    }

    case CMD_WRITE_LOW_BYTE:
      i = get_char(); // get low byte
      send_char(REPLY_ACK);
      break;

    case CMD_WRITE_HIGH_BYTE:
      i |= (get_char() << 8); // get high byte; combine
      SP_LoadFlashWord(address << 1, i);
      address++;
      send_char(REPLY_ACK);
      break;

    case CMD_WRITE_PAGE:
      if (address >= (APP_SECTION_SIZE >> 1))
        send_char(REPLY_ERROR); // don't allow bootloader overwrite

      else {
        SP_WriteApplicationPage(address << 1);
        send_char(REPLY_ACK);
      }
      break;

    case CMD_WRITE_EEPROM_BYTE:
      eeprom_write_byte((uint8_t *)(uint16_t)address, get_char());
      address++;
      send_char(REPLY_ACK);
      break;

    case CMD_READ_EEPROM_BYTE:
      send_char(eeprom_read_byte((uint8_t *)(uint16_t)address));
      address++;
      break;

    case CMD_READ_LOW_FUSE_BITS: send_char(SP_ReadFuseByte(0)); break;
    case CMD_READ_HIGH_FUSE_BITS: send_char(SP_ReadFuseByte(1)); break;
    case CMD_READ_EXT_FUSE_BITS: send_char(SP_ReadFuseByte(2)); break;

    case CMD_ENTER_PROG_MODE: case CMD_LEAVE_PROG_MODE:
      send_char(REPLY_ACK);
      break;

    case CMD_EXIT_BOOTLOADER:
      send_char(REPLY_ACK);
      return;
      break;

    case CMD_PROGRAMMER_TYPE: send_char('S'); break; // serial

    case CMD_DEVICE_CODE:
      send_char(123); // send only this device
      send_char(0); // terminator
      break;

    case CMD_SET_LED: case CMD_CLEAR_LED: case CMD_SET_TYPE:
      get_char(); // discard parameter
      send_char(REPLY_ACK);
      break;

    case CMD_PROGRAM_ID:
      send_char('b');
      send_char('b');
      send_char('c');
      send_char('t');
      send_char('r');
      send_char('l');
      send_char(' ');
      break;

    case CMD_VERSION:
      send_char('0');
      send_char('2');
      break;

    case CMD_READ_SIGNATURE:
      send_char(SIGNATURE_2);
      send_char(SIGNATURE_1);
      send_char(SIGNATURE_0);
      break;

    case CMD_READ_CHECKSUM:
      // Setup
      nvm_wait();

      // Reset CRC
      CRC_CTRL |= CRC_RESET_RESET1_gc;
      CRC.CHECKSUM0 = CRC.CHECKSUM1 = CRC.CHECKSUM2 = CRC.CHECKSUM3 = 0xff;

      // 32-bit mode, flash source
      CRC_CTRL = CRC_CRC32_bm | CRC_SOURCE_FLASH_gc;

      // Start address
      NVM.ADDR0 = (uint8_t)(APP_SECTION_START >> 0);
      NVM.ADDR1 = (uint8_t)(APP_SECTION_START >> 8);
      NVM.ADDR2 = 0;

      // End address
      NVM.DATA0 = (uint8_t)(APP_SECTION_END >> 0);
      NVM.DATA1 = (uint8_t)(APP_SECTION_END >> 8);
      NVM.DATA2 = (uint8_t)(APP_SECTION_END >> 16);

      NVM.CMD = NVM_CMD_FLASH_RANGE_CRC_gc;
      CCP = CCP_IOREG_gc;
      NVM.CTRLA = NVM_CMDEX_bm;

      // Compute
      nvm_wait();
      while (CRC.STATUS & CRC_BUSY_bm) continue;

      // Send 32-bit checksum
      send_char(CRC.CHECKSUM3);
      send_char(CRC.CHECKSUM2);
      send_char(CRC.CHECKSUM1);
      send_char(CRC.CHECKSUM0);
      break;

    case CMD_FLASH_LENGTH:
      send_char((uint8_t)(APP_SECTION_SIZE >> 16));
      send_char((uint8_t)(APP_SECTION_SIZE >> 8));
      send_char((uint8_t)(APP_SECTION_SIZE >> 0));
      break;

    case CMD_BLOCK_CRC:
      send_char(block_crc >> 8);
      send_char((uint8_t)block_crc);
      break;

    case CMD_SYNC: break; // ESC (0x1b) to sync

    default: // otherwise, error
      send_char(REPLY_ERROR);
      break;
    }

    // Wait for any lingering SPM instructions to finish
    nvm_wait();
  }
}


int main() {
  // Init
  clock_init();
  uart_init();
  watchdog_init();

  // Check for trigger
  uint16_t j = INITIAL_WAIT;
  while (0 < j--) {
    if (uart_has_char() && uart_recv_char() == CMD_SYNC) {
      bootloader();
      break;
    }

    watchdog_reset();
    _delay_ms(1);
  }

  // Deinit
  uart_deinit();
  watchdog_disable();

  // Disable further self programming until next reset
  SP_LockSPM();

  // Jump to application code
  asm("jmp 0");
}
