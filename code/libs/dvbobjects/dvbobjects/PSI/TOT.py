#! /usr/bin/env python

# This file is part of the dvbobjects library.
# 
# Copyright © 2009-2013 Lorenzo Pallara l.pallara@avalpa.com
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
from dvbobjects.utils import *
from dvbobjects.utils.DVBobject import *
from dvbobjects.utils.MJD import *
from dvbobjects.MPEG.Section import Section
from dvbobjects.utils import *
from dvbobjects.DVB.Descriptors import *

######################################################################
class time_offset_section(DVBobject):
	
	def pack(self):
	
		date = MJD_convert(self.year, self.month, self.day)
		# pack service_stream_loop
		tl_bytes = string.join(
			map(lambda x: x.pack(),
			self.descriptor_loop),
			"")
		
		fmt = "!BHHBBBH%ds" % len(tl_bytes) 
		data = pack(fmt,
			0x73,
			0x7000 | ((len(tl_bytes) + 11) & 0xFFF),
			date,
			self.hour,
			self.minute,
			self.second,
			0xF000 | (len(tl_bytes) & 0xFFF),
			tl_bytes
		)
		return data + self.crc_32(data)
	
	def crc_32(self, data):
		crc = crc32.CRC_32(data)
		return pack("!L", crc)
