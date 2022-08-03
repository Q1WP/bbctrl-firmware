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


var api = require('./api');


module.exports = {
  template: '#view-settings-template',
  props: ['config', 'template', 'state'],


  data: function () {
    return {
      index: -1,
      view: undefined,
      modified: false
    }
  },


  components: {
    'settings-general':   require('./settings-general'),
    'settings-motor':     require('./settings-motor'),
    'settings-tool':      require('./settings-tool'),
    'settings-io':        require('./settings-io'),
    'settings-macros':    require('./settings-macros'),
    'settings-network':   require('./settings-network'),
    'settings-admin':     require('./settings-admin')
  },


  events: {
    'route-changing': function (path, cancel) {
      if (this.modified && path[0] != 'settings') {
        cancel();

        let done = (success) => {if (success) location.hash = path.join(':')}

        this.$root.open_dialog({
          title: 'Save settings?',
          body: 'Changes to the settings have not been saved.  ' +
            'Would you like to save them now?',
          width: '320px',
          buttons: [
            {
              text: 'Cancel',
              title: 'Stay on the Settings page.'
            }, {
              text: 'Discard',
              title: 'Discard settings changes.'
            }, {
              text: 'Save',
              title: 'Save settings.',
              'class': 'button-success'
            }
          ],
          callback: {
            save() {this.save(done)},
            discard() {this.discard(done)}
          }
        });
      }
    },


    route: function (path) {
      if (path[0] != 'settings') return;
      var view = path.length < 2 ? '' : path[1];

      if (typeof this.$options.components['settings-' + view] == 'undefined')
        return this.$root.replace_route('settings:general');

      if (path.length == 3) this.index = path[2];
      this.view = view;
    },


    'config-changed': function () {
      this.modified = true;
      return false;
    },


    'input-changed': function() {
      this.$dispatch('config-changed');
      return false;
    }
  },


  ready: function () {this.$root.parse_hash()},


  methods: {
    save: function (done) {
      api.put('config/save', this.config)
        .done((data) => {
          this.modified = false
          done(true)
        }).fail((error) => {
          this.api_error('Save failed', error)
          done(false)
        })
    },


    discard: function (done) {
      this.$root.update((success) => {
        this.modified = false
        done(success)
      })
    }
  }
}
