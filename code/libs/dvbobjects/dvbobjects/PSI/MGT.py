#! /usr/bin/env python

# This file is part of the dvbobjects library.
# 
# Copyright 2010-2013 Lorenzo Pallara l.pallara@avalpa.com
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

######################################################################
class master_guide_section(Section):

    table_id = 0xC7
    
    section_max_size = 4096

    def pack_section_body(self):
    
        # pack tables_loop
        tl_bytes = string.join(
            map(lambda x: x.pack(),
                self.tables_loop),
            "")

        # pack descriptors_loop
        dl_bytes = string.join(
            map(lambda x: x.pack(),
                self.descriptors_loop),
            "")

        self.table_id_extension = 0
        self.private_indicator = 1

        fmt = "!BH%dsH%ds" % (len(tl_bytes), len(dl_bytes))
        return pack(fmt,
    	    self.ATSC_protocol_version,
            len(self.tables_loop),
            tl_bytes,
            0xF000 | (len(dl_bytes) & 0x0FFF),
            dl_bytes,
            )

######################################################################
class table_loop_item(DVBobject):

    def pack(self):
    
        # pack transport_descriptor_loop
        dl_bytes = string.join(
            map(lambda x: x.pack(),
                self.descriptors_loop),
            "")

        fmt = "!HHBLH%ds" % len(dl_bytes)
        return pack(fmt,
    		    self.table_type,
    		    0xE000 | (self.table_type_pid & 0x1FFF),
    		    0xE0 | (self.table_type_version_number & 0x1F),
    		    self.number_bytes,
                    0xF000 | (len(dl_bytes) & 0x0FFF),
                    dl_bytes,
                    )
