#! /usr/bin/env python

#
# Copyright (C) 2004  Andreas Berger, berger@ftw.at
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

class ip_mac_platform_name_descriptor(Descriptor):
    
    descriptor_tag = 0x0c
    
    def bytes(self):
    
        fmt = "!3s%ds" % len(self.text_char_bytes)
    
        return pack(fmt,
                    self.ISO_639_language_code,
                    self.text_char_bytes
                    )
    
class ip_mac_platform_provider_name_descriptor(Descriptor):
    
    descriptor_tag = 0x0d
    
    def bytes(self):
    
        fmt = "!3s%ds" % len(self.text_char_bytes)
    
        return pack(fmt,
                    self.ISO_639_language_code,
                    self.text_char_bytes
                    )

class target_serial_number_descriptor(Descriptor):
    
    descriptor_tag = 0x08
    
    def bytes(self):
    
        fmt = "!%ds" % len(serial_data_bytes)
    
        return pack(fmt,
                    self.serial_data_bytes
                    )

class target_smartcard_descriptor(Descriptor):
    
    descriptor_tag = 0x06
    
    def bytes(self):
    
        fmt = "!I%ds" % len(self.private_data_bytes)
    
        return pack(fmt,
                    self.super_CA_system_id,
                    self.private_data_bytes
                    )                    

class target_MAC_address_descriptor(Descriptor):
    
    descriptor_tag = 0x07
    
    def bytes(self):
    
        fmt = "!6s%ds" % len(self.mac_addr_bytes)
    
        return pack(fmt,
                    self.mac_addr_mask,
                    self.mac_addr_bytes
                    ) 

class target_MAC_address_range_descriptor(Descriptor):
    
    descriptor_tag = 0x0e
    
    def bytes(self):
    
        fmt = "!6s%ds" % len(self.mac_addr_bytes)
    
        return pack(fmt,
                    self.mac_addr_mask,
                    self.mac_addr_bytes
                    )   

class target_IP_address_descriptor(Descriptor):
    
    descriptor_tag = 0x09
    
    def bytes(self):
    
        fmt = "!I%ds" % len(self.IPv4_addr_bytes)
    
        return pack(fmt,
                    self.IPv4_addr_mask,
                    self.IPv4_addr_bytes
                    )   
 
class target_IP_slash_descriptor(Descriptor):
    
    descriptor_tag = 0x0f
    
    def bytes(self):
    
        fmt = "!4BB"
    
        return pack(fmt,
                    self.IPv4_addr[0],
                    self.IPv4_addr[1],
                    self.IPv4_addr[2],
                    self.IPv4_addr[3],
                    self.IPv4_slash_mask
                    )   

class target_IP_source_slash_descriptor(Descriptor):
    
    descriptor_tag = 0x10
    
    def bytes(self):
    
        fmt = "!%ds" % len(self.IPv4_source_dest_bytes)
    
        return pack(fmt,
                    self.IPv4_source_dest_bytes
                    )   

class target_IPv6_address_descriptor(Descriptor):
    
    descriptor_tag = 0x0a
    
    def bytes(self):
    
        fmt = "!7s%ds" % len(self.IPv6_address_bytes)
    
        return pack(fmt,
                    self.IPv6_address_mask,
                    self.IPv6_address_bytes
                    )   

class target_IPv6_slash_descriptor(Descriptor):
    
    descriptor_tag = 0x11
    
    def bytes(self):
    
        fmt = "!%ds" % len(self.IPv6_bytes)
    
        return pack(fmt,
                    self.IPv6_bytes
                    )   

class target_IPv6_source_slash_descriptor(Descriptor):
    
    descriptor_tag = 0x12
    
    def bytes(self):
    
        fmt = "!%ds" % len(self.IPv6_source_dest_bytes)
    
        return pack(fmt,
                    self.IPv6_source_dest_bytes
                    )   

class ip_mac_stream_location_descriptor(Descriptor):
    
    descriptor_tag = 0x13
    
    def bytes(self):
    
        fmt = "!HHHHB"
    
        return pack(fmt,
                    self.network_id,
                    self.original_network_id,
                    self.transport_stream_id,
                    self.service_id,
                    self.component_tag
                    )

class isp_access_mode_descriptor(Descriptor):
    
    descriptor_tag = 0x14
    
    def bytes(self):
    
        fmt = "!B"
    
        return pack(fmt,
                    self.access_mode
                    )

class telephone_descriptor(Descriptor):
    
    descriptor_tag = 0x57
    
    def bytes(self):
    
        fmt = "!BBBB%ds%ds%ds%ds%ds" % (len(country_prefix_bytes), len(international_area_code_bytes), len(operator_code_bytes), len(national_area_code_bytes), len(core_number_bytes))
    
        return pack(fmt,
                    (0x03 << 7) & 0xC0 | (self.foreign_availability << 5) & 0x20 | self.connection_type & 0x1F,
                    (0x01 << 7) & 0x80 | (self.country_prefix_length << 5) & 0x60 | (self.international_area_code << 4) & 0x1C | self.operator_code_length & 0x07,
                    (0x01 << 7) & 0x80 | (self.national_area_code_length << 4) & 0x70 | self.core_number_length & 0x0F, 
                    country_prefix_bytes,
                    international_area_code_bytes,
                    operator_code_bytes,
                    national_area_code_bytes,
                    core_number_bytes
                    )
  
class private_data_specifier_descriptor(Descriptor):
    
    descriptor_tag = 0x5f
    
    def bytes(self):
    
        fmt = "!I"
    
        return pack(fmt,
                    self.private_data_specifier
                    )

class time_slice_fec_identifier_descriptor(Descriptor):
    
    descriptor_tag = 0x77

    def bytes(self):

    	time_slice_fec_id = 0x00;

        fmt = "!BBB"

        return pack(fmt,
                    (self.time_slicing << 7) & 0x80 | (self.mpe_fec << 5) & 0x60 | (0x03 << 3) & 0x18 | self.frame_size & 0x07,
                    self.max_burst_duration,
                    (self.max_average_rate << 4) & 0xF0 | time_slice_fec_id & 0x0F,
                    )

# FIXME: move this class to another file, it's no descriptor
class platform_id_data2(DVBobject):
  
  def pack(self):
	fmt = "!BBBBB"

	return pack(fmt,
		    (self.platform_id >> 16) & 0xFF,
		    (self.platform_id >> 8) & 0xFF,
		    self.platform_id  & 0xFF,
		    self.action_type & 0xFF,
		    (0x03 << 6) & 0xC0 | (0x00 << 5) & 0x20 | 0x01 & 0x1F
		    )
    

# FIXME: move this class to another file, it's no descriptor
class ip_mac_notification_info(DVBobject):

  def pack(self):
	  
    # pack platform id data loop
    pid_bytes = string.join(
      map(lambda x: x.pack(),
	self.platform_id_data_loop),
      "")

    platform_id_data_length = len(pid_bytes);

    fmt = "!B%ds%ds" % (platform_id_data_length, len(self.private_data_bytes))

    return pack(fmt,
		platform_id_data_length,
		pid_bytes,
		self.private_data_bytes
		)

# FIXME: move this class to another file, it's no descriptor
class platform_name(DVBobject):
  
  def pack(self):
	platform_name_length = len(self.text_char_bytes)

	fmt = "!3sB%ds" % platform_name_length

	return pack(fmt,
		    self.ISO_639_language_code,
		    platform_name_length,
		    self.text_char_bytes
		    )

# FIXME: move this class to another file, it's no descriptor
class platform_id_data(DVBobject):

  def pack(self):
	  
	pn_bytes = string.join(
	  map(lambda x: x.pack(),
	    self.platform_name_loop),
          "")

	platform_name_loop_length = len(pn_bytes)

	fmt = "!BBBB%ds" % platform_name_loop_length

	return pack(fmt,
		    (self.platform_id >> 16) & 0xFF,
		    (self.platform_id >> 8) & 0xFF,
		    self.platform_id  & 0xFF,
		    platform_name_loop_length,
		    pn_bytes
		    )
  
class linkage_descriptor(Descriptor):
    
    descriptor_tag = 0x4A;

    def bytes(self):

	if (self.linkage_type == 0x0B):
        
	  # pack platform id data loop
	  pid_bytes = string.join(
          map(lambda x: x.pack(),
	    self.platform_id_data_loop),
          "")

	  platform_id_data_length = len(pid_bytes);

	  fmt = "!BBBBBBBB%ds%ds" % (platform_id_data_length, len(self.private_data_bytes))

	  return pack(fmt,
		    (self.transport_stream_id >> 8) & 0xFF,
		    self.transport_stream_id & 0xFF,
		    (self.original_network_id >> 8) & 0xFF,
		    self.original_network_id & 0xFF,
		    (self.service_id >> 8) & 0xFF,
		    self.service_id & 0xFF,
		    self.linkage_type,
		    platform_id_data_length,
		    pid_bytes,
		    self.private_data_bytes
		    )
	else:
	  fmt = "!BBBBBBB%ds"

	  # we care only for linkage_type = 0x0B, other linkage descriptors
	  # have to be implemented according to ETSI EN 300 468 standard

	  return pack(fmt,
		    (self.transport_stream_id >> 8) & 0xFF,
		    self.transport_stream_id & 0xFF,
		    (self.original_network_id >> 8) & 0xFF,
		    self.original_network_id & 0xFF,
		    (self.service_id >> 8) & 0xFF,
		    self.service_id & 0xFF,
		    self.linkage_type,
		    private_data_bytes
		    )

class terrestrial_delivery_system_descriptor(Descriptor):

    descriptor_tag = 0x5a

    def bytes(self):
        fmt = "!LBBBL"
        return pack(fmt,
                    self.center_frequency & 0xFFFFFFFF,
		    (self.bandwidth << 5) & 0xE0 | (self.priority << 4) & 0x10 | (self.timeslice_ind << 3) & 0x08 |
		    (self.mpe_fec_ind << 2) & 0x04 | 0x03,
		    (self.constellation << 6) | (self.hierarchy_information << 3)| (self.code_rate_HP_stream),
		    (self.code_rate_LP_stream << 5) | (self.guard_interval << 3) | (self.transmission_mode << 1) | (self.other_frequency_flag),
		    0xffffffff,
                    )
