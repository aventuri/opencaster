#! /usr/bin/env python

#
# Copyright (C) 2013  Lorenzo Pallara, l.pallara@avalpa.com
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
from dvbobjects.utils.MJD import *
from dvbobjects.MPEG.Descriptor import Descriptor

######################################################################

class rated_dimension_loop_item(DVBobject):

    def pack(self):
	
	fmt = "!BB"
	return pack(fmt,
		self.rating_dimension_j,
		0xF0 | self.rating_value & 0xF
		)

######################################################################

class rating_region_loop_item(DVBobject):

    def pack(self):
	
	self.rated_dimensions = len(self.rated_dimension_loop)
	
	data_bytes = string.join(
		map(lambda x: x.pack(),
		self.rated_dimension_loop),
		"")

	rating_description_bytes = self.rating_description_text.pack()

	fmt = "!BB%dsB%ds" % (len(data_bytes), len(rating_description_bytes))
	return pack(fmt,
		self.rating_region,
		self.rated_dimensions,
		data_bytes,
		len(rating_description_bytes),
		rating_description_bytes
		)

######################################################################
class content_advisory_descriptor(Descriptor):

    descriptor_tag = 0x87

    def bytes(self):
	self.rating_region_count = len(self.rating_region_loop)
	data_bytes = string.join(
		    map(lambda x: x.pack(),
		    self.rating_region_loop),
		    "")
		
	fmt = "!B%ds" % len(data_bytes)
	return pack(fmt,
		(0x3 << 6) | (self.rating_region_count & 0x3F),
		data_bytes,
		)

######################################################################
class AC3_audio_stream_descriptor(Descriptor):

    descriptor_tag = 0x81

