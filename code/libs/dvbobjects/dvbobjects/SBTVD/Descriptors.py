#! /usr/bin/env python
#
# Copyright (C) 2010 Robson Tovo <robson.tovo@gmail.com>,
#                    Marco Casaroli <marco.casaroli@gmail.com>
#					 LIFIA - Facultad de Informatica - Univ. Nacional de La Plata
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
class tds_frequency_item(DVBobject):

    def pack(self):
        self.freq7 = (self.freq * 7) + 1
        fmt = "!H"
        return pack(fmt, self.freq7)

class terrestrial_delivery_system_descriptor(Descriptor):

    descriptor_tag = 0xfa

    def bytes(self):
        frequency_bytes = string.join(
                map(lambda x: x.pack(), 
                self.frequencies),
                "")
            
        fmt = "!H%ds" % len(frequency_bytes)
        return pack(fmt, ((self.area_code & 0xFFF) << 4) | ((self.guard_interval & 0x3) << 2) | (self.transmission_mode & 0x3), frequency_bytes)

######################################################################
# As specified by ARIB
class partial_reception_descriptor(Descriptor):

    descriptor_tag = 0xfb

    def bytes(self):
	sid_pack = string.join(map (lambda x: pack("!H", x), self.service_ids), "")

	fmt = "!%ds" % (len(sid_pack))
        return pack(fmt, sid_pack)

######################################################################
# As specified by ARIB
class system_management_descriptor(Descriptor):

    descriptor_tag = 0xfe

    def bytes(self):
        sys_man_id = (
            (self.broadcasting_flag << 14) |
            (self.broadcasting_identifier << 8) |
            (self.additional_broadcasting_identification)
        )

        additional_id_info_pack = string.join(
            map (lambda x: pack("!B", x), self.additional_identification_bytes),
            ""
        )
        fmt = "!H%ds" % len(additional_id_info_pack)

        return pack(fmt, sys_man_id, additional_id_info_pack)

######################################################################
class data_component_descriptor(Descriptor):

    descriptor_tag = 0xfd

    def bytes(self):
        fmt = "!H%ds" % len(self.additional_data_component_info)
        return pack(fmt,
            self.data_component_id,
            self.additional_data_component_info,
        )

######################################################################
class service_id_loop_item(DVBobject):
    def pack(self):
        fmt = "!H"
        return pack(fmt, self.service_id)

class transmission_type_loop_item(DVBobject):

    def pack(self):
        num_of_service = len(self.service_id_loop)
        service_id_loop_cat = string.join(
            map(
                lambda x: x.pack(),
                self.service_id_loop
            ), ""
        )
    
        fmt = "!BB%ds" % len(service_id_loop_cat)
        return pack(fmt,
            self.transmission_type_info,
            num_of_service,
            service_id_loop_cat,
        )

class transport_stream_information_descriptor(Descriptor):

    descriptor_tag = 0xcd

    def bytes(self):

        name_len = len(self.ts_name)
        ts_type_count = len(self.transmission_type_loop)
        transmission_type_loop_cat = string.join(
            map(
                lambda x: x.pack(),
                self.transmission_type_loop
            ), ""
        )

        fmt = "!BB%ds%ds" % (name_len, len(transmission_type_loop_cat))
        return pack(fmt,
            self.remote_control_key_id,
            (name_len << 2) | ts_type_count,
            self.ts_name,
            transmission_type_loop_cat
        )

######################################################################

class video_decode_control_descriptor(Descriptor):
    descriptor_tag = 0xc8

    def bytes(self):
        return pack("!B",
            (self.still_picture_flag << 7) |
            (self.sequence_end_code_flag << 6) |
            (self.video_encode_format << 2) |
            0x03
        )

######################################################################

class ts_loop_item(DVBobject):
    #tx_type_info
    # svc_loop = []
    def pack(self):
        svc_bytes = "".join(map(lambda x: chr(x>>8 & 0xFF) + chr(x&0xFF), self.svc_loop))
        fmt = "!BB%ds" % len(svc_bytes)
        return pack(fmt, self.tx_type_info, len(self.svc_loop), svc_bytes)

class ts_info_descriptor(Descriptor):
    descriptor_tag = 0xcd
    # rc_key
    # tsname
    #ts_loop = []
    def pack(self):
        ts_loop_bytes = "".join(map(lambda x: x.pack(), self.ts_loop))
        fmt = "!BBBB%ds%ds" % (len(self.tsname), len(ts_loop_bytes))
#	print len(ts_loop_bytes)
        return pack(fmt, self.descriptor_tag, len(self.tsname) + len(ts_loop_bytes) + 2, self.rc_key, len(self.tsname)<<2 | len(self.ts_loop) & 3, self.tsname, ts_loop_bytes)
	
