#! /usr/bin/env python

# This file is part of the dvbobjects library.
# 
# Copyright 2000-2001, GMD, Sankt Augustin
# -- German National Research Center for Information Technology 
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

from dvbobjects.utils import *
import Tap
import string

######################################################################
class ModuleInfo(DVBobject):

    ModuleTimeOut = 0xFFFFFFFF
    BlockTimeOut  = 0xFFFFFFFF
#   MinBlockTime  = 0x000061a8
    MinBlockTime  = 0x00000001

    userInfo = ""

    def __init__(self, **kwargs):
        # Initialize SuperClass
        apply(DVBobject.__init__, (self,), kwargs)
        self.taps = [
            Tap.object_tap(
                assocTag = self.assocTag,
                )
            ]

    def pack(self):

        assert len(self.taps) >= 1      # MHP
        taps_bytes = string.join(map(lambda t: t.pack(),
                                     self.taps),
                                 "")
        user_info_bytes = string.join(map(lambda ui: ui.pack(),
                                     self.userInfo),
                                 "")

        FMT =("!"
              "L"                       # ModuleTimeOut
              "L"                       # BlockTimeOut
              "L"                       # MinBlockTime
              "B"                       # taps_count
              "%ds"                     # taps
              "B"
	      "%ds"                    # userInfo
              ) % (
            len(taps_bytes),
            len(user_info_bytes),
            )

        return pack(FMT,
                    self.ModuleTimeOut,
                    self.BlockTimeOut,
                    self.MinBlockTime,
                    len(self.taps),
                        taps_bytes,
                        len(user_info_bytes),
                    user_info_bytes,
                    )

