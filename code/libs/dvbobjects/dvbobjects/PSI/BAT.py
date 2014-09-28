#! /usr/bin/env python

# This file is part of the dvbobjects library.
# 
# Copyright © 2004-2013 Lorenzo Pallara l.pallara@avalpa.com
# Copyright © 2010 Andreas Regel
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
class bouquet_association_section(Section):

    table_id = 0x4A
    
    section_max_size = 1024

    def pack_section_body(self):
    
        # pack bouquet_descriptor_loop
        bdl_bytes = string.join(
            map(lambda x: x.pack(),
                self.bouquet_descriptor_loop),
            "")

        # pack transport_stream_loop
        tsl_bytes = string.join(
            map(lambda x: x.pack(),
                self.transport_stream_loop),
            "")

        self.table_id_extension = self.bouquet_id
        self.private_indicator = 1

        fmt = "!H%dsH%ds" % (len(bdl_bytes), len(tsl_bytes))
        return pack(fmt,
            0xF000 | (len(bdl_bytes) & 0x0FFF),
            bdl_bytes,
            0xF000 | (len(tsl_bytes) & 0x0FFF),
            tsl_bytes,
            )

######################################################################
class transport_stream_loop_item(DVBobject):

    def pack(self):
    
        # pack transport_descriptor_loop
        tdl_bytes = string.join(
            map(lambda x: x.pack(),
                self.transport_descriptor_loop),
            "")

        fmt = "!HHH%ds" % len(tdl_bytes)
        return pack(fmt,
                    self.transport_stream_id,
                    self.original_network_id,
                    0xF000 | (len(tdl_bytes) & 0x0FFF),
                    tdl_bytes,
                    )
