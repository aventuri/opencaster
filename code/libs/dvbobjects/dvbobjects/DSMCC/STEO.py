#! /usr/bin/env python

#
# Copyright (C) 2006-2013  Lorenzo Pallara, l.pallara@avalpa.com
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

import os
import string

from dvbobjects.DSMCC.BIOP.Tap import *
from dvbobjects.utils import *


###############################
class Event_names(DVBobject):

    def pack(self):

        result = pack("!H", self.eventnames_count)
        for name in self.event_name_loop:
            name = name + "\x00"
            result = result + pack(
                "!B%ds" % len(name),
                len(name),
                name,
                )
	return result
    
###############################
class Event_ids(DVBobject):

    def pack(self):

        result = pack("!B", self.eventids_count)
        for id in self.event_id_loop:
            result = result + pack("!H", id)
	return result
    
###############################
class Taps(DVBobject):

    def pack(self):

        result = pack("!B", self.taps_count)
        for tap in self.tap_loop:
            result = result + tap.pack()
	return result
###############################
	

