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
from dvbobjects.utils.DVBobject import *
from dvbobjects.MPEG.Descriptor import *
from dvbobjects.MPEG.Section import Section
from dvbobjects.utils.MJD import *

# @author Ignacio Jaureguiberry <ijaureguiberry@lifia.info.unlp.edu.ar>

# TODO Lots of testing, most classes needs to be modified
#      so that they work with default values.
# TODO Clean up a bit.

#
# SDTT as defined in ARIB STD-B21 - ver 4.6-E1 - Table 12-1
#
class software_download_trigger_table(Section):
    
    table_id = 0xC3

    def pack_section_body(self):

        self.table_id_extension = (self.maker_id << 8) | (self.model_id & 0xff)

        content_bytes = string.join(
            map(lambda x: x.pack(),
                self.contents),
            ""
        )

        fmt = ("!"
            "H" # transport_stream_id: 16 bits
            "H" # original_network_id: 16
            "H" # service_id: 16
            "B" # number_of_contents: 8
            "%ds" # contents()
              % len(content_bytes))
        return pack (fmt,
            self.transport_stream_id,
            self.original_network_id,
            self.service_id,
            len(self.contents), # number of contents, should be length in bytes instead of qty.??
            content_bytes)

class sdtt_content_loop_item(DVBobject):
        
    def pack(self):
        schedule_loop_bytes = string.join(
            map(lambda x: x.pack(), self.schedule_loop_items), ""
        )
        descriptor_loop_bytes = string.join(
            map(lambda x: x.pack(), self.descriptors), ""
        )

        fmt = ("!"
            "H" # group: 4 bits, target_version: 12
            "H" # new_version: 12, download_level: 2, version_indicator: 2
            "H" # content_descriptor_length: 12, reserved: 4
            "H" # schedule_descriptor_length: 12, schedule_timeshift_information: 4
            "%ds" # loop: start_time: 40, duration: 24
            "%ds" # loop: descriptors
            % (len(schedule_loop_bytes),len(descriptor_loop_bytes)))
        return pack(fmt,
            (self.group       << 12) | (self.target_version),
            (self.new_version <<  4) | (self.download_level << 2) | self.version_indicator,
            ( (len(schedule_loop_bytes)+len(descriptor_loop_bytes)) << 4) | 0x0F,
            ( (len(schedule_loop_bytes)) << 4) | self.schedule_timeshift_information,
            schedule_loop_bytes,
            descriptor_loop_bytes)


class sdtt_schedule_loop_item(DVBobject):
    def pack(self):
        fmt = ("!HBBBBBB")
        return pack(fmt,
            int(MJD_convert(self.start_year, self.start_month, self.start_day)),
            self.start_hour,
            self.start_minute,
            self.start_second,
            self.duration_hours,
            self.duration_minutes,
            self.duration_seconds,
        )

class download_content_descriptor(Descriptor):

    descriptor_tag = 0xC9

    def has(self, bytes):
        if len(bytes) > 0:
            return 1
        return 0

    def bytes(self):
        # TODO clean up...
        if self.compatibility_descriptor_bytes == None:
            self.compatibility_descriptor_bytes = "\000\000"
        if self.modules_info_bytes == None:
            self.modules_info_bytes             = ""

        if len(self.text_data) > 0 and len(self.text_ISO639_lang) == 3:
            text_data_bytes = pack(
                "3sB%ds" % len(self.text_data),
                self.text_ISO639_lang,
                len(self.text_data), self.text_data
            )
        else:
            text_data_bytes = ""

        fmt = ("!"
            "B" # reboot:1, add_on:1, compatibility_flag=1, module_info_flag=1, text_info_flag=1, reserved=3
            "L" # component_size:32
            "L" # download_id: 32
            "L" # time_out_value_DII: 32
            "BH" # leak_rate: 22, reserved: 2
            "B" # component_tag: 8
            "H%ds" # compatibility_descriptor: if compatibility_flag=1
            "%ds" # modules_info: if module_info_flag=1
            "B" # private_data_length: 8
            "%ds" # private_data_bytes
            "%ds" # text_data_bytes: if text_info_flag=1
            % (len(self.compatibility_descriptor_bytes),
            len (self.modules_info_bytes),
            len (self.privateData),
            len (text_data_bytes)
            ))
        return pack(
            fmt,
                (self.reboot << 7) |
                (self.add_on << 6) |
                (self.has(self.compatibility_descriptor_bytes)) << 5 |
                (self.has(self.modules_info_bytes)) << 4 |
                (self.has(text_data_bytes)) << 3 |
                0x07, # 3x1 bit for reserved field.
                self.component_size, # sum of data sizes transmitted by the carousel in bytes.
                self.download_id,     # same as the one transmitted in DII/DDB
                self.time_out_value_DII, # timeout value in ms for all DII sections of the carousel.
                self.leak_rate >> 14, ((self.leak_rate << 2) | 0x3)&0xFFFF, # leak_rate. # TODO CHECK its ok...
                self.component_tag,
		len(self.compatibility_descriptor_bytes),
                self.compatibility_descriptor_bytes,
                self.modules_info_bytes,
                len(self.privateData),
                self.privateData,
                text_data_bytes
            )
