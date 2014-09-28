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
from dvbobjects.PSI.NIT import *
from dvbobjects.PSI.SDT import *
from dvbobjects.PSI.TDT import *
from dvbobjects.PSI.EIT import *
from dvbobjects.PSI.PMT import *
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
# Network Information Table
# this is a basic NIT with the minimum desciptors, OpenCaster has a big library ready to use
#


nit = network_information_section(
	network_id = 1,
        network_descriptor_loop = [
	    network_descriptor(network_name = "Avalpa",), 
            ],
	transport_stream_loop = [
	    transport_stream_loop_item(
		transport_stream_id = avalpa_transport_stream_id,
		original_network_id = avalpa_original_transport_stream_id,
		transport_descriptor_loop = [
		    service_list_descriptor(
			dvb_service_descriptor_loop = [
			    service_descriptor_loop_item(
				service_ID = avalpa1_service_id, 
				service_type = 1, # digital tv service type
			    ),
			],
		    ),
		],		
	     ),
	  ],
        version_number = 1, # you need to change the table number every time you edit, so the decoder will compare its version with the new one and update the table
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
    	    program_loop_item(
	        program_number = 0, # special program for the NIT
    		PID = 16,
    	    ), 
        ],
        version_number = 1, # you need to change the table number every time you edit, so the decoder will compare its version with the new one and update the table
        section_number = 0,
        last_section_number = 0,
        )


#
# Service Description Table (ETSI EN 300 468 5.2.3) 
# this is a basic SDT with the minimum desciptors, OpenCaster has a big library ready to use
#

sdt = service_description_section(
	transport_stream_id = avalpa_transport_stream_id,
	original_network_id = avalpa_original_transport_stream_id,
        service_loop = [
	    service_loop_item(
		service_ID = avalpa1_service_id,
		EIT_schedule_flag = 1, # 0 no current even information is broadcasted, 1 broadcasted
		EIT_present_following_flag = 1, # 0 no next event information is broadcasted, 1 is broadcasted
		running_status = 4, # 4 service is running, 1 not running, 2 starts in a few seconds, 3 pausing
		free_CA_mode = 0, # 0 means service is not scrambled, 1 means at least a stream is scrambled
		service_descriptor_loop = [
		    service_descriptor(
			service_type = 1, # digital television service
			service_provider_name = "Avalpa",
			service_name = "Avalpa 1",
		    ),    
		],
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
			stream_type = 4, # mpeg2 audio stream type
			elementary_PID = 2068,
			element_info_descriptor_loop = []
		),
	],
        version_number = 1, # you need to change the table number every time you edit, so the decoder will compare its version with the new one and update the table
        section_number = 0,
        last_section_number = 0,
        )    




#
# Time Description Table (ETSI EN 300 468 5.2.5) 
# it should be replaced at run time with tstdt
#

tdt = time_date_section(
	year = 108, # since 1900
	month = 6,
	day = 16,
	hour = 0x11, # use hex like decimals
	minute = 0x30,
	second = 0x21,
        version_number = 1,
        section_number = 0,
        last_section_number = 0,
        )


#
# Event Information Table (ETSI EN 300 468 5.2.4) 
#

eit = event_information_section(
	table_id = EIT_ACTUAL_TS_PRESENT_FOLLOWING,
	service_id = avalpa1_service_id,
	transport_stream_id = avalpa_transport_stream_id,
	original_network_id = avalpa_original_transport_stream_id,
        event_loop = [
	    event_loop_item(
		event_id = 1,
		start_year = 108, # since 1900
		start_month = 6, 
		start_day = 10, 
		start_hours = 0x00, # use hex like decimals
		start_minutes = 0x00,
		start_seconds = 0x00,
		duration_hours = 0x23,
		duration_minutes = 0x00,
		duration_seconds = 0x00,
		running_status = 4, # 4 service is running, 1 not running, 2 starts in a few seconds, 3 pausing
		free_CA_mode = 0, # 0 means service is not scrambled, 1 means at least a stream is scrambled
		event_descriptor_loop = [
		    short_event_descriptor (
			ISO639_language_code = "ITA", 
			event_name = "epg event name",
			text = "this is an epg event text example",
		    ),
		],
	    ),	    	
            ],
        segment_last_section_number = 1,
        version_number = 1,
        section_number = 0,
        last_section_number = 1, # pay attention here, we have another section after this!
        )


eit_follow = event_information_section(
	table_id = EIT_ACTUAL_TS_PRESENT_FOLLOWING,
	service_id = avalpa1_service_id,
	transport_stream_id = avalpa_transport_stream_id,
	original_network_id = avalpa_original_transport_stream_id,
        event_loop = [
	    event_loop_item(
		event_id = 2, 
		start_year = 108, # since 1900
		start_month = 06, 
		start_day = 10,
		start_hours = 0x23,
		start_minutes = 0x30,
		start_seconds = 0x00, 
		duration_hours = 0x12, 
		duration_minutes = 0x00,
		duration_seconds = 0x00, 
		running_status = 4, # 4 service is running, 1 not running, 2 starts in a few seconds, 3 pausing
		free_CA_mode = 0, # 0 means service is not scrambled, 1 means at least a stream is scrambled
		event_descriptor_loop = [
		    short_event_descriptor (
			ISO639_language_code = "ITA", 
			event_name = "epg event name 2",
			text = "this is the following text example", 
		    )    
		],
	    ),
            ],
        segment_last_section_number = 1,
        version_number = 1, 
        section_number = 1, # this is the second section
        last_section_number = 1, 
        )

#
# PSI marshalling and encapsulation
#

out = open("./nit.sec", "wb")
out.write(nit.pack())
out.close
out = open("./nit.sec", "wb") # python  flush bug
out.close
os.system('/usr/local/bin/sec2ts 16 < ./nit.sec > ./firstnit.ts')

out = open("./pat.sec", "wb")
out.write(pat.pack())
out.close
out = open("./pat.sec", "wb") # python   flush bug
out.close
os.system('/usr/local/bin/sec2ts 0 < ./pat.sec > ./firstpat.ts')

out = open("./sdt.sec", "wb")
out.write(sdt.pack())
out.close
out = open("./sdt.sec", "wb") # python   flush bug
out.close
os.system('/usr/local/bin/sec2ts 17 < ./sdt.sec > ./firstsdt.ts')

out = open("./eit.sec", "wb")
out.write(eit.pack())
out.close
out = open("./eit.sec", "wb") # python   flush bug
out.close
os.system('/usr/local/bin/sec2ts 18 < ./eit.sec > ./firsteit.ts')

out = open("./eit_follow.sec", "wb")
out.write(eit_follow.pack())
out.close
out = open("./eit_follow.sec", "wb") # python   flush bug
out.close
os.system('/usr/local/bin/sec2ts 18 < ./eit_follow.sec >> ./firsteit.ts')

out = open("./tdt.sec", "wb")
out.write(tdt.pack())
out.close
out = open("./tdt.sec", "wb") # python   flush bug
out.close
os.system('/usr/local/bin/sec2ts 20 < ./tdt.sec > ./firsttdt.ts')

out = open("./pmt.sec", "wb")
out.write(pmt.pack())
out.close
out = open("./pmt.sec", "wb") # python   flush bug
out.close
os.system('/usr/local/bin/sec2ts ' + str(avalpa1_pmt_pid) + ' < ./pmt.sec > ./firstpmt.ts')

