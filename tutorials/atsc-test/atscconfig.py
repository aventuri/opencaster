#! /usr/bin/env python

#
# Copyright (C) 2008  Lorenzo Pallara, l.pallara@avalpa.com
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

import os

from dvbobjects.PSI.PAT import *
from dvbobjects.PSI.PMT import *
from dvbobjects.PSI.MGT import *
from dvbobjects.PSI.TVCT import *
from dvbobjects.DVB.Descriptors import *
from dvbobjects.MPEG.Descriptors import *

#
# Shared values
#

avalpa_transport_stream_id = 1 # demo value, an official value should be demanded to dvb org
avalpa_original_transport_stream_id = 1 # demo value, an official value should be demanded to dvb org
avalpa1_service_id = 1 # demo value
avalpa1_pmt_pid = 1031


#
# Terrestrial Virtual Channel Table
#
tvct = terrestrial_virtual_channel_section(
	transport_stream_id = avalpa_transport_stream_id,
	ATSC_protocol_version = 0,
	channels_loop = [
		channel_loop_item(
			short_name = "\x00\x41\x00\x76\x00\x61\x00\x6c\x00\x70\x00\x61\x00\x31", #Avalpa1 as utf-16 unicode
			major_channel_number = 5,
			minor_channel_number = 1,
			modulation_mode = 0x04, #8 VSB
			carrier_frequency = 0, # deprecated
			channel_TSID = avalpa_transport_stream_id,
			program_number = avalpa1_service_id,
			ETM_location = 0, # no ETM
			access_controlled = 0, # no access control
			hidden = 0,
			hide_guide = 0,
			service_type = 0x2, # television
			source_id = 0x1, # region?
			descriptors_loop = [],
		)
	],
	additional_descriptors_loop = [],
	version_number = 1,
	section_number = 0,
	last_section_number = 0,
	)


#
# Master Guide Table
#

mgt = master_guide_section(
	ATSC_protocol_version = 0,
	tables_loop = [
		table_loop_item(
			table_type = 0x0, # terrestrial vct
			table_type_pid = 0x1FFB,
			table_type_version_number = tvct.version_number,
			number_bytes = len(tvct.pack()),
			descriptors_loop = [],
		),
	],
	descriptors_loop = [],
	version_number = 1,
	section_number = 0,
	last_section_number = 0,
	)



#
# Program Association Table (ISO/IEC 13818-1 2.4.4.3)
#

pat = program_association_section(
	transport_stream_id = avalpa_transport_stream_id,
        program_loop = [
    	    program_loop_item(
	        program_number = avalpa1_service_id,
    		PID = avalpa1_pmt_pid,
    	    ),  
        ],
        version_number = 1, # you need to change the table number every time you edit, so the decoder will compare its version with the new one and update the table
        section_number = 0,
        last_section_number = 0,
        )


#
# Program Map Table (ISO/IEC 13818-1 2.4.4.8)
# this is a basic PMT the the minimum desciptors, OpenCaster has a big library ready to use
#	

pmt = program_map_section(
	program_number = avalpa1_service_id,
	PCR_PID = 2064,
	program_info_descriptor_loop = [],
	stream_loop = [
		stream_loop_item(
			stream_type = 2, # mpeg2 video stream type
			elementary_PID = 2064,
			element_info_descriptor_loop = []
		),
		stream_loop_item(
			stream_type = 129, # atsc ac3
			elementary_PID = 2068,
			element_info_descriptor_loop = [
				registration_descriptor(
					format_identifier="AC-3",
				),
			]
		),
	],
        version_number = 1, # you need to change the table number every time you edit, so the decoder will compare its version with the new one and update the table
        section_number = 0,
        last_section_number = 0,
        )    

#
# PSI marshalling and encapsulation
#

out = open("./pat.sec", "wb")
out.write(pat.pack())
out.close
out = open("./pat.sec", "wb") # python   flush bug
out.close
os.system('/usr/local/bin/sec2ts 0 < ./pat.sec > ./firstpat.ts')

out = open("./pmt.sec", "wb")
out.write(pmt.pack())
out.close
out = open("./pmt.sec", "wb") # python   flush bug
out.close
os.system('/usr/local/bin/sec2ts ' + str(avalpa1_pmt_pid) + ' < ./pmt.sec > ./firstpmt.ts')

out = open("./mgt.sec", "wb")
out.write(mgt.pack())
out.close
out = open("./mgt.sec", "wb") # python   flush bug
out.close
os.system('/usr/local/bin/sec2ts 8187 < ./mgt.sec > ./8187.ts')

out = open("./tvct.sec", "wb")
out.write(tvct.pack())
out.close
out = open("./tvct.sec", "wb") # python   flush bug
out.close
os.system('/usr/local/bin/sec2ts 8187 < ./tvct.sec >> ./8187.ts')
