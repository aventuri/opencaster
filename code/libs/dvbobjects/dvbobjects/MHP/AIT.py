#! /usr/bin/env python

# This file is part of the dvbobjects library.
# 
# Copyright  2000-2001, GMD, Sankt Augustin
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

import string
from dvbobjects.MPEG.Section import Section
from dvbobjects.utils import *

######################################################################
class application_information_section(Section):

    table_id = 0x74
    
    section_max_size = 1024
    

    def pack_section_body(self):
    
        self.private_indicator = 1

        # pack common_descriptor_loop
        cdl_bytes = string.join(
            map(lambda x: x.pack(),
                self.common_descriptor_loop),
            "")

        # pack applicaton_loop
        apl_bytes = string.join(
            map(lambda x: x.pack(),
                self.application_loop),
            "")

        self.table_id_extension = self.application_type # ???

        fmt = "!H%dsH%ds" % (len(cdl_bytes), len(apl_bytes))
        return pack(fmt,
            0xF000 | (len(cdl_bytes) & 0x0FFF),
            cdl_bytes,
            0xF000 | (len(apl_bytes) & 0x0FFF),
            apl_bytes,
            )

######################################################################
class application_loop_item(DVBobject):

    def pack(self):
    
        # pack application_descriptors_loop
        adl_bytes = string.join(
            map(lambda x: x.pack(),
                self.application_descriptors_loop),
            "")

        fmt = "!LHBH%ds" % len(adl_bytes)
        return pack(fmt,
                    self.organisation_id,
                    self.application_id,
                    self.application_control_code,
                    0xF000 | (len(adl_bytes) & 0x0FFF),
                    adl_bytes,
                    )
