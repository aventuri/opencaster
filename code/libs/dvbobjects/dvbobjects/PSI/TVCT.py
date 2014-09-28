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
from dvbobjects.utils import *
from dvbobjects.MPEG.Section import Section
from dvbobjects.utils.DVBobject import *

######################################################################
class terrestrial_virtual_channel_section(Section):

	table_id = 0xC8
	section_max_size = 1024

	def pack_section_body(self):

		self.table_id_extension = self.transport_stream_id
	
		# pack event_loop
		cl_bytes = string.join(
			map(lambda x: x.pack(),
			self.channels_loop),
		"")
		
		adl_bytes = string.join(
			map(lambda x: x.pack(),
			self.additional_descriptors_loop),
		"")

		fmt = "!BB%dsH%ds" % (len(cl_bytes), len(adl_bytes))
		return pack(fmt,
			self.ATSC_protocol_version,
			len(self.channels_loop),
			cl_bytes,
			0xFC00 | (len(adl_bytes) & 0x3FF),
			adl_bytes
		)

######################################################################
class channel_loop_item(DVBobject):

	def pack(self):
	
		assert len(self.short_name) == 14 #bytes -> 7*16 bit, 7 utf-16 chars
	
	# pack event_descriptor_loop
		dl_bytes = string.join(
			map(lambda x: x.pack(),
			self.descriptors_loop),
			"")

		fmt = "!%dsLLHHBBHH%ds" % (len(self.short_name), len(dl_bytes))
		return pack(fmt,
			self.short_name,
			0xF0000000 | ((self.major_channel_number & 0x3FF) << 18) | ((self.minor_channel_number & 0x3FF) << 8) | (self.modulation_mode & 0xFF),
			self.carrier_frequency,
			self.channel_TSID,
			self.program_number,
			(self.ETM_location << 6) | (self.access_controlled << 5) | (self.hidden << 4) | 0xC | (self.hide_guide << 1) | 0x1,
			0xC0 | self.service_type,
			self.source_id,
			0xFC00 | (len(dl_bytes) & 0x3FF),
			dl_bytes
			)
