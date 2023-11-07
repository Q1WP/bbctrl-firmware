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



module.exports = {
  template: '#dragbar-template',

  props: {
    direction: {
      validator(value) {
        return value == 'veritcal' || value == 'horizontal'
      },
      default: 'vertical'
    }
  },


  data() {
    return {
      last: 0
    }
  },


  methods: {
    resize(e) {
      if (e.which != 1) return // Primary click only
      e.preventDefault()

      let parent = e.target.parentNode
      let startX = e.pageX
      let startY = e.pageY
      let startHeight = parent.clientHeight
      let startWidth = parent.clientWidth
      let cursor = document.body.style.cursor
      document.body.style.cursor =
        this.direction == 'vertical' ? 'row-resize' : 'col-resize'

      let onmove = e => {
        e.preventDefault()

        if (this.direction == 'vertical') {
          let h = startHeight + (startY - e.pageY)
          if (h < 0) h = 0
          parent.style.height = h + 'px'

        } else {
          let w = startWidth + (startX - e.pageX)
          if (w < 0) w = 0
          parent.style.width = w + 'px'
        }

        window.dispatchEvent(new Event('resize'))
      }

      let onup = e => {
        e.preventDefault()

        document.body.style.cursor = cursor
        window.removeEventListener('mousemove', onmove)
        window.removeEventListener('mouseup', onup)

        // Toggle on double click
        let now = new Date().getTime()
        if (now - this.last < 500) {
          if (this.direction == 'vertical') parent.style.height =
            parent.style.height == '0px' ? '10000px' : '0px'
          else parent.style.width =
            parent.style.width == '0px' ? '10000px' : '0px'

          this.last = 0

        } else this.last = now

        window.dispatchEvent(new Event('resize'))
      }

      window.addEventListener('mousemove', onmove)
      window.addEventListener('mouseup', onup)
    }
  }
}
