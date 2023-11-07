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



module.exports = {
  template: '#mapped-io-template',
  props: ['pin', 'state', 'template'],


  computed: {
    index() {
      let io_map = this.template['io-map']
      let code = ''

      for (let i = 0; i < io_map.pins.length; i++) {
        let pin = io_map.pins[i]
        if (pin.id == this.pin) return i
      }

      console.error('Pin', this.pin, 'not found in template')
    },


    code() {return this.template['io-map'].index[this.index]},
    type() {return this.template['io-map'].pins[this.index].type},


    func() {
      let funcID = this.state[this.code + 'io']
      return this.template['io-map'].template.function.values[funcID]
    },


    mode() {
      let modeID = this.state[this.code + 'im']
      return this.template['io-map'].template.mode.values[modeID]
    }
  },


  methods: {
    capitalize(s) {return s.charAt(0).toUpperCase() + s.slice(1)}
  }
}
