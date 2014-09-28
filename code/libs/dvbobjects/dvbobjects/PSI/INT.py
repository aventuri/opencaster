#! /usr/bin/env python

# Copyright  2007 Andreas Berger
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
class ip_mac_notification_section(Section):

    table_id = 0x4c
    
    section_max_size = 4096

    def pack_section_body(self):

        self.action_type = 0x01
        self.platform_id_hash = ( (self.platform_id>>16) & 0xff ) ^ ( (self.platform_id>>8) & 0xff ) ^ ( self.platform_id & 0xff )
    
        # pack platform descriptor loop
        pdl_bytes = string.join(
            map(lambda x: x.pack(),
                self.platform_descriptor_loop),
            "")

        # pack associaton_loop
        al_bytes = string.join(
            map(lambda x: x.pack(),
                self.association_loop),
            "")

	pdl_bytes_length = len(pdl_bytes)
        self.table_id_extension = self.action_type << 8 | self.platform_id_hash

        fmt = "!BBBBBB%ds%ds" % (len(pdl_bytes), len(al_bytes))
        return pack(fmt,
            (self.platform_id >> 16) & 0xFF,
            (self.platform_id >> 8) & 0xFF,
            self.platform_id & 0xFF,
            self.processing_order,
            0xF0 << 8 | (pdl_bytes_length >> 8) & 0x0F,
	    pdl_bytes_length & 0xFF,
            pdl_bytes,
            al_bytes
            )

class association_loop_item(DVBobject):
  
  def pack(self):
        # pack target descriptor loop
        tdl_bytes = string.join(
            map(lambda x: x.pack(),
                self.target_descriptor_loop),
            "")

        # pack operational descriptor loop
        odl_bytes = string.join(
            map(lambda x: x.pack(),
                self.operational_descriptor_loop),
            "")

	tdl_bytes_length = len(tdl_bytes)
	odl_bytes_length = len(odl_bytes)
	
	fmt = "!BB%dsBB%ds" % (tdl_bytes_length, odl_bytes_length)
	
	return pack(fmt,
		0xF0 << 8 | (tdl_bytes_length >> 8) & 0x0F,
		tdl_bytes_length & 0xFF,
		tdl_bytes,
		0xF0 << 8 | (odl_bytes_length >> 8) & 0x0F,
		odl_bytes_length & 0xFF,
		odl_bytes
		)
		
