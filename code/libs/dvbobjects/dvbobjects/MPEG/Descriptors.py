#! /usr/bin/env python

#
# Copyright (C) 2004-2013  Lorenzo Pallara, l.pallara@avalpa.com
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
from dvbobjects.MPEG.Descriptor import Descriptor

######################################################################
class STD_descriptor(Descriptor):

    descriptor_tag = 0x11

    def bytes(self):
        fmt = "!B"
        return pack(fmt,
                    0xFE | self.leal_valid_flag,
                    )
		    
######################################################################
class video_stream_descriptor(Descriptor):

    descriptor_tag = 0x2
    
    def bytes(self):
	if self.MPEG_1_only_flag == 0:
	    fmt = "!B"
	    return pack(fmt,
		(self.multiple_frame_rate_flag & 0x1) << 7 |
		(self.frame_rate_code & 0x3) << 3 |
		(self.MPEG_1_only_flag & 0x1) << 2 |
		(self.constrained_parameter_flag & 0x1) << 1 |
		(self.still_picture_flag & 0x1),
	    )
	else :
	    fmt = "!BBB"
	    return pack(fmt,
		(self.multiple_frame_rate_flag & 0x1) << 7 | 
		(self.frame_rate_core & 0xF) << 3 | 
		(self.MPEG_1_only_flag & 0x1) << 2 | 
		(self.constrained_parameter_flag & 0x1) << 1 | 
		(self.still_picture_flag & 0x1),
		profile_and_level_indication,
		(chroma_format & 0x3) << 6 | 
		(frame_rate_extension_flag & 0x1) << 5 |
		0x1F,
	    )
							    

######################################################################
class audio_stream_descriptor(Descriptor):

    descriptor_tag = 0x3
    
    def bytes(self):
	fmt = "!B"
	return pack(fmt,
		(self.free_format_flag & 0x1) << 7 |
		(self.ID & 0x1) << 6 |
		(self.layer & 0x3) << 4 |
		(self.variable_rate_audio_indicator & 0x1) << 3,
	)

######################################################################
class association_tag_descriptor(Descriptor):

    descriptor_tag = 0x14

    def bytes(self):
	if self.use == 0:
    	    fmt = "!HHBLL%ds" % (len(self.private_data))
    	    return pack(fmt,    
		    self.association_tag,
		    self.use,
		    self.selector_lenght,
		    self.transaction_id,
		    self.timeout,
		    self.private_data,		    
                    )		
	else:
    	    fmt = "!HHB%ds%ds" % (
		    len(self.selector_lenght), 
		    len(self.privatedata)
		    )	    
	    return pack(fmt,
                    self.association_tag,
		    self.use,
		    self.selector_lenght,
		    self.selector_bytes,
		    self.privatedata,
		    )

######################################################################
class graphics_constraints_descriptor(Descriptor):

    descriptor_tag = 0x14
    
    def bytes(self):
	gc_bytes = string.join(
	map(lambda byte, self=self: pack("!B", byte),
		self.graphics_configuration_bytes),
		"")
		
	fmt = "!B%ds" % (len(gc_bytes))
	return pack(fmt,
		0xF8 |
		(self.can_run_without_visible_ui << 2) |
		(self.handles_configuration_changed << 1) |
		(self.handles_externally_controlled_video),
		gc_bytes,
	)

######################################################################
class DTS_registration_descriptor(Descriptor):

    descriptor_tag = 0x05

    def bytes (self):
	fmt = "!%ds" % (len(self.format_identifier))
	return pack(fmt, self.format_identifier)

######################################################################
class carousel_identifier_descriptor(Descriptor):

    descriptor_tag = 0x13

    def bytes(self):
	if self.format_ID:
        	fmt = "!LBBHHHBLBB%ds%ds" % (len(self.object_key_data) , len(self.private_data))
        	return pack(fmt,
    			self.carousel_ID,
	        	self.format_ID,
			self.module_version,
			self.module_ID,
			self.block_size,
			self.module_size,
			self.compression_method,
			self.original_size,
			self.timeout,
			len(self.object_key_data),
			self.object_key_data,
			self.private_data,
			)
	else :
                fmt = "!LB%ds" % (len(self.private_data))
                return pack(fmt,
                        self.carousel_ID,
                        self.format_ID,
                        self.private_data,
                        )
			
######################################################################							
class ca_descriptor(Descriptor):

    descriptor_tag = 0x9
    
    def bytes(self):
        fmt = "!HH"
        return pack(fmt,
            self.CA_system_ID,
            0xE000 | (self.CA_PID & 0x1FFF),
        )
        
