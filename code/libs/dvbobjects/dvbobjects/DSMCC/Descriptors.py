#! /usr/bin/env python

# This file is part of the dvbobjects library.
# 
# Copyright 2000-2001, GMD, Sankt Augustin
# -- German National Research Center for Information Technology 
#
# Copyright 2010, LIFIA - Facultad de Informatica - Univ. Nacional de La Plata
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
class data_broadcast_descriptor(Descriptor):

    descriptor_tag    = 0x64
    data_broadcast_id = 0x0007
    text_chars = ""

    def bytes(self):
        self.selector_length = len(self.selector_bytes)
        self.text_length = len(self.text_chars)

        FMT = "!HBB%ds%dsB%ds" % (
            self.selector_length,
            3,                          # ISO_639_language_code
            self.text_length,
            )

        return pack(
            FMT,
            self.data_broadcast_id,
            self.component_tag,
            self.selector_length,
            self.selector_bytes,
            self.ISO_639_language_code,
            self.text_length,
            self.text_chars,
            )

    def sample(self):
        self.set(
            component_tag = 0x11,
            selector_bytes = "012345",
            text_chars = "abcde",
            )


######################################################################
class object_carousel_info(DVBobject):

    carousel_type_id   = 0x02
    transaction_id     = 0xFFFFFFFF
    time_out_value_DSI = 0xFFFFFFFF
    time_out_value_DII = 0xFFFFFFFF
    leak_rate = 0x00

    def pack(self):

        obj_loop_bytes = ""
        for obj_name_chars in self.object_names:
            object_name_length = len(obj_name_chars)
            obj_bytes = pack(
                "!3sB%ds" % object_name_length,
                self.ISO_639_language_code,
                object_name_length,
                obj_name_chars,
                )
            obj_loop_bytes = obj_loop_bytes + obj_bytes
            

        obj_loop_length = len(obj_loop_bytes)

        FMT = "!B4L%ds" % obj_loop_length
        return pack(
            FMT,
            (self.carousel_type_id << 6 | 0x63),
            self.transaction_id,
            self.time_out_value_DSI,
            self.time_out_value_DII,
            (0x0C000000 | self.leak_rate),
            obj_loop_bytes,
            )

    def sample(self):
        self.set(
            object_names = ["aaa", "bbbbbb"]
            )

######################################################################
class stream_event_descriptor(Descriptor):

    descriptor_tag = 0x1a
    
    reserved = 0x0L # TODO Check: Shouldn't this be 0xFFFFFFFEL?
    
    def bytes(self):
        self.text_length = len(self.private_data)

        FMT = "!HLL%ds" % self.text_length

        return pack(
            FMT,
            self.event_id,
            self.reserved | ( self.eventNPT >> 32), # 31 bits reserved, 1 bit NPT_reference
            self.eventNPT & 0xFFFFFFFF,     # 32 bits NPT_reference
            self.private_data,
            )

######################################################################
class stream_event_do_it_now_descriptor(stream_event_descriptor):
    
    eventNPT = 0x0
######################################################################
class stream_event_ginga_descriptor(stream_event_descriptor):

    eventNPT  = 0x0
    
    command_tag = 0x0
    sequence    = 0x0 
    finalFlag   = 0x0
    payload     = ""

    FCS         = 0x00 # TODO calculate FCS.

    def bytes(self):
        FMT = ("!"
            "B" # private data length
            "B" # command_tag
            "B" # sequence_number: 7b, final_flag: 1b
            "%ds"# private data payload
            "B" # FCS
        ) % (len(self.payload))

        self.private_data = pack(FMT,
            3+len(self.payload),
            self.command_tag,
            ((self.sequence_number & 0x7F) << 1) | (self.final_flag & 0x01),
            self.payload,
            self.FCS
        )

        # call superclass implementation.
        return stream_event_descriptor.bytes(self)

######################################################################
class npt_reference_descriptor(Descriptor):

    descriptor_tag = 0x01
    
    def bytes(self):
        self.text_length = len(self.private_data)

        FMT = ( "!"
		"B" # post_continuity_indicator: 1b, content_id: 7b
		"B" # reserved:  7b, STC_reference:  1b - most significant bit
		"L" #                STC_reference: 32b
		"L" # reserved: 31b, NPT_reference:  1b - most significant bit
		"L" #                NPT_reference: 32b
		"H" # scale_numerator  : 16b
		"H")# scale_denominator: 16b

        return pack(
            FMT,
            (self.post_continuity_indicator << 7) | (self.content_id & 0x7F), # 1 bit postcontdesc, 7 bits contentId
            0xFE | ( self.STC_reference >> 32),  # 31 bits reserved, 1 bit STC_reference
            self.STC_reference & 0xFFFFFFFF,     # 32 bits STC_reference
            0xFFFFFFFE | ( self.NPT_reference >> 32), # 31 bits reserved, 1 bit NPT_reference
            self.NPT_reference & 0xFFFFFFFF,     # 32 bits NPT_reference
            self.scale_numerator,
            self.scale_denominator
            )

######################################################################
class npt_endpoint_descriptor(Descriptor):

    descriptor_tag = 0x02
    
    def bytes(self):
        self.text_length = len(self.private_data)

        FMT = ( "!"
		"H" # reserved: 15b, NPT_start:  1b - most significant bit
		"L" #                NPT_start: 32b
		"L" # reserved: 31b, NPT_stop :  1b - most significant bit
		"L")#                NPT_stop : 32b

        return pack(
            FMT,
            0xFFFE | ( self.NPT_start >> 32), # 15 bits reserved, 1 bit NPT_start
            self.NPT_start & 0xFFFFFFFF,      # 32 bits STC_reference
            0xFFFFFFFE | ( self.NPT_stop >> 32), # 31 bits reserved, 1 bit NPT_stop
            self.NPT_stop & 0xFFFFFFFF        # 32 bits NPT_stop
            )

