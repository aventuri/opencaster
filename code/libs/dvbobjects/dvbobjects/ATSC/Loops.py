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

######################################################################

class segment_loop_item(DVBobject):

    def pack(self):
	
	fmt = "!BBB%ds" % len(self.compressed_string)
	return pack(fmt,
		self.compression_type,
		self.mode,
		len(self.compressed_string),
		self.compressed_string,
		)



######################################################################

class string_loop_item(DVBobject):

    def pack(self):
	assert len(self.ISO639_language_code) == 3
	
	data_bytes = string.join(
		map(lambda x: x.pack(),
		self.segment_loop),
		"")

	self.number_segments = len(self.segment_loop)
	
	fmt = "!%dsB%ds" % (len(self.ISO639_language_code), len(data_bytes))
	return pack(fmt,
		self.ISO639_language_code,
		self.number_segments,
		data_bytes
		)


######################################################################

class multiple_string_structure(DVBobject):

    def pack(self):
	
	
	data_bytes = string.join(
		map(lambda x: x.pack(),
		self.string_loop),
		"")

	self.number_strings = len(self.string_loop)
	
	fmt = "!B%ds" % len(data_bytes)
	return pack(fmt,
		self.number_strings,
		data_bytes,
		)
	