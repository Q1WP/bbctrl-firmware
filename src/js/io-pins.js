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

'use strict'


module.exports = {
  template: "#io-pins-template",
  props: ['template', 'state'],


  components: {
    'io-pins-row': {
      template: "#io-pins-row-template",
      props: ['pin'],
      replace: true
    }
  },


  computed: {
    columns: () => ['pin', 'state', 'mode', 'function', '',
                    'pin', 'state', 'mode', 'function'],


    rows: function () {return Math.ceil(this.io_pins.length / 2.0)},


    io_pins: function () {
      let l = []
      let io_map = this.template['io-map']
      let pins = io_map.pins
      let modes = io_map.template.mode.values
      let functions = io_map.template.function.values

      for (let i = 0; i < pins.length; i++) {
        let c = String.fromCharCode(97 + i)
        let state = this.state[c + 'is']
        let mode = modes[this.state[c + 'im']]
        let func = functions[this.state[c + 'io']]

        l.push({id: pins[i].id, state, mode, func})
      }

      return l
    }
  }
}
