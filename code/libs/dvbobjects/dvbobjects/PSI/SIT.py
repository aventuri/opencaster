#! /usr/bin/env python

# This file is part of the dvbobjects library.
# 
# Copyright 2011-2013 Lorenzo Pallara l.pallara@avalpa.com
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

import string
from dvbobjects.MPEG.Section import Section
from dvbobjects.utils import *
from dvbobjects.DVB.Descriptors import *

######################################################################
class selection_information_section(Section):

    table_id = 0x7F
    
    section_max_size = 4092

    def pack_section_body(self):

        self.table_id_extension = 0xFFFF
        self.private_indicator = 1
        self.section_number = 0
        self.last_section_number = 0
    
        ti_bytes = string.join(
            map(lambda x: x.pack(),
                self.transmission_info_loop),
            "")

        sl_bytes = string.join(
            map(lambda x: x.pack(),
                self.service_loop),
            "")

        fmt = "!H%ds%ds" % (len(ti_bytes), len(sl_bytes))
        return pack(fmt,
	    0xF000 | len(ti_bytes),
            ti_bytes,
            sl_bytes
            )

######################################################################
class service_loop_item(DVBobject):

    def pack(self):
    
        sdl_bytes = string.join(
            map(lambda x: x.pack(),
                self.service_descriptor_loop),
            "")

        fmt = "!HH%ds" % len(sdl_bytes)
        return pack(fmt,
                    self.service_ID,
                    0x8000 | (self.running_status << 13) |  (len(sdl_bytes) & 0x0FFF),
                    sdl_bytes,
                    )
