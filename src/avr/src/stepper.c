/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2021, Buildbotics LLC, All rights reserved.

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

#include "stepper.h"

#include "config.h"
#include "motor.h"
#include "hardware.h"
#include "estop.h"
#include "util.h"
#include "cpp_magic.h"
#include "exec.h"
#include "drv8711.h"

#include <util/atomic.h>

#include <string.h>
#include <stdio.h>


typedef struct {
  // Runtime
  bool busy;
  bool requesting;
  float dwell;
  uint8_t power_buf;
  uint8_t power_index;

  // Move prep
  bool move_ready;  // Prepped move ready for loader
  bool move_queued; // Prepped move queued
  float prep_dwell;
  int8_t power_next;

  power_update_t powers[2][POWER_MAX_UPDATES];

  uint32_t underrun;
} stepper_t;


static stepper_t st = {0};


void stepper_init() {
  // Setup step timer
  TIMER_STEP.CTRLB    = TC_WGMODE_NORMAL_gc; // Count to TOP & rollover
  TIMER_STEP.INTCTRLA = TC_OVFINTLVL_HI_gc;  // Interrupt level
  TIMER_STEP.PER      = STEP_TIMER_POLL;     // Timer rate
  TIMER_STEP.CTRLA    = TC_CLKSEL_DIV8_gc;   // Start step timer
}


static void _end_move() {
  for (int motor = 0; motor < MOTORS; motor++)
    motor_end_move(motor);
}


static void _load_move() {
  for (int motor = 0; motor < MOTORS; motor++)
    motor_load_move(motor);
}


void st_shutdown() {
  TIMER_STEP.CTRLA = 0;         // Stop stepper clock
  _end_move();                  // Stop motor clocks
  ADCB_CH0_INTCTRL = 0;         // Disable next move interrupt
}


/// Return true if motors or dwell are running
bool st_is_busy() {return st.busy;}


/// Interrupt handler for calling move exec function.
/// ADC channel 0 triggered by load ISR as a "software" interrupt.
ISR(STEP_LOW_LEVEL_ISR) {
  while (true) {
    stat_t status = exec_next();

    switch (status) {
    case STAT_NOP:                          // No move executed, idle
      if (!st.busy) {
        if (MIN_VELOCITY < exec_get_velocity()) st.underrun++;
        exec_set_velocity(0); // Velocity is zero if there are no moves

        spindle_idle();
      }
      break;

    case STAT_AGAIN: continue;              // No command executed, try again

    case STAT_OK:                           // Move executed
      ESTOP_ASSERT(st.move_queued, STAT_EXPECTED_MOVE);
      st.move_queued = false;
      st.move_ready = true;
      break;

    default: ESTOP_ASSERT(false, status); break;
    }

    break;
  }

  ADCB_CH0_INTCTRL = 0;
  st.requesting = false;
}


static void _request_exec_move() {
  if (st.requesting) return;
  st.requesting = true;

  // Use ADC as "software" interrupt to trigger next move exec as low interrupt.
  ADCB_CH0_INTCTRL = ADC_CH_INTLVL_LO_gc;
  ADCB_CTRLA = ADC_ENABLE_bm | ADC_CH0START_bm;
}


static void _update_power() {
  if (st.power_index < POWER_MAX_UPDATES)
    spindle_update(st.powers[st.power_buf][st.power_index++]);
}


/// Step timer interrupt routine.
/// Dwell or dequeue and load next move.
ISR(STEP_TIMER_ISR) {
  static uint8_t tick = 0;

  // Update spindle power on every tick
  _update_power();

  // Dwell
  if (0 < st.dwell) {
    st.dwell -= 0.001; // 1ms
    return;
  }
  st.dwell = 0;

  if (tick++ & 3) return; // Proceed every 4 ticks

  // If the next move is not ready try to load it
  if (!st.move_ready) {
    _request_exec_move();
    _end_move();
    tick = 0; // Try again in 1ms
    st.busy = false;
    return;
  }

  if (st.prep_dwell) {
    // End last move, if any
    _end_move();

    // Start dwell
    st.dwell = st.prep_dwell;
    st.prep_dwell = 0;

  } else {
    // Start move
    _load_move();

    // Request next move when not in a dwell.  Requesting the next move may
    // power up motors which should not be powered up during a dwell.
    _request_exec_move();
  }

  // Handle power updates
  if (st.power_next != -1) {
    st.power_index = 0;
    st.power_buf = st.power_next;
    st.power_next = -1;
    _update_power();
  }

  st.busy = true;        // Executing move so mark busy
  st.move_ready = false; // We are done with this move, flip the flag back
}


void st_prep_power(const power_update_t powers[]) {
  ESTOP_ASSERT(!st.move_ready, STAT_STEPPER_NOT_READY);
  st.power_next = !st.power_buf;
  memcpy(st.powers[st.power_next], powers,
         sizeof(power_update_t) * POWER_MAX_UPDATES);
}


void st_prep_line(const float target[]) {
  // Trap conditions that would prevent queuing the line
  ESTOP_ASSERT(!st.move_ready, STAT_STEPPER_NOT_READY);

  // Prepare motor moves
  for (int motor = 0; motor < MOTORS; motor++)
    motor_prep_move(motor, target[motor_get_axis(motor)]);

  st.move_queued = true; // signal prep buffer ready (do this last)
}


/// Add a dwell to the move buffer
void st_prep_dwell(float seconds) {
  ESTOP_ASSERT(!st.move_ready, STAT_STEPPER_NOT_READY);
  if (seconds <= 1e-4) seconds = 1e-4; // Min dwell
  st.power_next = !st.power_buf;
  spindle_load_power_updates(st.powers[st.power_next], 0, 0);
  st.prep_dwell = seconds;
  st.move_queued = true; // signal prep buffer ready
}


// Var callbacks
uint32_t get_underrun() {return st.underrun;}


float get_dwell_time() {
  float dwell;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) dwell = st.dwell;
  return dwell;
}
