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

from dvbobjects.PSI.SDT import *
from dvbobjects.DVB.Descriptors import *
from dvbobjects.MPEG.Descriptors import *


#
# Service Description Table (ETSI EN 300 468 5.2.3) 
#

sdt = service_description_section(
	transport_stream_id = 1, # demo value, an official value should be demanded to dvb org
	original_network_id = 1, # demo value, an official value should be demanded to dvb org
        service_loop = [
	    service_loop_item(
		service_ID = 1, # demo value
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
# PSI marshalling and encapsulation
#

out = open("./firstsdt.sec", "wb")
out.write(sdt.pack())
out.close
