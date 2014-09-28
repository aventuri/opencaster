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
		EIT_schedule_flag = 0, # 0 no current even information is broadcasted, 1 broadcasted
		EIT_present_following_flag = 0, # 0 no next event information is broadcasted, 1 is broadcasted
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
			element_info_descriptor_loop = [
				ca_descriptor (
					CA_system_ID = 0x2600,
					CA_PID = 0x1FFF,
				),
			]
		),
		stream_loop_item(
			stream_type = 4, # mpeg2 audio stream type
			elementary_PID = 2068,
			element_info_descriptor_loop = [
				ca_descriptor (
					CA_system_ID = 0x2600,
					CA_PID = 0x1FFF,
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

out = open("./pmt.sec", "wb")
out.write(pmt.pack())
out.close
out = open("./pmt.sec", "wb") # python   flush bug
out.close
os.system('/usr/local/bin/sec2ts ' + str(avalpa1_pmt_pid) + ' < ./pmt.sec > ./firstpmt.ts')

