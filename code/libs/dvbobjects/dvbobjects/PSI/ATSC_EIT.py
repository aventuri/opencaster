#! /usr/bin/env python

# This file is part of the dvbobjects library.
# 
# Copyright © 2005-2013 Lorenzo Pallara l.pallara@avalpa.com
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
from dvbobjects.utils.MJD import *
from dvbobjects.ATSC.Descriptors import *
from dvbobjects.ATSC.Loops import *

######################################################################
class event_information_section(Section):
    
    table_id = 0xCB
    
    section_max_size = 4093

    def pack_section_body(self):

        self.table_id_extension = self.source_id
        self.protocol_version = 0
	
        self.num_events_in_section = len(self.event_loop)

        # pack event_loop
        el_bytes = string.join(
            map(lambda x: x.pack(),
                self.event_loop),
            "")

        fmt = "!BB%ds" % len(el_bytes)
        return pack(fmt,
	    self.protocol_version,
	    self.num_events_in_section,
            el_bytes,
            )

######################################################################
class event_loop_item(DVBobject):

    def pack(self):
    

        # pack event_descriptor_loop
        event_descriptors_bytes = string.join(
            map(lambda x: x.pack(),
                self.descriptor_loop),
            "")

	title_text_bytes = self.title_text.pack()

        fmt = "!HLBHB%dsH%ds" % (len(title_text_bytes), len(event_descriptors_bytes))
        return pack(fmt,
                    (0x3 << 14) | (self.event_id & 0x3FFF),
		    self.start_time,
		    (0x3 << 6) | ((self.ETM_location & 0x3) << 4) | ((self.length_in_seconds >> 16) & 0xF),
		    (self.length_in_seconds & 0xFFFF),
		    len(title_text_bytes),
		    title_text_bytes,
		    (0xF << 12) | (len(event_descriptors_bytes) & 0x0FFF),
		    event_descriptors_bytes,
		    )
