#! /usr/bin/env python

# This file is part of the dvbobjects library.
# 
# Copyright © 2008-2013, Lorenzo Pallara l.pallara@avalpa.com
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
class entitlement_control_message_section(Section):

    table_id = 0x80
    
    section_max_size = 1024

    def pack_section_body(self):
    
        # pack program_loop_item
        pl_bytes = string.join(
            map(lambda x: x.pack(),
                self.ecm_loop),
            "")

        self.table_id_extension = 0xFFFF

        fmt = "!%ds" % (len(pl_bytes))
        return pack(fmt,
            pl_bytes
            )

######################################################################
class ecm_loop_item(DVBobject):

    def pack(self):
    
        # pack program_loop_item
        fmt = "!HHHHHHHH"
	return pack(fmt,
	    self.cw1_1,
	    self.cw1_2,
	    self.cw1_3,
	    self.cw1_4,
	    self.cw2_1,
	    self.cw2_2,
	    self.cw2_3,
	    self.cw2_4,
	)
