#! /usr/bin/env python

# This file is part of the dvbobjects library.
#
# Copyright (C) 2004-2013, Lorenzo Pallara l.pallara@avalpa.com
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


class program_association_section(Section):

    table_id = 0x00

    section_max_size = 1024

    def pack_section_body(self):

        # pack program_loop_item
        pl_bytes = b''.join(
            [x.pack() for x in self.program_loop])

        self.table_id_extension = self.transport_stream_id  # ???
        self.private_indicator = 0

        fmt = "!%ds" % (len(pl_bytes))
        return pack(fmt,
                    pl_bytes
                    )

######################################################################


class program_loop_item(DVBobject):

    def pack(self):

        # pack program_loop_item
        fmt = "!HH"
        return pack(fmt,
                    self.program_number,
                    0xE000 | (self.PID & 0x1FFF),
                    )
