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

#include "config.h"
#include "exec.h"
#include "command.h"
#include "spindle.h"
#include "util.h"
#include "SCurve.h"

#include <math.h>
#include <float.h>
#include <string.h>


typedef struct {
  float start[AXES];
  float target[AXES];
  float times[7];
  float target_vel;
  float max_accel;
  float max_jerk;

  float unit[AXES];
  float length;
} line_t;


static struct {
  line_t line;

  uint8_t section;
  float t;

  float iD; // Initial section distance
  float iV; // Initial section velocity
  float iA; // Initial section acceleration
  float jerk;
  float lV; // Last velocity
  float lD; // Last distance

  power_update_t power_updates[POWER_MAX_UPDATES];
} l;


static void _segment_target(float target[AXES], float d) {
  for (int axis = 0; axis < AXES; axis++)
    target[axis] = l.line.start[axis] + l.line.unit[axis] * d;
}


static float _segment_distance(float t) {
  return l.iD + SCurve::distance(t, l.iV, l.iA, l.jerk);
}


static float _segment_velocity(float t) {
  return l.iV + SCurve::velocity(t, l.iA, l.jerk);
}


static float _segment_accel(float t) {
  return l.iA + SCurve::acceleration(t, l.jerk);
}


static bool _section_next() {
  while (++l.section < 7) {
    if (!l.line.times[l.section]) continue;

    // Jerk
    switch (l.section) {
    case 0: case 6: l.jerk =  l.line.max_jerk; break;
    case 2: case 4: l.jerk = -l.line.max_jerk; break;
    default: l.jerk = 0;
    }
    exec_set_jerk(l.jerk);

    // Acceleration
    switch (l.section) {
    case 1: case 2: l.iA =  l.line.max_jerk * l.line.times[0]; break;
    case 5: case 6: l.iA = -l.line.max_jerk * l.line.times[4]; break;
    default: l.iA = 0;
    }

    return true;
  }

  return false;
}


static stat_t _exec_segment(float time, const float target[], float vel,
                            float accel) {
  return exec_segment(time, target, vel, accel, l.line.max_accel,
                      l.line.max_jerk, l.power_updates);
}


static stat_t _line_exec() {
  // Compute times
  float section_time = l.line.times[l.section];
  float seg_time = SEGMENT_TIME;
  l.t += seg_time;

  // Don't exceed section time
  if (section_time < l.t) {
    seg_time = section_time - (l.t - seg_time);
    l.t = section_time;
  }

  // Compute distance, velocity and acceleration
  float d = _segment_distance(l.t);
  float v = _segment_velocity(l.t);
  float a = _segment_accel(l.t);

  // Don't allow overshoot
  if (l.line.length < d) d = l.line.length;

  // Handle synchronous speeds
  spindle_load_power_updates(l.power_updates, l.lD, d);
  l.lD = d;

  // Check if section complete
  if (l.t == section_time) {
    if (_section_next()) {
      // Setup next section
      l.t = 0;
      l.iD = d;
      l.iV = v;

    } else {
      exec_set_cb(0);

      // Last segment of last section
      // Use exact target values to correct for floating-point errors
      return _exec_segment(seg_time, l.line.target, l.line.target_vel, a);
    }
  }

  // Compute target position from distance
  float target[AXES];
  _segment_target(target, d);

  // Segment move
  return _exec_segment(seg_time, target, v, a);
}


stat_t command_line(char *cmd) {
  line_t line = {};

  cmd++; // Skip command code

  // Get start position
  command_get_position(line.start);

  // Get target velocity
  if (!decode_float(&cmd, &line.target_vel)) return STAT_BAD_FLOAT;
  if (line.target_vel < 0) return STAT_INVALID_ARGUMENTS;

  // Get max accel
  if (!decode_float(&cmd, &line.max_accel)) return STAT_BAD_FLOAT;
  if (line.max_accel < 0) return STAT_INVALID_ARGUMENTS;

  // Get max jerk
  if (!decode_float(&cmd, &line.max_jerk)) return STAT_BAD_FLOAT;
  if (line.max_jerk < 0) return STAT_INVALID_ARGUMENTS;

  // Get target position
  copy_vector(line.target, line.start);
  stat_t status = decode_axes(&cmd, line.target);
  if (status) return status;

  // Get times
  bool has_time = false;
  while (*cmd) {
    if (*cmd < '0' || '6' < *cmd) break;
    int section = *cmd - '0';
    cmd++;

    float time;
    if (!decode_float(&cmd, &time)) return STAT_BAD_FLOAT;

    if (time < 0) return STAT_NEGATIVE_SCURVE_TIME;
    line.times[section] = time;
    if (time) has_time = true;
   }

  if (!has_time) return STAT_ALL_ZERO_SCURVE_TIMES;

  // Check for end of command
  if (*cmd) return STAT_INVALID_ARGUMENTS;

  // Set next start position
  command_set_position(line.target);

  // Compute direction vector
  for (int axis = 0; axis < AXES; axis++) {
    line.unit[axis] = line.target[axis] - line.start[axis];
    line.length += line.unit[axis] * line.unit[axis];
  }

  line.length = sqrt(line.length);
  for (int axis = 0; axis < AXES; axis++)
    if (line.unit[axis]) line.unit[axis] /= line.length;

  // Queue
  command_push(COMMAND_line, &line);

  return STAT_OK;
}


unsigned command_line_size() {return sizeof(line_t);}


void command_line_exec(void *data) {
  l.line = *(line_t *)data;

  // Setup first section
  l.t = 0;
  l.iD = 0;
  l.lD = 0;
  // If current velocity is non-zero use last target velocity
  l.iV = exec_get_velocity() ? l.lV : 0;
  l.lV = l.line.target_vel;

  // Find first section
  l.section = -1;
  if (!_section_next()) return;

#if 0
  // Compare start position to actual position
  float diff[AXES];
  bool report = false;
  exec_get_position(diff);
  for (int i = 0; i < AXES; i++) {
    diff[i] -= l.line.start[i];
    if (0.1 < fabs(diff[i])) report = true;
  }

  if (report)
    STATUS_DEBUG("diff: %.4f %.4f %.4f %.4f",
                 diff[0], diff[1], diff[2], diff[3]);
#endif

  // Set callback
  exec_set_cb(_line_exec);
}
