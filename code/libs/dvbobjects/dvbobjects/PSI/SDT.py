#! /usr/bin/env python

# This file is part of the dvbobjects library.
# 
# Copyright © 2004-2013 Lorenzo Pallara l.pallara@avalpa.com
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
class service_description_section(Section):

    table_id = 0x42
    
    section_max_size = 1024

    def pack_section_body(self):

        self.table_id_extension = self.transport_stream_id
        self.private_indicator = 1
    
        # pack service_stream_loop
        sl_bytes = string.join(
            map(lambda x: x.pack(),
                self.service_loop),
            "")

        fmt = "!HB%ds" % len(sl_bytes)
        return pack(fmt,
	    self.original_network_id,
	    0xFF,
            sl_bytes,
            )

######################################################################
class service_description_other_ts_section(Section):

    table_id = 0x46
    
    section_max_size = 1024

    def pack_section_body(self):

        self.table_id_extension = self.transport_stream_id
        self.private_indicator = 1
    
        # pack service_stream_loop
        sl_bytes = string.join(
            map(lambda x: x.pack(),
                self.service_loop),
            "")

        fmt = "!HB%ds" % len(sl_bytes)
        return pack(fmt,
	    self.original_network_id,
	    0xFF,
            sl_bytes,
            )

######################################################################
class service_loop_item(DVBobject):

    def pack(self):
    
        # pack service_descriptor_loop
        sdl_bytes = string.join(
            map(lambda x: x.pack(),
                self.service_descriptor_loop),
            "")

        fmt = "!HBH%ds" % len(sdl_bytes)
        return pack(fmt,
                    self.service_ID,
                    0xFC | (self.EIT_schedule_flag << 1) | (self.EIT_present_following_flag),
		    (self.running_status << 13) | (self.free_CA_mode << 12) | (len(sdl_bytes) & 0x0FFF),
                    sdl_bytes,
                    )
