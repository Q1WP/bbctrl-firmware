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


#include "config.h"
#include "status.h"
#include "motor.h"

#include <stdint.h>
#include <stdbool.h>

enum {
  DRV8711_CTRL_REG,
  DRV8711_TORQUE_REG,
  DRV8711_OFF_REG,
  DRV8711_BLANK_REG,
  DRV8711_DECAY_REG,
  DRV8711_STALL_REG,
  DRV8711_DRIVE_REG,
  DRV8711_STATUS_REG,
};


enum {
  DRV8711_CTRL_ENBL_bm            = 1 << 0,
  DRV8711_CTRL_RDIR_bm            = 1 << 1,
  DRV8711_CTRL_RSTEP_bm           = 1 << 2,
  DRV8711_CTRL_MODE_1             = 0 << 3,
  DRV8711_CTRL_MODE_2             = 1 << 3,
  DRV8711_CTRL_MODE_4             = 2 << 3,
  DRV8711_CTRL_MODE_8             = 3 << 3,
  DRV8711_CTRL_MODE_16            = 4 << 3,
  DRV8711_CTRL_MODE_32            = 5 << 3,
  DRV8711_CTRL_MODE_64            = 6 << 3,
  DRV8711_CTRL_MODE_128           = 7 << 3,
  DRV8711_CTRL_MODE_256           = 8 << 3,
  DRV8711_CTRL_EXSTALL_bm         = 1 << 7,
  DRV8711_CTRL_ISGAIN_5           = 0 << 8,
  DRV8711_CTRL_ISGAIN_10          = 1 << 8,
  DRV8711_CTRL_ISGAIN_20          = 2 << 8,
  DRV8711_CTRL_ISGAIN_40          = 3 << 8,
  DRV8711_CTRL_DTIME_400          = 0 << 10,
  DRV8711_CTRL_DTIME_450          = 1 << 10,
  DRV8711_CTRL_DTIME_650          = 2 << 10,
  DRV8711_CTRL_DTIME_850          = 3 << 10,
};


enum {
  DRV8711_TORQUE_SMPLTH_50        = 0 << 8,
  DRV8711_TORQUE_SMPLTH_100       = 1 << 8,
  DRV8711_TORQUE_SMPLTH_200       = 2 << 8,
  DRV8711_TORQUE_SMPLTH_300       = 3 << 8,
  DRV8711_TORQUE_SMPLTH_400       = 4 << 8,
  DRV8711_TORQUE_SMPLTH_600       = 5 << 8,
  DRV8711_TORQUE_SMPLTH_800       = 6 << 8,
  DRV8711_TORQUE_SMPLTH_1000      = 7 << 8,
};


enum {
  DRV8711_OFF_PWMMODE_bm          = 1 << 8,
};


enum {
  DRV8711_BLANK_ABT_bm            = 1 << 8,
};


enum {
  DRV8711_DECAY_DECMOD_SLOW       = 0 << 8,
  DRV8711_DECAY_DECMOD_OPT        = 1 << 8,
  DRV8711_DECAY_DECMOD_FAST       = 2 << 8,
  DRV8711_DECAY_DECMOD_MIXED      = 3 << 8,
  DRV8711_DECAY_DECMOD_AUTO_OPT   = 4 << 8,
  DRV8711_DECAY_DECMOD_AUTO_MIXED = 5 << 8,
};


enum {
  DRV8711_STALL_SDCNT_1           = 0 << 8,
  DRV8711_STALL_SDCNT_2           = 1 << 8,
  DRV8711_STALL_SDCNT_4           = 2 << 8,
  DRV8711_STALL_SDCNT_8           = 3 << 8,
  DRV8711_STALL_VDIV_32           = 0 << 10,
  DRV8711_STALL_VDIV_16           = 1 << 10,
  DRV8711_STALL_VDIV_8            = 2 << 10,
  DRV8711_STALL_VDIV_4            = 3 << 10,
};


enum {
  DRV8711_DRIVE_OCPTH_250         = 0 << 0,
  DRV8711_DRIVE_OCPTH_500         = 1 << 0,
  DRV8711_DRIVE_OCPTH_750         = 2 << 0,
  DRV8711_DRIVE_OCPTH_1000        = 3 << 0,
  DRV8711_DRIVE_OCPDEG_1          = 0 << 2,
  DRV8711_DRIVE_OCPDEG_2          = 1 << 2,
  DRV8711_DRIVE_OCPDEG_4          = 2 << 2,
  DRV8711_DRIVE_OCPDEG_8          = 3 << 2,
  DRV8711_DRIVE_TDRIVEN_250       = 0 << 4,
  DRV8711_DRIVE_TDRIVEN_500       = 1 << 4,
  DRV8711_DRIVE_TDRIVEN_1000      = 2 << 4,
  DRV8711_DRIVE_TDRIVEN_2000      = 3 << 4,
  DRV8711_DRIVE_TDRIVEP_250       = 0 << 6,
  DRV8711_DRIVE_TDRIVEP_500       = 1 << 6,
  DRV8711_DRIVE_TDRIVEP_1000      = 2 << 6,
  DRV8711_DRIVE_TDRIVEP_2000      = 3 << 6,
  DRV8711_DRIVE_IDRIVEN_100       = 0 << 8,
  DRV8711_DRIVE_IDRIVEN_200       = 1 << 8,
  DRV8711_DRIVE_IDRIVEN_300       = 2 << 8,
  DRV8711_DRIVE_IDRIVEN_400       = 3 << 8,
  DRV8711_DRIVE_IDRIVEP_50        = 0 << 10,
  DRV8711_DRIVE_IDRIVEP_100       = 1 << 10,
  DRV8711_DRIVE_IDRIVEP_150       = 2 << 10,
  DRV8711_DRIVE_IDRIVEP_200       = 3 << 10,
};


enum {
  DRV8711_STATUS_OTS_bm           = 1 << 0,
  DRV8711_STATUS_AOCP_bm          = 1 << 1,
  DRV8711_STATUS_BOCP_bm          = 1 << 2,
  DRV8711_STATUS_APDF_bm          = 1 << 3,
  DRV8711_STATUS_BPDF_bm          = 1 << 4,
  DRV8711_STATUS_UVLO_bm          = 1 << 5,
  DRV8711_STATUS_STD_bm           = 1 << 6,
  DRV8711_STATUS_STDLAT_bm        = 1 << 7,
  DRV8711_COMM_ERROR_bm           = 1 << 8,
};


#define DRV8711_READ(ADDR) ((1 << 15) | ((ADDR) << 12))
#define DRV8711_WRITE(ADDR, DATA) (((ADDR) << 12) | ((DATA) & 0xfff))
#define DRV8711_CMD_ADDR(CMD) (((CMD) >> 12) & 7)
#define DRV8711_CMD_IS_READ(CMD) ((1 << 15) & (CMD))

#define DRV8711_CTRL_ISGAIN_BM (3 << 8)
#define DRV8711_CTRL_GET_GAIN(CTRL) ((CTRL) & DRV8711_CTRL_ISGAIN_BM)

#define DRV8711_CTRL_GAIN(CTRL)                                         \
  (DRV8711_CTRL_GET_GAIN(CTRL) == DRV8711_CTRL_ISGAIN_5 ? 5 :           \
   (DRV8711_CTRL_GET_GAIN(CTRL) == DRV8711_CTRL_ISGAIN_10 ? 10 :        \
    (DRV8711_CTRL_GET_GAIN(CTRL) == DRV8711_CTRL_ISGAIN_20 ? 20 : 40)))

#define DRV8711_CTRL_VDIV_BM (3 << 10)
#define DRV8711_STALL_GET_VDIV(STALL) ((STALL) & DRV8711_CTRL_VDIV_BM)
#define DRV8711_STALL_VDIV(STALL)                                       \
  (DRV8711_STALL_GET_VDIV(STALL) == DRV8711_STALL_VDIV_4 ? 4 :          \
   (DRV8711_STALL_GET_VDIV(STALL) == DRV8711_STALL_VDIV_8 ? 8 :         \
    (DRV8711_STALL_GET_VDIV(STALL) == DRV8711_STALL_VDIV_16 ? 16 : 32)))

#define DRV8711_STALL_THRESH(STALL) ((0xff & (STALL)) * 1.8 / 256)

#define DRV8711_GAIN DRV8711_CTRL_GAIN(DRV8711_CTRL)
#define DRV8711_MAX_CURRENT                                     \
  (CURRENT_SENSE_REF / (CURRENT_SENSE_RESISTOR * DRV8711_GAIN))


typedef enum {
  DRV8711_DISABLED,
  DRV8711_IDLE,
  DRV8711_ACTIVE,
} drv8711_state_t;


typedef void (*stall_callback_t)(int driver);


void drv8711_init();
void drv8711_remap_switches();
void drv8711_set_state(int driver, drv8711_state_t state);
void drv8711_set_microsteps(int driver, uint16_t msteps);
void drv8711_set_stalled(int driver, bool stalled);
void drv8711_set_stall_detect(int driver, bool enable);
bool drv8711_detect_stall(int driver);
