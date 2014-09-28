#! /usr/bin/env python

# This file is part of the dvbobjects library.
# 
# Copyright 2009-2013 Lorenzo Pallara l.pallara@avalpa.com
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
class update_notification_section(Section):

    table_id = 0x4B
    
    section_max_size = 4096

    def pack_section_body(self):
    
        self.table_id_extension = self.action_type << 8 | ((self.OUI >> 16) ^ ((self.OUI >> 8) & 0xFF) ^ (self.OUI & 0xFF))
        
        # pack common_descriptor_loop
        common_bytes = string.join(
            map(lambda x: x.pack(),
                self.common_descriptor_loop),
            "")

        # pack compatibility_descriptor_loop
        compatibility_bytes = string.join(
            map(lambda x: x.pack(),
                self.compatibility_descriptor_loop),
            "")

        fmt = "!HBBH%ds%ds" % (len(common_bytes), len(compatibility_bytes))
        return pack(fmt,
    	    self.OUI >> 8,
    	    self.OUI & 0xFF,
    	    self.processing_order,
    	    0xF000 | len(common_bytes),
    	    common_bytes,
    	    compatibility_bytes,
            )

######################################################################
class unt_compatibility_descriptor_loop_item(DVBobject):

    def pack(self):
    
        # pack target_descriptor_loop
        tdl_bytes = string.join(
            map(lambda x: x.pack(),
                self.target_descriptor_loop),
            "")
            
        # pack operational descriptor_loop
        odl_bytes = string.join(
            map(lambda x: x.pack(),
                self.operational_descriptor_loop),
            "")
        
        fmt = "!%dsHH%dsH%ds" % (len(self.compatibility_descriptor), len(tdl_bytes), len(odl_bytes))
        return pack(fmt,
    		    self.compatibility_descriptor,
    		    len(tdl_bytes) + len(odl_bytes),
    		    0xF000 | len(tdl_bytes),
                    tdl_bytes,
                    0xF000 | len(odl_bytes),
                    odl_bytes,
                    )
