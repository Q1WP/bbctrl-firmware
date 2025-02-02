/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2022, Buildbotics LLC, All rights reserved.

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

'use strict'


module.exports = {
  template: '#range-slider-template',


  props: {
    min:   {type: Number, default: 0},
    max:   {type: Number, default: 100},
    step:  {type: Number, default: 1},
    value: {type: Number, required: true, twoWay: true},
    label: {type: String}
  },


  computed: {
    pos() {
      let width = $(this.$el).find('input')[0].offsetWidth
      return width * (this.value - this.min) / (this.max - this.min)
    }
  },


  methods: {
    on_change(e) {this.$emit('change', this.value)},
  }
}
