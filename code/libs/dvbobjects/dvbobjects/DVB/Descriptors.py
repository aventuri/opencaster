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
from dvbobjects.utils.MJD import *
from dvbobjects.MPEG.Descriptor import Descriptor

######################################################################                                                                                                                 
class short_event_descriptor(Descriptor):                                                                                                                                              

    descriptor_tag = 0x4D
    
    def bytes(self):
	assert len(self.ISO639_language_code) == 3
	fmt = "!%dsB%dsB%ds" % (
	    len(self.ISO639_language_code),
	    len(self.event_name),
	    len(self.text),
	)
	return pack(fmt,
	    self.ISO639_language_code,
	    len(self.event_name),
	    self.event_name,
	    len(self.text),
	    self.text,
	)

######################################################################
class content_user_loop_item(DVBobject):

    def pack(self):
	fmt = "!BB"
	return pack(fmt,
	    ((self.content_nibble_level_1 & 0xF) << 4)|
	    ((self.content_nibble_level_2 & 0xF)),
	    ((self.user_nibble_level_1 & 0xF) << 4)|
	    ((self.user_nibble_level_2 & 0xF))
	)


class content_descriptor(Descriptor):

    descriptor_tag = 0x54

    def bytes(self):
	data_bytes = string.join(
		map(lambda x: x.pack(),
		self.content_user_loop),
		"")

	fmt = "!%ds" % len(data_bytes)
	return pack(fmt,
	    data_bytes,
	)

######################################################################
class parental_rating_descriptor(Descriptor):                                                                                                                                              

    descriptor_tag = 0x55
    
    def bytes(self):
	assert len(self.country_code) == 3
	fmt = "!%dsB" % (
	    len(self.country_code),
	)
	return pack(fmt,
	    self.country_code,
	    self.rating,
	)

######################################################################
class dts_registration_descriptor(Descriptor):

    descriptor_tag = 0x05

    def bytes(self):
	fmt = "!L" 
	return pack(fmt,
	    self.format_identifier,
		)
#DTS format_identifier is 0x44545331 ("DTS1") for frame size 512;
#DTS format_identifier is 0x44545332 ("DTS2") for frame size 1 024;
#DTS format_identifier is 0x44545333 ("DTS3") for frame size 2 048.

######################################################################
class teletext_descriptor(Descriptor):

    descriptor_tag = 0x56

    def bytes(self):
	data_bytes = string.join(
		    map(lambda x: x.pack(),
		    self.teletext_descriptor_loop),
		    "")
		    
	fmt = "!%ds" % len(data_bytes)
	return pack(fmt,
		data_bytes,
		)

class teletext_descriptor_loop_item(DVBobject):

    def pack(self):
	assert len(self.ISO639_language_code) == 3
	fmt = "!%dsBB" % len(self.ISO639_language_code)
	return pack(fmt,
	    self.ISO639_language_code,
	    (self.type << 3) | 
	    (self.magazine_number & 0x07),
	    self.page_number,
		)

######################################################################
class vbi_data_descriptor_loop_item(DVBobject):

    def pack(self):
            fmt = "!B"
	    return pack(fmt,
		0x3F & 
		((self.field_parity & 0x01) >> 5) | 
		(self.line_offset & 0x1F),    
		)

class vbi_data_descriptor(Descriptor):

    descriptor_tag = 0x45

    def bytes(self):
	data_bytes = string.join(
		    map(lambda x: x.pack(),
		    self.vbi_data_descriptor_loop),
		    "")
		    
	fmt = "!BB%ds" % len(data_bytes)
	return pack(fmt,
	    self.data_service_id,
	    len(data_bytes),
	    data_bytes,
		)

######################################################################
class stream_identifier_descriptor(Descriptor):

    descriptor_tag = 0x52

    def bytes(self):
	fmt = "!B"
	return pack(fmt,
		    self.component_tag,
		)
		
######################################################################
class bouquet_name_descriptor(Descriptor):

    descriptor_tag = 0x71

    def bytes(self):
        fmt = "!%ds" % len(self.bouquet_name)
        return pack(fmt,
    		self.bouquet_name)


######################################################################
class data_broadcast_id_descriptor(Descriptor):

    descriptor_tag = 0x66

    def bytes(self):
	if (self.data_broadcast_ID == 0x000A): # DVB-SSU
	
	    oui_data_bytes = string.join(
		map(lambda x: x.pack(),
		self.OUI_info_loop),
		"")
	
	    oui_data_length = len(oui_data_bytes);

    	    FMT = "!HB%ds%ds" % (oui_data_length,len(self.private_data_bytes))
    	    return pack(FMT,         
                    self.data_broadcast_ID,
		    oui_data_length,
		    oui_data_bytes,
		    self.private_data_bytes,
                    )            

	else:
    	    FMT = "!H%ds" % len(self.ID_selector_bytes)
    	    return pack(FMT,         
                    self.data_broadcast_ID,
                    self.ID_selector_bytes,
                    )            
                                 
######################################################################

class local_time_offset_loop_item(DVBobject):

	def pack(self):
	
		FMT = "!%dsBBBHBBBBB" % len(self.ISO_639_language_code)
		return pack(FMT,
			self.ISO_639_language_code,
			((self.country_region_id & 0x3F) << 2) | 0x2 | (self.local_time_offset_polarity & 0x1),
			self.local_time_offset_hour,
			self.local_time_offset_minute,
			MJD_convert(self.year_of_change, self.month_of_change, self.day_of_change),
			self.hour_of_change,
			self.minute_of_change,
			self.second_of_change,
			self.next_time_offset_hour,
			self.next_time_offset_minute,
			)


######################################################################

class local_time_offset_descriptor(Descriptor):

	descriptor_tag = 0x58

	def bytes(self):
		lt_bytes = string.join(
	    map(lambda x: x.pack(),	
                self.local_time_offset_loop),
            "")								    
		FMT = "!%ds" % len(lt_bytes)
		return pack(FMT,
			lt_bytes
			)


######################################################################

class _broadcast_id_descriptor(Descriptor):

    descriptor_tag = 0x66

    def bytes(self):
        FMT = "!H%ds" % len(self.ID_selector_bytes)
        return pack(FMT,         
                    self.data_broadcast_ID,
                    self.ID_selector_bytes,
                    )            
                                 
######################################################################
class application_signalling_descriptor(Descriptor):

    descriptor_tag = 0x6F
    application_type = -1

    def bytes(self):
	if (self.application_type == -1):
	    fmt = "!"
    	    return pack(fmt)
	else:
    	    FMT = "!HB"
    	    return pack(FMT, 
		self.application_type,
		0xE0 | (self.AIT_version & 0x1F),
                )
		    
######################################################################
class network_descriptor(Descriptor):

    descriptor_tag = 0x40

    def bytes(self):
        fmt = "!%ds" % len(self.network_name)
        return pack(fmt,
                    self.network_name,
                    )
		    
######################################################################
class service_descriptor(Descriptor):

    descriptor_tag = 0x48

    def bytes(self):
        fmt = "!BB%dsB%ds" % (len(self.service_provider_name), len(self.service_name))
        return pack(fmt,
		    self.service_type,
		    len(self.service_provider_name),
		    self.service_provider_name,
		    len(self.service_name),
		    self.service_name,
                    )


######################################################################
class transport_stream_terrestrial_descriptor(Descriptor):

   descriptor_tag = 0x5a

   def bytes(self):
       fmt = "!LBBBL"
       return pack(fmt,
           self.center_frequency,
           (self.bandwidth << 5) | 0x1f,
           (self.constellation << 6) | (self.hierarchy_information << 3)| (self.code_rate_HP_stream),
           (self.code_rate_LP_stream << 5) | (self.guard_interval << 3) | (self.transmission_mode << 1) | self.other_frequency_flag,
           0xffffffff,
           )

######################################################################
class transport_stream_sat_descriptor(Descriptor):

    descriptor_tag = 0x43
 
    def bytes(self):
    	fmt = "!LHBL"
	return pack(fmt,
		self.frequency,
		self.orbital_position,
		(self.west_east_flag << 7) | (self.polarization << 5) | self.modulation,
		(self.symbol_rate << 4)| self.FEC_inner,
	)

#######################################################################
class transport_stream_cable_descriptor(Descriptor):

    descriptor_tag = 0x44
 
    def bytes(self):
	fmt = "!LHBL"
	return pack(fmt,
	    self.frequency,
	    (self.FEC_outer) | 0xFFF0,
	    (self.modulation),
	    (self.symbol_rate << 4)| (self.FEC_inner),
	)
 		    
######################################################################
class service_descriptor_loop_item(DVBobject):

    def pack(self):
	fmt = "!HB"
	return pack(fmt,
	    self.service_ID,
	    self.service_type,
	)

class service_list_descriptor(Descriptor):

    descriptor_tag = 0x41

    def bytes(self):
        dvb_service_bytes = string.join(
	    map(lambda x: x.pack(),	
                self.dvb_service_descriptor_loop),
            "")								    

        FMT = "!%ds" % len(dvb_service_bytes)
        return pack(FMT,
                    dvb_service_bytes,
                    )
                    
######################################################################
class registration_descriptor(Descriptor):

    descriptor_tag = 0x05

    def bytes(self):
        fmt = "!%ds" % len(self.format_identifier)
        return pack(fmt,
			self.format_identifier
        )

######################################################################
class lcn_service_descriptor_loop_item(DVBobject):

    def pack(self):
	fmt = "!HH"
	return pack(fmt,
	    self.service_ID,
	    ((self.visible_service_flag << 15) | 0x7C00 | self.logical_channel_number),
	)

class logical_channel_descriptor(Descriptor):

    descriptor_tag = 0x83

    def bytes(self):
        lcn_service_bytes = string.join(
	    map(lambda x: x.pack(),	
                self.lcn_service_descriptor_loop),
            "")								    

        FMT = "!%ds" % len(lcn_service_bytes)
        return pack(FMT,
                    lcn_service_bytes,
                    )

####################################################################
class logical_channel_descriptor_v2(Descriptor):

   descriptor_tag = 0x87

   def bytes(self):
       lcn_service_bytes = string.join(
       map(lambda x: x.pack(),                  self.lcn_service_descriptor_loop),
           "")                                  
       FMT = "!BB%ds%dsB%ds" % (len(self.channel_list_name), len(self.country_code), len(lcn_service_bytes))
       return pack(FMT,
               self.channel_list_id,
        	len(self.channel_list_name),
               self.channel_list_name,
               self.country_code,
               len(lcn_service_bytes),
                   lcn_service_bytes,
        )

######################################################################
class component_descriptor(Descriptor):

    descriptor_tag = 0x50

    def bytes(self):
        fmt = "!BBB%ds%ds" % (
			len(self.ISO_639_language_code),
			len(self.text_char),
			)
        return pack(fmt,
                    0xF0 | (self.stream_content),
                    self.component_type,
		    self.component_tag,
		    self.ISO_639_language_code,
		    self.text_char,
        )

######################################################################
class PDC_descriptor(Descriptor):

    descriptor_tag = 0x69

    def bytes(self):
        fmt = "!BH"
        return pack(fmt,
                    (0xF0) | (self.day >> 1),
		    (self.day << 15) | (self.month << 11) | (self.hour << 6) | (self.minute),
        )
					

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
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


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

class subtitling_data_descriptor_loop_item(DVBobject):

    def pack(self):
    
        fmt = "!%dsBHH" % len(self.ISO_639_language_code)
        return pack(fmt,
                    self.ISO_639_language_code,
                    self.subtitling_type,
                    self.composition_page_id,
                    self.ancillary_page_id,
                    )


class subtitling_descriptor(Descriptor):
    
    descriptor_tag = 0x59

    def bytes(self):
	data_bytes = string.join(
		    map(lambda x: x.pack(),
		    self.subtitling_data_descriptor_loop),
		    "")
		    
	fmt = "!B%ds" % len(data_bytes)
	return pack(fmt,
	    len(data_bytes),
	    data_bytes,
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
    
        fmt = "!I%dI" % len(self.IPv4_addr_bytes)
    
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
                    (self.time_slicing << 7) & 0x80 | (self.mpe_fec << 5) & 0x60 | (0x03 << 3) | self.frame_size & 0x07,
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

# FIXME: move these classes to another file, they are no exactly descriptors
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

class OUI_data(DVBobject):

  def pack(self):
  
	fmt = "!HBB%ds" % len(self.selector_bytes)

	return pack(fmt,
		    self.OUI >> 8,
		    self.OUI & 0xFF,
		    len(self.selector_bytes),
		    self.selector_bytes,
		    )

class OUI_info_loop_item(DVBobject):

  def pack(self):
  
	fmt = "!HBBBB%ds" % len(self.selector_bytes)

	return pack(fmt,
		    self.OUI >> 8,
		    self.OUI & 0xFF,
		    0xF0 | self.update_type & 0xF,
		    0xC0 | (self.update_versioning_flag & 0x1) << 5 | self.update_version & 0x1F,
		    len(self.selector_bytes),
		    self.selector_bytes,
		    )

class compatibility_descriptor_loop_item(DVBobject):

  def pack(self):
	
	number_descriptors = len(self.compatibility_descriptor_subloop)
	
	compatibility_descriptor_subloop_bytes = string.join(
	  map(lambda x: x.pack(),
	    self.compatibility_descriptor_subloop),
          "")

	fmt = "!BBBBBBHHB%ds" % len(compatibility_descriptor_subloop_bytes)

	return pack(fmt,
		    self.descriptor_type,
		    9 + len(compatibility_descriptor_subloop_bytes),
		    self.specifier_type,
		    (self.specifier_data >> 16) & 0xFF,
		    (self.specifier_data >> 8) & 0xFF,
		    self.specifier_data & 0xFF,
		    self.model,
		    self.version,
		    number_descriptors,
		    compatibility_descriptor_subloop_bytes,
		    )


class compatibility_descriptor(DVBobject):

  def pack(self):
  
 	number_descriptors = len(self.compatibility_descriptor_loop)
 	
	compatibility_descriptor_loop_bytes = string.join(
	  map(lambda x: x.pack(),
	    self.compatibility_descriptor_loop),
          "")
          
	fmt = "!H%ds" % len(compatibility_descriptor_loop_bytes)
	
	return pack(fmt,
		    number_descriptors,
		    compatibility_descriptor_loop_bytes,
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
	
	elif (self.linkage_type == 0x09):

	  # pack oui data loop
	  oui_data_bytes = string.join(
          map(lambda x: x.pack(),
	    self.OUI_loop),
          "")

	  oui_data_length = len(oui_data_bytes);

	  fmt = "!BBBBBBBB%ds%ds" % (oui_data_length, len(self.private_data_bytes))

	  return pack(fmt,
		    (self.transport_stream_id >> 8) & 0xFF,
		    self.transport_stream_id & 0xFF,
		    (self.original_network_id >> 8) & 0xFF,
		    self.original_network_id & 0xFF,
		    (self.service_id >> 8) & 0xFF,
		    self.service_id & 0xFF,
		    self.linkage_type,
		    oui_data_length,
		    oui_data_bytes,
		    self.private_data_bytes
		    )

	elif (self.linkage_type == 0x0A):

	  fmt = "!BBBBBBBB%ds" % len(self.private_data_bytes)

	  return pack(fmt,
		    (self.transport_stream_id >> 8) & 0xFF,
		    self.transport_stream_id & 0xFF,
		    (self.original_network_id >> 8) & 0xFF,
		    self.original_network_id & 0xFF,
		    (self.service_id >> 8) & 0xFF,
		    self.service_id & 0xFF,
		    self.linkage_type,
		    self.table_type,
		    self.private_data_bytes
		    )

	else:
	  fmt = "!BBBBBBB%ds" % len(self.private_data_bytes)

	  # we care only for some linkage_type, other linkage descriptors
	  # have to be implemented according to ETSI EN 300 468 standard

	  return pack(fmt,
		    (self.transport_stream_id >> 8) & 0xFF,
		    self.transport_stream_id & 0xFF,
		    (self.original_network_id >> 8) & 0xFF,
		    self.original_network_id & 0xFF,
		    (self.service_id >> 8) & 0xFF,
		    self.service_id & 0xFF,
		    self.linkage_type,
		    self.private_data_bytes
		    )

class terrestrial_delivery_system_descriptor(Descriptor):

    descriptor_tag = 0x5a

    def bytes(self):
        fmt = "!LBBBL"
	priority = 1
	timeslice_ind = 1
	mpe_fec_ind = 1
        return pack(fmt,
                    self.center_frequency & 0xFFFFFFFF,
		    (self.bandwidth << 5) & 0xE0 | (self.priority << 4) & 0x10 | (self.timeslice_ind << 3) & 0x08 |
		    (self.mpe_fec_ind << 2) & 0x04 | 0x03,
		    (self.constellation << 6) | (self.hierarchy_information << 3)| (self.code_rate_HP_stream),
		    (self.code_rate_LP_stream << 5) | (self.guard_interval << 3) | (self.transmission_mode << 1) | (self.other_frequency_flag),
		    0xffffffff,
                    )



######################################################################                                                                                                                 
class crid_descriptor_loop_item(DVBobject):

    def pack(self):

	if (self.location == 0x0):
    
    	    fmt = "!BB%ds" % len(self.crid)
	    return pack(fmt,
		(self.type & 0x3F) << 2 |
		(self.location),
	        len(self.crid),
	        self.crid,
	    )
	
	else:
	    fmt = "!BH"
	    return pack(fmt,
		(self.type & 0x3F) << 2 |
		(self.location & 0x3),
	        self.crid_ref,
	    )

class content_identifier_descriptor(Descriptor):

    descriptor_tag = 0x76

    def bytes(self):
        crid_bytes = string.join(
	    map(lambda x: x.pack(),	
                self.crid_descriptor_loop),
            "")								    

        FMT = "!%ds" % len(crid_bytes)
        return pack(FMT,
                    crid_bytes,
                    )


class ssu_location_descriptor(Descriptor):

    descriptor_tag = 0x03

    def bytes(self):
    
	if (self.data_broadcast_id == 0x000A):
	
    	    FMT = "!HH%ds" % len(self.private_data_bytes)
    	    return pack(FMT,
    		    self.data_broadcast_id,
    		    self.association_tag,
                    self.private_data_bytes,
                    )

        else:
    	    FMT = "!H%ds" % len(self.private_data_bytes)
    	    return pack(FMT,
    		    self.data_broadcast_id,
                    self.private_data_bytes,
                    )

class scheduling_descriptor(Descriptor):

    descriptor_tag = 0x01

    def bytes(self):
    
	FMT = "!HBBBHBBBBBBB%ds" % len(self.private_data_bytes)
    	return pack(FMT,
    		MJD_convert(self.year_of_start_date_time, self.month_of_start_date_time, self.day_of_start_date_time),
		self.hour_of_start_date_time,
		self.minute_of_start_date_time,
		self.second_of_start_date_time,
    		MJD_convert(self.year_of_end_date_time, self.month_of_end_date_time, self.day_of_end_date_time),
		self.hour_of_end_date_time,
		self.minute_of_end_date_time,
		self.second_of_end_date_time,
    		self.final_availability << 7 | self.Periodicity_flag << 6 | self.period_unit << 4 | self.duration_unit << 2 | self.estimated_cycle_time_unit,
    		self.Period,
    		self.Duration,
    		self.estimated_cycle_time,
                self.private_data_bytes,
                )

######################################################################
class default_authority_descriptor(Descriptor):

    descriptor_tag = 0x73
    
    def bytes(self):
        fmt = "!%ds" % len(self.authority)
        return pack(fmt,
                    self.authority,
                    )

######################################################################                                                                                                                 
class extended_event_loop_item(DVBobject):

    def pack(self):
    
	fmt = "!B%dsB%ds" % (len(self.item_description), len(self.item))
	return pack(fmt,
	    len(self.item_description),
	    self.item_description,
	    len(self.item),
	    self.item,
	)

class extended_event_descriptor(Descriptor):                                                                                                                                              

    descriptor_tag = 0x4E
    
    def bytes(self):
	item_bytes = string.join(
	    map(lambda x: x.pack(),	
                self.extended_event_loop),
            "")	        
	assert len(self.ISO639_language_code) == 3
        FMT = "!B%dsB%dsB%ds" % (len(self.ISO639_language_code), len(item_bytes), len(self.text))
        return pack(FMT,
    		    (self.descriptor_number & 0xF) << 4 |
    		    (self.last_descriptor_number & 0xF),
    		    self.ISO639_language_code,
		    len(item_bytes),
    		    item_bytes,
    		    len(self.text),
    		    self.text,
    		    )
     
######################################################################
class ac3_descriptor(Descriptor):

    descriptor_tag = 0x6A
    
    
    def bytes(self):
    	fmt = "!B"
    	flags_flag = 0
    	flags = ""
	if self.component_type_flag == 1:
	    flags = pack("!B", self.component_type)
	    flags_flag = 1
	if self.bsid_flag == 1:
	    flags = flags + pack("!B", self.bsid)
	    flags_flag = 1
	if self.mainid_flag == 1:
	    flags = flags + pack("!B", self.mainid)
	    flags_flag = 1
	if self.asvc_flag == 1:
	    flags = flags + pack("!B", self.asvc)
	    flags_flag = 1
	if flags_flag == 0 :
	    fmt = (fmt + "%ds") % len(self.additional_info)
	    return pack(fmt,
		(self.component_type_flag & 0x1) << 7 |
		(self.bsid_flag & 0x1) << 6 |
		(self.mainid_flag & 0x1) << 5 |
		(self.asvc_flag & 0x1) << 4,
		self.additional_info
		)
	else :
	    fmt = (fmt + "%ds%ds") % (len(flags), len(self.additional_info))
	    return pack(fmt,
		(self.component_type_flag & 0x1) << 7 |
		(self.bsid_flag & 0x1) << 6 |
		(self.mainid_flag & 0x1) << 5 |
		(self.asvc_flag & 0x1) << 4,
		flags,
		self.additional_info
		)
	
######################################################################
class dts_audio_stream_descriptor(Descriptor):

	descriptor_tag = 0x7B
	
	def bytes(self):
		fmt = "!BBBBB%ds" % len(self.additional_info)
		return pack(fmt,
		((self.sample_rate_code & 0xF) << 4) | ((self.bit_rate_code >> 2) & 0xF),
		((self.bit_rate_code & 0x3) << 6) | ((self.nblks >> 1) & 0x3F),
		((self.nblks & 0x1) << 7) | ((self.fsize >> 7) & 0x7F),
		((self.fsize & 0x7F) << 1) | ((self.surround_mode >> 5) & 0x1),
		((self.surround_mode & 0x1F) << 3) | ((self.lfe_flag & 0x1) << 2) | (self.extendend_surround_flag & 0x3),
		self.additional_info
		)

######################################################################
class ISO_639_language_descriptor(Descriptor):

    descriptor_tag = 0x0A

    def bytes(self):
        fmt = "!%dsB" % (
			len(self.ISO_639_language_code),
			)
        return pack(fmt,
		    self.ISO_639_language_code,
		    self.Audio_type,
        )

######################################################################
class supplementary_audio_descriptor(Descriptor):

    descriptor_tag = 0x7F;
    
    def bytes(self):
	
	descriptor_tag_extension = 0x06;
	
	fmt = "!BB%ds%ds" % (len(self.ISO_639_language_code),len(self.private_data_bytes))
	
	return pack(fmt,
		descriptor_tag_extension,
		(self.mix_type & 0x1) << 7 | (self.editorial_classification & 0x1F) << 2 | 0x3,
		self.ISO_639_language_code,
		self.private_data_bytes,
	)

######################################################################
class private_data_descriptor(Descriptor):

    descriptor_tag = 0x0F

    def bytes(self):
        fmt = "!"
        return pack(fmt)

######################################################################
class bouquet_descriptor(Descriptor):

    descriptor_tag = 0x47

    def bytes(self):
        fmt = "!%ds" % len(self.bouquet_name)
        return pack(
            fmt,
            self.bouquet_name,
        )

