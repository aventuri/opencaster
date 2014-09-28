#! /usr/bin/env python
#
# Copyright (C) 2009  Lorenzo Pallara, l.pallara@avalpa.com
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
from dvbobjects.PSI.UNT import *
from dvbobjects.PSI.TDT import *
from dvbobjects.DVB.Descriptors import *
from dvbobjects.MPEG.Descriptors import *
from dvbobjects.DVB.DataCarousel import *

#
# Shared values
#

avalpa_transport_stream_id = 1 # demo value, an official value should be demanded to dvb org
avalpa_original_transport_stream_id = 1 # demo value, an official value should be demanded to dvb org
avalpa1_service_id = 1 # demo value
avalpa1_pmt_pid = 1031
avalpa1_unt_pid = 1032
avalpa1_dsmcc_pid = 2003
dsmccB_association_tag = 0xB # association aka component tag for the dsmcc carousel
anyOUI = 0x00015A # default demo value for any OUI

comp_desc1 = compatibility_descriptor(
	compatibility_descriptor_loop = [
		compatibility_descriptor_loop_item (
			descriptor_type = 0x01, # hardware descriptor
			specifier_type = 0x01, # IEEE OUI
			specifier_data = anyOUI,
			model = 3,
			version = 2,
			compatibility_descriptor_subloop = [],
		),
		compatibility_descriptor_loop_item (
			descriptor_type = 0x02, # software descriptor
			specifier_type = 0x01, # IEEE OUI
			specifier_data = anyOUI,
			model = 4,
			version = 3,
			compatibility_descriptor_subloop = [],
		),
	]
)

#
# Network Information Table
#

nit = network_information_section(
	network_id = 1,
        network_descriptor_loop = [
	    network_descriptor(network_name = "Avalpa",), 
	    linkage_descriptor(
			transport_stream_id = avalpa_transport_stream_id,
			original_network_id = avalpa_original_transport_stream_id,
			service_id = 1,
			linkage_type = 0x09,
			OUI_loop = [
				OUI_data (
					OUI = anyOUI,
					selector_bytes = "",
				)
			],
			private_data_bytes = "",
			),
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
				service_type = 0x0C, # data broadcast service
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
			service_type = 0x0C, # data broadcast service
			service_provider_name = "Avalpa",
			service_name = "Avalpa data",
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
#

pmt = program_map_section(
	program_number = avalpa1_service_id,
	PCR_PID = 8191,
	program_info_descriptor_loop = [],
	stream_loop = [
		stream_loop_item(
			stream_type = 11, # data stream type
			elementary_PID = avalpa1_unt_pid,
			element_info_descriptor_loop = [
				data_broadcast_id_descriptor(
					data_broadcast_ID = 0x000A, # DVB-SSU
					OUI_info_loop = [
						OUI_info_loop_item (
							OUI = anyOUI,
							update_type = 0x02, # with broadcasted UNT, 0x01 without UNT: it requires a stream_identifier_descriptor
							update_versioning_flag = 0, # no version change
							update_version = 1, # increment this at update change
							selector_bytes = "",
						),
					],
					private_data_bytes = "",
				),
			]
		),
		stream_loop_item(
			stream_type = 11, # data stream type
			elementary_PID = avalpa1_dsmcc_pid,
			element_info_descriptor_loop = [
				stream_identifier_descriptor(
					component_tag = dsmccB_association_tag,
				),
			]
		)	
	],
        version_number = 1, # you need to change the table number every time you edit, so the decoder will compare its version with the new one and update the table
        section_number = 0,
        last_section_number = 0,
        )    

#
# Update Notification Table
# according to TS 102 006 v 1.3.1
#

# dovrei usare almeno un location descriptor, o non ha senso e magari un scheduling descriptor

unt = update_notification_section(
	action_type = 0x01, # system software upgrade
	OUI = anyOUI,
	processing_order = 0x00, # first action
	common_descriptor_loop = [],
	compatibility_descriptor_loop = [
		unt_compatibility_descriptor_loop_item (
			compatibility_descriptor = comp_desc1.pack(),
			target_descriptor_loop = [],
			operational_descriptor_loop = [
				scheduling_descriptor(
					year_of_start_date_time = 109, 
					month_of_start_date_time = 9,
					day_of_start_date_time = 27,
					hour_of_start_date_time = 12,
					minute_of_start_date_time = 0,
					second_of_start_date_time = 0,
					year_of_end_date_time = 109, 
					month_of_end_date_time = 8,
					day_of_end_date_time = 27,
					hour_of_end_date_time = 12,
					minute_of_end_date_time = 0,
					second_of_end_date_time = 0,
					final_availability = 1,
					Periodicity_flag = 1,
					period_unit = 2, # hour
					duration_unit = 1, # minute
					estimated_cycle_time_unit = 2, # hour
					Period = 1,
					Duration = 30,
					estimated_cycle_time = 0,
					private_data_bytes = "",
				),
				ssu_location_descriptor(
					data_broadcast_id = 0x000A, # standard DVB-SSU two layer DSMCC
					association_tag = dsmccB_association_tag,
					private_data_bytes = "",
				),
			],
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
# DSMCC description
#

g1 = Group(
    PATH="DII-1.sec",
    transactionId = 0x80000002,
    downloadId    = 0x00000001,
    blockSize     = 4066,
    version	  = 1,
    )
g1.set(
    compatibilityDescriptor = comp_desc1.pack(),
    modules = [
         Module(
    	    INPUT="ocdir1/RootRelook2100",
    	    moduleId = 0x0001,
    	    moduleVersion = 0x00,
    	    descriptors = [name_descriptor(name="a bin")],
	),
         Module(
    	    INPUT="ocdir1/RootRelook2101",
    	    moduleId = 0x0002,
    	    moduleVersion = 0x00,
    	    descriptors = [name_descriptor(name="a bin")],
	),
         Module(
    	    INPUT="ocdir1/RootRelook2102",
    	    moduleId = 0x0003,
    	    moduleVersion = 0x00,
    	    descriptors = [name_descriptor(name="a bin")],
	),
         Module(
    	    INPUT="ocdir1/RootRelook2103",
    	    moduleId = 0x0004,
    	    moduleVersion = 0x00,
    	    descriptors = [name_descriptor(name="a bin")],
	),
         Module(
    	    INPUT="ocdir1/RootRelook2104",
    	    moduleId = 0x0005,
    	    moduleVersion = 0x00,
    	    descriptors = [name_descriptor(name="a bin")],
	),
         Module(
    	    INPUT="ocdir1/RootRelook2105",
    	    moduleId = 0x0006,
    	    moduleVersion = 0x00,
    	    descriptors = [name_descriptor(name="a bin")],
	),
         Module(
    	    INPUT="ocdir1/RootRelook2106",
    	    moduleId = 0x0007,
    	    moduleVersion = 0x00,
    	    descriptors = [name_descriptor(name="a bin")],
	),
         Module(
    	    INPUT="ocdir1/RootRelook2107",
    	    moduleId = 0x0008,
    	    moduleVersion = 0x00,
    	    descriptors = [name_descriptor(name="a bin")],
	),
         Module(
    	    INPUT="ocdir1/RootRelook2108",
    	    moduleId = 0x0009,
    	    moduleVersion = 0x00,
    	    descriptors = [name_descriptor(name="a bin")],
	),
         Module(
    	    INPUT="ocdir1/RootRelook2109",
    	    moduleId = 0x000A,
    	    moduleVersion = 0x00,
    	    descriptors = [name_descriptor(name="a bin")],
	),
    ],
    )

g2 = Group(
    PATH="DII-2.sec",
    transactionId = 0x80000004,
    downloadId    = 0x00000002,
    blockSize     = 4066,
    version	  = 1,
    )
g2.set(
	compatibilityDescriptor = "hello", 
	modules = [
         Module(
    	    INPUT="big-file.raw",
    	    moduleId = 0x0010,
    	    moduleVersion = 0x00,
    	    descriptors = [name_descriptor(name="another bin")],
    	    ),
    ],
    )

dsi = SuperGroup(
    PATH = "/",
    transactionId = 0x80000000,
    version	  = 1,
    )
dsi.set(
    compatibilityDescriptor = "\000\000", # as specified in etsi 102 006
)
dsi.addGroup(g1)
dsi.addGroup(g2)


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

out = open("./unt.sec", "wb")
out.write(unt.pack())
out.close
out = open("./unt.sec", "wb") # python   flush bug
out.close
os.system('/usr/local/bin/sec2ts ' + str(avalpa1_unt_pid) + ' < ./unt.sec > ./firstunt.ts')

out = open("./tdt.sec", "wb")
out.write(tdt.pack())
out.close
out = open("./tdt.sec", "wb") # python   flush bug
out.close
os.system('/usr/local/bin/sec2ts 20 < ./tdt.sec > ./firstdt.ts')

# 
# DSMCC streams generation
#

# for an object carousel the call would be: dsi.generate("objectcarousel", srg_ior_file_content)
dsi.generate("datacarousel")
