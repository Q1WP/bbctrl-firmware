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

__all__ = ['Events']


class Events(object):
  def __init__(self, ctrl):
    self.ctrl = ctrl
    self.listeners = {}
    self.log = ctrl.log.get('Events')


  def on(self, event, listener):
    if not event in self.listeners: self.listeners[event] = []
    self.listeners[event].append(listener)


  def off(self, event, listener):
    if event in self.listeners:
      self.listeners[event].remove(listener)


  def emit(self, event, *args, **kwargs):
    if event in self.listeners:
      for listener in self.listeners[event]:
        try:
          listener(*args, **kwargs)
        except Exception as e:
          self.log.exception()
