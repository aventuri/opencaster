#! /usr/bin/env python

# This file is part of the dvbobjects library.
# 
# Copyright © 2008, Lorenzo Pallara
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
class conditional_access_section(Section):

    table_id = 0x01
    
    section_max_size = 1024

    def pack_section_body(self):
    
        # pack ca_descriptor_loop
        pl_bytes = string.join(
            map(lambda x: x.pack(),
                self.ca_descriptor_loop),
            "")

        self.table_id_extension = 0xFFFF

        fmt = "!%ds" % (len(pl_bytes))
        return pack(fmt,
            pl_bytes
            )

