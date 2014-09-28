#! /usr/bin/env python

# This file is part of the dvbobjects library.
# 
# Copyright 2006-2013 Lorenzo Pallara
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
from dvbobjects.DSMCC.Descriptors import *

######################################################################
class stream_event_section(Section):
    
    table_id = 0x3d
    
    section_max_size = 4096

    def pack_section_body(self):

        self.table_id_extension = self.event_id
	self.segment_last_section_number = self.last_section_number
	self.last_table_id = self.table_id
	
        # pack event_loop
        stel_bytes = string.join(
            map(lambda x: x.pack(),
                self.stream_event_descriptor_loop),
            "")

        fmt = "!%ds" % len(stel_bytes)
        return pack(fmt,
            stel_bytes,
            )

