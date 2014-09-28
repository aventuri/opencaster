#! /usr/bin/env python

# This file is part of the dvbobjects library.
# 
# Copyright  2009-2013, Lorenzo Pallara l.pallara@avalpa.com
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
from dvbobjects.utils.Hamming import *
from dvbobjects.utils.ByteParity import *
from dvbobjects.utils.ByteInvert import *

######################################################################
class EBUTeletext(DVBobject):
	"""The class for EBU Teletext data units, NB. they are not sections
	"""
	def pack(self):
		if (self.data_unit_id == 0xFF):
#			return pack("!BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB", 
			return pack("BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB", 
				    0xFF, 0x2C, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
				    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
				    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
				    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
				    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
				    )
		b3 = self.row & 0x1 
		b2 = (self.magazine >> 2) & 0x01  
		b1 = (self.magazine >> 1) & 0x01
		b0 = self.magazine & 0x1
		hamming1 = invert(hamming84((b3 << 3) | (b2 << 2) | (b1 << 1) | b0))
		b3 = (self.row >> 4) & 0x01
		b2 = (self.row >> 3) & 0x01
		b1 = (self.row >> 2) & 0x01
		b0 = (self.row >> 1) & 0x01
		hamming2 = invert(hamming84((b3 << 3) | (b2 << 2) | (b1 << 1) | b0))
		payload = ""
		if (self.row == 0) :
			for i in range(0, len(self.chars)):
				#temp1 = ord(self.chars[i])
				#temp2 = oddparity(temp1)
				#temp3 = invert(temp2)
				#temp4 = temp3 & 0xFF
				#payload += pack("B", temp4)
				#payload += pack("!B", invert(oddparity(ord(self.chars[i]))))
				payload += pack("B", invert(oddparity(ord(self.chars[i]))))
			assert len(payload) == 32
			hamming3 = invert(hamming84(self.page & 0xF))
			hamming4 = invert(hamming84((self.page >> 4) & 0xF))
			hamming5 = invert(hamming84(self.subpage & 0xF))
			b3 = self.erase_page & 0x1
			b2 = (self.subpage >> 6 ) & 0x1
			b1 = (self.subpage >> 5 ) & 0x1
			b0 = (self.subpage >> 4 ) & 0x1
			hamming6 = invert(hamming84((b3 << 3) | (b2 << 2) | (b1 << 1) | b0))
			b3 = (self.subpage >> 10 ) & 0x1
			b2 = (self.subpage >> 9 ) & 0x1
			b1 = (self.subpage >> 8 ) & 0x1
			b0 = (self.subpage >> 7 ) & 0x1
			hamming7 = invert(hamming84((b3 << 3) | (b2 << 2) | (b1 << 1) | b0))
			b3 = self.subtitle & 0x1
			b2 = self.newsflash & 0x1
			b1 = (self.subpage >> 12 ) & 0x1
			b0 = (self.subpage >> 11 ) & 0x1
			hamming8 = invert(hamming84((b3 << 3) | (b2 << 2) | (b1 << 1) | b0))
			b3 = self.inhibit_display & 0x1
			b2 = self.interrupted_sequence & 0x1
			b1 = self.update_indicator & 0x1
			b0 = self.suppress_header & 0x1
			hamming9 = invert(hamming84((b3 << 3) | (b2 << 2) | (b1 << 1) | b0))
			b3 = self.country_code & 0x1
			b2 = (self.country_code >> 1) & 0x1
			b1 = (self.country_code >> 2) & 0x1
			b0 = self.magazine_serial & 0x1
			hamming10 = invert(hamming84((b3 << 3) | (b2 << 2) | (b1 << 1) | b0))
#			return pack("!BBBBBBBBBBBBBB",
			return pack("BBBBBBBBBBBBBB",
				self.data_unit_id, #0x02 non-subtile or 0x03 for subtitles
				0x2C, # 44 bytes
				0x01 << 6 | (self.field_parity & 0x01) << 5 | self.line_offset & 0x1F,
				0xE4, # frame coding EBU Teletext
				hamming1,
				hamming2,
				hamming3,
				hamming4,
				hamming5,
				hamming6,
				hamming7,
				hamming8,
				hamming9,
				hamming10
			) + payload
		elif (self.row == 27) :
			link1bytes = self.link1.pack()
			link2bytes = self.link2.pack()
			link3bytes = self.link3.pack()
			link4bytes = self.link4.pack()
			link5bytes = self.link5.pack()
			link6bytes = self.link6.pack()
			crc = 0x0000 # not used 
#			fmt = "!BBBBBBB%ds%ds%ds%ds%ds%dsBH" % (len(link1bytes), len(link2bytes), len(link3bytes), len(link4bytes), len(link5bytes), len(link6bytes))
			fmt = "BBBBBBB%ds%ds%ds%ds%ds%dsBH" % (len(link1bytes), len(link2bytes), len(link3bytes), len(link4bytes), len(link5bytes), len(link6bytes))
			return pack(fmt,
				self.data_unit_id, #0x02 non-subtile or 0x03 for subtitles
				0x2C, # 44 bytes
				0x01 << 6 | (self.field_parity & 0x01) << 5 | self.line_offset & 0x1F,
				0xE4, # frame coding EBU Teletext
				hamming1,
				hamming2,
				invert(hamming84(self.designation_code)), # designation code allows 2^4 = 16 different pages more
				link1bytes,
				link2bytes,
				link3bytes,
				link4bytes,
				link5bytes,
				link6bytes,
				invert(hamming84(0xE)), # LC
				crc
			)
		else :
			for i in range(0, len(self.chars)):
#				payload += pack("!B", invert(oddparity(ord(self.chars[i]))))
				payload += pack("B", invert(oddparity(ord(self.chars[i]))))
			assert len(payload) == 40
#			return pack("!BBBBBB",
			return pack("BBBBBB",
				self.data_unit_id, #0x02 non-subtile or 0x03 for subtitles
				0x2C, # 44 bytes
				0x01 << 6 | (self.field_parity & 0x01) << 5 | self.line_offset & 0x1F,
				0xE4, # frame coding EBU Teletext
				hamming1,
				hamming2,
			) + payload # 40 characters of a line, a page is 24 lines
		
		
######################################################################
class EBUTeletextUnits(DVBobject):
	def pack(self):
	
		# pack unit_loop
		pl_bytes = string.join(
			map(lambda x: x.pack(),
			self.unit_loop),
			"")
		fmt = "!%ds" % (len(pl_bytes))
		return pack(fmt, 
			pl_bytes
		)

class EBUTeletextLink(DVBobject):
	def pack(self):
		hamming1 = invert(hamming84(self.page & 0xF))
		hamming2 = invert(hamming84((self.page >> 4) & 0xF))
		hamming3 = invert(hamming84(self.subpage & 0xF))
		b3 = not (self.magazine & 0x1)
		b2 = (self.subpage >> 6 ) & 0x1
		b1 = (self.subpage >> 5 ) & 0x1
		b0 = (self.subpage >> 4 ) & 0x1
		hamming4 = invert(hamming84((b3 << 3) | (b2 << 2) | (b1 << 1) | b0))
		b3 = (self.subpage >> 10 ) & 0x1
		b2 = (self.subpage >> 9 ) & 0x1
		b1 = (self.subpage >> 8 ) & 0x1
		b0 = (self.subpage >> 7 ) & 0x1
		hamming5 = invert(hamming84((b3 << 3) | (b2 << 2) | (b1 << 1) | b0))
		b3 = not ((self.magazine >> 1) & 0x1)
		b2 = not ((self.magazine >> 2) & 0x1)
		b1 = (self.subpage >> 12 ) & 0x1
		b0 = (self.subpage >> 11 ) & 0x1
		hamming6 = invert(hamming84((b3 << 3) | (b2 << 2) | (b1 << 1) | b0))
#		fmt = "!BBBBBB" 
		fmt = "BBBBBB" 
		return pack(fmt, 
			hamming1,
			hamming2,
			hamming3,
			hamming4,
			hamming5,
			hamming6,
		)

