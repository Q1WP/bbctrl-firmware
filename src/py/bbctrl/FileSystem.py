################################################################################
#                                                                              #
#                 This file is part of the Buildbotics firmware.               #
#                                                                              #
#        Copyright (c) 2015 - 2021, Buildbotics LLC, All rights reserved.      #
#                                                                              #
#         This Source describes Open Hardware and is licensed under the        #
#                                 CERN-OHL-S v2.                               #
#                                                                              #
#         You may redistribute and modify this Source and make products        #
#    using it under the terms of the CERN-OHL-S v2 (https:/cern.ch/cern-ohl).  #
#           This Source is distributed WITHOUT ANY EXPRESS OR IMPLIED          #
#    WARRANTY, INCLUDING OF MERCHANTABILITY, SATISFACTORY QUALITY AND FITNESS  #
#     FOR A PARTICULAR PURPOSE. Please see the CERN-OHL-S v2 for applicable    #
#                                  conditions.                                 #
#                                                                              #
#                Source location: https://github.com/buildbotics               #
#                                                                              #
#      As per CERN-OHL-S v2 section 4, should You produce hardware based on    #
#    these sources, You must maintain the Source Location clearly visible on   #
#    the external case of the CNC Controller or other product you make using   #
#                                  this Source.                                #
#                                                                              #
#                For more information, email info@buildbotics.com              #
#                                                                              #
################################################################################

import os
import shutil

from . import util

__all__ = ['FileSystem']


class FileSystem:
  extensions = ('.nc', '.gc', '.gcode', '.ngc', '.tap', '.txt', '.tpl')


  def __init__(self, ctrl):
    self.ctrl = ctrl
    self.log = ctrl.log.get('FileSystem')
    self.locations = ['Home']

    upload = self.ctrl.root + '/upload'
    os.environ['GCODE_SCRIPT_PATH'] = upload

    if not os.path.exists(upload):
      os.mkdir(upload)
      from shutil import copy
      copy(util.get_resource('http/buildbotics.nc'), upload)

    self.usb_update()

    self._update_first_file()
    ctrl.events.on('invalidate', self._invalidate)


  def _invalidate(self, path):
    if path == self.ctrl.state.get('first_file', ''):
      self._update_first_file()


  def _update_first_file(self):
    # Get GCode files from root upload directory
    upload = self.ctrl.root + '/upload'

    files = []
    for path in os.listdir(upload):
      parts = os.path.splitext(path)
      if (len(parts) == 2 and parts[1] in self.extensions and
          os.path.isfile(upload + '/' + path)):
        files.append(path)

    files.sort()

    # Set first file
    path = 'Home/' + files[0] if len(files) else ''
    self.ctrl.state.set('first_file', path)


  def realpath(self, path):
    path = os.path.normpath(path)
    parts = path.split('/', 1)

    if not len(parts): return ''
    path = parts[1] if len(parts) == 2 else ''

    if parts[0] == 'Home': return self.ctrl.root + '/upload/' + path

    usb = '/media/' + parts[0]
    if os.path.exists(usb): return usb + '/' + path

    return ''


  def exists(self, path): return os.path.exists(self.realpath(path))
  def isfile(self, path): return os.path.isfile(self.realpath(path))


  def delete(self, path):
    realpath = self.realpath(path)

    try:
      if os.path.isdir(realpath): shutil.rmtree(realpath, True)
      else: os.unlink(realpath)
    except OSError: pass

    self.log.info('Deleted ' + path)
    self.ctrl.events.emit('invalidate', path)


  def mkdir(self, path):
    realpath = self.realpath(path)

    if not os.path.exists(realpath):
      os.makedirs(realpath)
      os.sync()


  def write(self, path, data):
    realpath = self.realpath(path)

    with open(realpath.encode('utf8'), 'wb') as f:
      f.write(data)

      self.log.info('Wrote ' + path)
      self.ctrl.events.emit('invalidate', path)
      os.sync()


  def usb_update(self):
    self.locations = ['Home']

    for name in os.listdir('/media'):
      if os.path.isdir('/media/' + name):
        self.locations.append(name)

    self.ctrl.state.set('locations', self.locations)
