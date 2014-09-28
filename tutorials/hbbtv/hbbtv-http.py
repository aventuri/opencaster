#! /usr/bin/env python

#
# Copyright  2010, mediatvcom (http://www.mediatvcom.com/), Claude Vanderm. Based on Lorenzo Pallara scripts (l.pallara@avalpa.com) 
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
from dvbobjects.HBBTV.Descriptors import *

#
# Shared values
#

demo_transport_stream_id = 1 # demo value, an official value should be demanded to dvb org
demo_original_network_id = 1 # demo value, an official value should be demanded to dvb org
demo_service_id = 1 # demo value
pmt1_pid= 1031
ait1_pid = 2001
ste1_pid = 2002


# parameters reported into the AIT to signalize a broadband application.
appli_name = "my hbbtv application" #application name
appli_root = "http://my_application_root_path/myHbbTV-app/" #URL base of transport_protocol_descriptor
appli_path = "index.html"  #initial_path_bytes of simple application descriptor. So the application path will be "http://my_application_root_path/myHbbTV-app/index.html"
organisationId_1 = 10  # this is a demo value, dvb.org should assign an unique value
applicationId_1 = 1001 # this is a demo value. This number corresponds to a trusted application. 
organisationId_2 = 11
applicationId_2 = 1002

# below, the video and audio PIDs reported into the PMT
video1_pid = 2064
audio1_pid = 2068
#
# Network Information Table
# this is a basic NIT with the minimum desciptors, OpenCaster has a big library ready to use
#

nit = network_information_section(
	network_id = 1,
        network_descriptor_loop = [
	    network_descriptor(network_name = "OpenHbb",), 
            ],
	transport_stream_loop = [
	    transport_stream_loop_item(
		transport_stream_id = demo_transport_stream_id,
		original_network_id = demo_original_network_id,
		transport_descriptor_loop = [
		    service_list_descriptor(
			dvb_service_descriptor_loop = [
			    service_descriptor_loop_item(
				service_ID = demo_service_id, 
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
	transport_stream_id = demo_transport_stream_id,
        program_loop = [
    	    program_loop_item(
	        program_number = demo_service_id,
    		PID = pmt1_pid,
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
# Program Map Table (ISO/IEC 13818-1 2.4.4.8)
# this is PMT for HbbTV interactive applications
#	
  
pmt = program_map_section(
	program_number = demo_service_id,
	PCR_PID = video1_pid, # usualy the same than the video
	program_info_descriptor_loop = [],
	stream_loop = [
		stream_loop_item(
			stream_type = 2, # mpeg2 video stream type
			elementary_PID = video1_pid,
			element_info_descriptor_loop = []
		),
		stream_loop_item(
			stream_type = 3, # mpeg2 audio stream type
			elementary_PID = audio1_pid,
			element_info_descriptor_loop = []
		),
		stream_loop_item(
			stream_type = 5, # AIT stream type
			elementary_PID = ait1_pid,
			element_info_descriptor_loop = [ 
			    application_signalling_descriptor(
				application_type = 0x0010, # HbbTV service
				AIT_version = 1,  # current ait version
			    ),
			]	
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
	transport_stream_id = demo_transport_stream_id,
	original_network_id = demo_original_network_id,
        service_loop = [
			service_loop_item(
				service_ID = demo_service_id,
				EIT_schedule_flag = 0, # 0 no current even information is broadcasted, 1 broadcasted
				EIT_present_following_flag = 0, # 0 no next event information is broadcasted, 1 is broadcasted
				running_status = 4, # 4 service is running, 1 not running, 2 starts in a few seconds, 3 pausing
				free_CA_mode = 0, # 0 means service is not scrambled, 1 means at least a stream is scrambled
				service_descriptor_loop = [
					service_descriptor(
						service_type = 1, # digital television service
						service_provider_name = "mediatvcom",
						service_name = "openHbb",
					),    
				],
			),	
        ],
        version_number = 1, # you need to change the table number every time you edit, so the decoder will compare its version with the new one and update the table
        section_number = 0,
        last_section_number = 0,
        )
#
# Application Informaton Table (ETSI TS 101 812 10.4.6)
#
#

ait = application_information_section(
        application_type = 0x0010,
        common_descriptor_loop = [
		external_application_authorisation_descriptor(
			application_identifiers = [[organisationId_1,applicationId_1] , [organisationId_2,applicationId_2]],
			application_priority =     [  			5			          ,		 		1			      ]
			# This descriptor informs that 2 applications are available on the program by specifying the applications identifiers (couple of organization_Id and application_Id parameters) and their related priorities (5 for the first and 1 for the second).
			# Actualy our service contains only one application so this descriptor is not relevent and is just here to show you how to use this descriptor.
			# This descriptor is not mandatory and you could remove it (i.e. common_descriptor_loop = []).
			) 
		],
        application_loop = [
		application_loop_item(
			organisation_id = organisationId_1,  # this is a demo value, dvb.org should assign an unique value
			application_id = applicationId_1, 
			
			application_control_code = 1, 
						# 2 is PRESENT, the decoder will add this application to the user choice of application
						# 1 is AUTOSTART, the application will start immedtiatly to load and to execute
						# 7 is DISABLED, The application shall not be started and attempts to start it shall fail.
						# 4 is KILL, it will stop execute the application
			application_descriptors_loop = [
				transport_protocol_descriptor(
					protocol_id = 0x0003, # HTTP transport protocol
					URL_base = appli_root,
					URL_extensions = [],
					transport_protocol_label = 3, # HTTP transport protocol
				),  
				application_descriptor(
					application_profile = 0x0000,
						#0x0000 basic profile
						#0x0001 download feature
						#0x0002 PVR feature
						#0x0004 RTSP feature
					version_major = 1, # corresponding to version 1.1.1
					version_minor = 1,
					version_micro = 1,
					service_bound_flag = 1, # 1 means the application is expected to die on service change, 0 will wait after the service change to receive all the AITs and check if the same app is signalled or not
					visibility = 3, # 3 the applications is visible to the user, 1 the application is visible only to other applications
					application_priority = 1, # 1 is lowset, it is used when more than 1 applications is executing
					transport_protocol_labels = [3], # If more than one protocol is signalled then each protocol is an alternative delivery mechanism. The ordering indicates 
													 # the broadcaster's view of which transport connection will provide the best user experience (first is best)
				),
				application_name_descriptor(
					application_name = appli_name,
					 ISO_639_language_code = "FRA"
				),
				simple_application_location_descriptor(initial_path_bytes = appli_path),		
			]
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
os.system('/usr/local/bin/sec2ts 16 < ./nit.sec > ./nit.ts')

out = open("./sdt.sec", "wb")
out.write(sdt.pack())
out.close
out = open("./sdt.sec", "wb") # python   flush bug
out.close
os.system('/usr/local/bin/sec2ts 17 < ./sdt.sec > ./sdt.ts')

out = open("./pat.sec", "wb")
out.write(pat.pack())
out.close
out = open("./pat.sec", "wb") # python   flush bug
out.close
os.system('/usr/local/bin/sec2ts 0 < ./pat.sec > ./pat.ts')

out = open("./pmt.sec", "wb")
out.write(pmt.pack())
out.close
out = open("./pmt.sec", "wb") # python   flush bug
out.close
os.system('/usr/local/bin/sec2ts ' + str(pmt1_pid) + ' < ./pmt.sec > ./pmt.ts')

out = open("./ait.sec", "wb")
out.write(ait.pack())
out.close
out = open("./ait.sec", "wb") # python   flush bug
out.close
os.system('/usr/local/bin/sec2ts ' + str(ait1_pid) + ' < ./ait.sec > ./ait.ts')

os.system('rm *.sec') # deleting of the section files.
