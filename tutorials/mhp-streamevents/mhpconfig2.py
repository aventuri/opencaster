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
from dvbobjects.MHP.AIT import *
from dvbobjects.MHP.Descriptors import *
from dvbobjects.DSMCC.STE import *


#
# Shared values
#

avalpa_transport_stream_id = 1 # demo value, an official value should be demanded to dvb org
avalpa_original_transport_stream_id = 1 # demo value, an official value should be demanded to dvb org
avalpa1_service_id = 1 # demo value
avalpa1_pmt_pid = 1031
ait1_pid = 2001
ste1_pid = 2002
dsmccB_association_tag = 0xB
dsmccB_carousel_id = 1
dsmccC_association_tag = 0xC
dsmccC_carousel_id = 2

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
# this is PMT with DSMCC and AIT desciptors for MHP interactive applications
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
			stream_type = 3, # mpeg2 audio stream type
			elementary_PID = 2068,
			element_info_descriptor_loop = []
		),
		stream_loop_item(
			stream_type = 5, # AIT stream type
			elementary_PID = ait1_pid,
			element_info_descriptor_loop = [ 
			    application_signalling_descriptor(
				application_type = 1, # 1 DVB-J application, 2 DVB-HTML
				AIT_version = 1,  # current ait version
			    ),
			]	
		),
		stream_loop_item(
			stream_type = 12, # Stream Event stream type
			elementary_PID = ste1_pid,
			element_info_descriptor_loop = [ 
				stream_identifier_descriptor(
					component_tag = 0xD,
				),
			]	
		),		
		stream_loop_item(
			stream_type = 11, # DSMCC stream type
			elementary_PID = 2003,
			element_info_descriptor_loop = [
				# a number of descriptors follow specifying the DSMCC properites
				association_tag_descriptor(
					association_tag = dsmccB_association_tag,  # this association tag identifys the carousel, it is used also while generating the DSMCC with oc-update.sh and referenced by the AIT
					use = 0,  # some default values follow, don't change then, different values are not supported
					selector_lenght = 0, # ...
					transaction_id = 0x80000000, # ...
					timeout = 0xFFFFFFFF, # ...
					private_data = "",
				),
				stream_identifier_descriptor(
					component_tag = dsmccB_association_tag, # it is the same as the assocation tag, some decoders will look for the component tag, others for the association tag, the same value should be used
				),
				carousel_identifier_descriptor(
					carousel_ID = dsmccB_carousel_id, # carousel id number, it's a different number from association/component tag, but it has a similiar purpouse: identifying the carousel
					format_ID = 0, # no enhanced boot supported
					private_data = "",
				),
				data_broadcast_id_descriptor(
					data_broadcast_ID = 240, # 240 is the code specifying this is an MHP Object Carousel
					ID_selector_bytes = "",
				),
			]
		),
		stream_loop_item(
			stream_type = 11, # DSMCC stream type
			elementary_PID = 2004,
			element_info_descriptor_loop = [
				# a number of descriptors follow specifying the DSMCC properites
				association_tag_descriptor(
					association_tag = dsmccC_association_tag,  # this association tag identifys the carousel, it is used also while generating the DSMCC with oc-update.sh and referenced by the AIT
					use = 0,  # some default values follow, don't change then, different values are not supported
					selector_lenght = 0, # ...
					transaction_id = 0x80000000, # ...
					timeout = 0xFFFFFFFF, # ...
					private_data = "",
				),
				stream_identifier_descriptor(
					component_tag = dsmccC_association_tag, # it is the same as the assocation tag, some decoders will look for the component tag, others for the association tag, the same value should be used
				),
				carousel_identifier_descriptor(
					carousel_ID = dsmccC_carousel_id, # carousel id number, it's a different number from association/component tag, but it has a similiar purpouse: identifying the carousel
					format_ID = 0, # no enhanced boot supported
					private_data = "",
				),
				data_broadcast_id_descriptor(
					data_broadcast_ID = 240, # 240 is the code specifying this is an MHP Object Carousel
					ID_selector_bytes = "",
				),
			]
		)
			
	],
        version_number = 1, # you need to change the table number every time you edit, so the decoder will compare its version with the new one and update the table
        section_number = 0,
        last_section_number = 0,
        )    


#
# Application Informaton Table (ETSI TS 101 812 10.4.6)
#

ait = application_information_section(
        application_type = DVB_J_application_type,
        common_descriptor_loop = [],
        application_loop = [
		application_loop_item(
		organisation_id = 10,  # this is a demo value, dvb.org should assign an unique value
	        application_id = 1001, # your application id, should be unique for every organisation id present in the same program
	        application_control_code = 2, # 2 is PRESENT, the decoder will add this application to the user choice of application
					# 1 is AUTOSTART, the application will start immedtiatly to load and to execute
					# 3 is DESTROY, it will signal to the application to stop executing
					# 4 is KILL, it will stop execute the application
		# Some required application descriptor follows application id
	        application_descriptors_loop = [
			transport_protocol_descriptor(
			        protocol_id = MHP_OC_protocol_id, # the application is broadcasted on a MHP DSMCC
			        transport_protocol_label = 1, # carousel id
			        remote_connection = 0,
			        component_tag = 0xB, # carousel common tag and association tag
			),
			application_descriptor(
			        application_profile = 0x0001, # Profile and MHP version of the application MHP 1.0.2
			        version_major = 1,
			        version_minor = 0,
			        version_micro = 2,
			        service_bound_flag = 1, # 1 means the application is expected to die on service change, 0 will wait after the service change to receive all the AITs and check if the same app is signalled or not
			        visibility = 3, # 3 the applications is visible to the user, 1 the application is visible only to other applications
			        application_priority = 1, # 1 is lowset, it is used when more than 1 applications is executing
			        transport_protocol_labels = [1], # carousel Id
			),
			application_name_descriptor(application_name = "Text input example"),
			dvb_j_application_descriptor(parameters = ["file:///sample8"]), # parameter passed to the xlet
			dvb_j_application_location_descriptor(
				base_directory = "/", # base directory, if set to "/hello" the xlet will act as "/hello" is its root directory
				class_path_extension = "", # an additiona classpath inside the carousel can be specified
				initial_class = "tv.cineca.apps.yambo.Wizard",  # the starting class implementing Xlet interface
			),
        	]
        	),
		application_loop_item(
		organisation_id = 10,  # this is a demo value, dvb.org should assign an unique value 
	        application_id = 1002, # your application id, should be unique for every organisation id present in the same program
	        application_control_code = 2, # 2 is PRESENT, the decoder will add this application to the user choice of application
					# 1 is AUTOSTART, the application will start immedtiatly to load and to execute
					# 3 is DESTROY, it will signal to the application to stop executing
					# 4 is KILL, it will stop execute the application
		# Some required application descriptor follows application id
	        application_descriptors_loop = [
			transport_protocol_descriptor(
			        protocol_id = MHP_OC_protocol_id, # the application is broadcasted on a MHP DSMCC
			        transport_protocol_label = 2, # carousel id
			        remote_connection = 0,
			        component_tag = 0xC, # carousel common tag and association tag
			),
			application_descriptor(
			        application_profile = 0x0001, # Profile and MHP version of the application MHP 1.0.2
			        version_major = 1,
			        version_minor = 0,
			        version_micro = 2,
			        service_bound_flag = 1, # 1 means the application is expected to die on service change, 0 will wait after the service change to receive all the AITs and check if the same app is signalled or not
			        visibility = 3, # 3 the applications is visible to the user, 1 the application is visible only to other applications
			        application_priority = 1, # 1 is lowset, it is used when more than 1 applications is executing
			        transport_protocol_labels = [2], # carousel Id
			),
			application_name_descriptor(application_name = "Test Stream Event"),
			dvb_j_application_descriptor(parameters = []), # no parameters passed
			dvb_j_application_location_descriptor(
				base_directory = "/", # base directory, if set to "/hello" the xlet will act as "/hello" is its root directory
				class_path_extension = "", # an additiona classpath inside the carousel can be specified
				initial_class = "Testste",  # the starting class implementing Xlet interface
			),
        	]
        	),

   	],
        version_number = 1,
        section_number = 0,
        last_section_number = 0,
	)

#
# Stream Event 
#

ste = stream_event_section(
        event_id = 1,
        stream_event_descriptor_loop = [
            stream_event_do_it_now_descriptor(
	        event_id = 1, 
		private_data = "event 1 private data",
	    ),
	],
        version_number = 1,
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

out = open("./ait.sec", "wb")
out.write(ait.pack())
out.close
out = open("./ait.sec", "wb") # python   flush bug
out.close
os.system('/usr/local/bin/sec2ts ' + str(ait1_pid) + ' < ./ait.sec > ./firstait.ts')

out = open("./ste.sec", "wb")
out.write(ste.pack())
out.close
out = open("./ste.sec", "wb") # python  flush bug
out.close
os.system('/usr/local/bin/sec2ts ' + str(ste1_pid) + ' < ./ste.sec > ./firstste.ts')
