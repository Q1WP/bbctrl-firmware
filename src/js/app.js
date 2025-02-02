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

var api     = require('./api');
var cookie  = require('./cookie');
var Sock    = require('./sock');
var util    = require('./util');
var Program = require('./program');


module.exports = new Vue({
  el: 'body',


  data() {
    return {
      status: 'connecting',
      currentView: 'loading',
      template: require('../resources/config-template.json'),
      config: {
        settings: {units: 'METRIC'},
        motors: [{}, {}, {}, {}],
        version: '<loading>'
      },
      state: {messages: []},
      crosshair: cookie.get_bool('crosshair', false),
      selected_program: new Program(cookie.get('selected-path')),
      active_program: undefined,
      errorTimeout: 30,
      errorTimeoutStart: 0,
      errorMessage: '',
      checkedUpgrade: false,
      latestVersion: '',
      showError: false,
      showPopup: true,
      webGLSupported: util.webgl_supported()
    }
  },


  components: {
    'estop':          {template: '#estop-template'},
    'view-loading':   {template: '<h1>Loading...</h1>'},
    'view-control':   require('./view-control'),
    'view-viewer':    require('./view-viewer'),
    'view-editor':    require('./view-editor'),
    'view-settings':  require('./view-settings'),
    'view-files':     require('./view-files'),
    'view-camera':    {template: '#view-camera-template'},
    'view-docs':      require('./view-docs')
  },


  watch: {
    crosshair() {cookie.set_bool('crosshair', this.crosshair)},


    'state.active_program'() {
      let path = this.state.active_program
      this.active_program = path ? new Program(path) : undefined
    },


    'state.first_file'(value) {
      if (!this.selected_program.path) this.select_path(value)
    }
  },


  events: {
    route(path) {
      let oldView = this.currentView;

      if (typeof this.$options.components['view-' + path[0]] == 'undefined')
        return location.hash = 'control';
      else this.currentView = path[0];

      if (oldView != this.currentView) $('#main').scrollTop(0)
    },


    'hostname-changed'(hostname) {this.hostname = hostname},


    send(msg) {
      if (this.status == 'connected') {
        console.debug('>', msg);
        this.sock.send(msg);
      }
    },


    connected() {this.update(this.parse_hash)},
    update() {this.update(this.parse_hash)},


    check(show_message) {
      this.latestVersion = '';

      $.ajax({
        type: 'GET',
        url: 'https://buildbotics.com/bbctrl/latest.txt',
        data: {hid: this.state.hid},
        cache: false

      }).done(function (data) {
        this.latestVersion = data;
        if (!show_message) return;
        var cmp = util.compare_versions(this.config.version, this.latestVersion)

        var msg;
        if (cmp == 0) msg = 'You have the latest official firmware.'
        else {
          msg = 'Your firmware is ' + (cmp < 0 ? 'older': 'newer') +
            ' than the latest official firmware release, version' +
            this.latestVersion + '.'

          if (cmp < 0) msg += ' Please upgrade.';
        }

        this.open_dialog({
          icon: cmp ? (cmp < 0 ? 'chevron-left' : 'chevron-right') : 'check',
          title: 'Firmware check',
          body: msg
        })

      }.bind(this))
    },


    error(msg) {
      // Honor user error blocking
      if (Date.now() - this.errorTimeoutStart < this.errorTimeout * 1000)
        return;

      // Wait at least 1 sec to pop up repeated errors
      if (1 < msg.repeat && Date.now() - msg.ts < 1000) return;

      // Popup error dialog
      this.showError = true;
      this.errorMessage = msg.msg;
    }
  },


  computed: {
    popupMessages() {
      var msgs = [];

      for (var i = 0; i < this.state.messages.length; i++) {
        var text = this.state.messages[i].text;
        msgs = msgs.concat(text.split('#'))
      }

      this.showPopup = msgs.length != 0;

      return msgs;
    },


    show_upgrade() {
      if (!this.latestVersion) return false;
      return util.compare_versions(this.config.version, this.latestVersion) < 0
    }
  },


  ready() {
    $(window).on('hashchange', this.parse_hash);
    this.connect();

    if (!this.webGLSupported) {
      var msg = 'Your browser does not have hardware support for 3D ' +
          'graphics.  3D viewer disabled.'

      console.warn(msg);

      if (!cookie.get_bool('webgl-warning', false))
        this.open_dialog({
          title: 'WebGL Warning',
          body: msg,
          buttons: 'ok',
          callback() {cookie.set_bool('webgl-warning', true)}
        });
    }
  },


  methods: {
    metric() {return this.config.settings.units != 'IMPERIAL'},


    block_error_dialog() {
      this.errorTimeoutStart = Date.now();
      this.showError = false;
    },


    estop() {
      if (this.state.xx == 'ESTOPPED') api.put('clear');
      else api.put('estop');
    },


    update(done) {
      api.get('config/load').done(function (config) {
        util.update_object(this.config, config, true);

        if (!this.checkedUpgrade) {
          this.checkedUpgrade = true;

          var check = this.config.admin['auto-check-upgrade'];
          if (typeof check == 'undefined' || check)
            this.$emit('check');
        }

        if (done) done(true);
      }.bind(this))
    },


    connect() {
      this.sock = new Sock('//' + window.location.host + '/sockjs');

      this.sock.onmessage = (e) => {
        if (typeof e.data != 'object') return;

        if ('log' in e.data) {
          this.$broadcast('log', e.data.log);
          delete e.data.log;
        }

        // Check for session ID change on controller
        if ('sid' in e.data) {
          if (typeof this.sid == 'undefined') this.sid = e.data.sid;

          else if (this.sid != e.data.sid) {
            if (typeof this.hostname != 'undefined' &&
                String(location.hostname) != 'localhost')
              location.hostname = this.hostname;
            location.reload(true);
          }
        }

        util.update_object(this.state, e.data, false);
        this.$broadcast('update');

        // Disable WebGL on Pi 3 for local head
        if (this.webGLSupported) {
          var model = this.state.rpi_model
          if (location.hostname == 'localhost' && model != undefined &&
              model.indexOf('Pi 3') != -1)
            this.webGLSupported = false;
        }
      }

      this.sock.onopen = (e) => {
        this.status = 'connected';
        this.$emit(this.status);
        this.$broadcast(this.status);
      }

      this.sock.onclose = (e) => {
        this.status = 'disconnected';
        this.$emit(this.status);
        this.$broadcast(this.status);
      }
    },


    route(path) {
      var cancel = false;
      var cb = function () {cancel = true}

      this.$emit('route-changing', path, cb);
      this.$broadcast('route-changing', path, cb);

      if (cancel) return history.back();
      this.$emit('route', path);
      this.$broadcast('route', path);
    },


    replace_route(hash) {
      history.replaceState(null, '', '#' + hash);
      this.parse_hash()
    },


    parse_hash() {
      var hash = location.hash.substr(1);

      if (!hash.trim().length)
        return location.hash = 'control';

      this.route(hash.split(':'));
    },


    close_messages(action) {
      if (action == 'stop') api.put('stop');
      if (action == 'continue') api.put('unpause');

      // Acknowledge messages
      if (this.state.messages.length) {
        var id = this.state.messages.slice(-1)[0].id
        api.put('message/' + id + '/ack');
      }
    },


    file_dialog(config) {this.$refs.fileDialog.open(config)},
    upload(config)      {this.$refs.uploader.upload(config)},
    open_dialog(config) {this.$refs.dialog.open(config)},
    error_dialog(msg)   {this.$refs.dialog.error(msg)},
    warning_dialog(msg) {this.$refs.dialog.warning(msg)},
    success_dialog(msg) {this.$refs.dialog.success(msg)},


    api_error(msg, error) {
      if (error != undefined) {
        if (error.message != undefined)
          msg += '\n' + error.message;
        else msg += '\n' + JSON.stringify(error);
      }

      this.error_dialog(msg);
    },


    run(path) {
      if (this.state.xx != 'READY') return;
      api.put('start/' + path).done(function () {location.hash = 'control'})
    },


    select_path(path) {
      if (path && this.selected_program.path != path) {
        cookie.set('selected-path', path)
        this.selected_program = new Program(path)
      }

      return this.selected_program
    },


    edit(path) {
      this.select_path(path)
      location.hash = 'editor'
    },


    view(path) {
      this.select_path(path)
      location.hash = 'viewer'
    }
  }
})
