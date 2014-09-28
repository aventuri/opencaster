#! /usr/bin/env python

#
# Copyright (C) 2011  Lorenzo Pallara, l.pallara@avalpa.com
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
import time

 
from dvbobjects.DVB.Descriptors import *
from dvbobjects.MPEG.Descriptors import *
from dvbobjects.PSI.TDT import *
from dvbobjects.PSI.TOT import *

current_time = time.gmtime()

tdt = time_date_section(
        year = current_time[0]-1900,
        month = current_time[1],
        day = current_time[2],
        hour = ((current_time[3] / 10) * 16) + (current_time[3] % 10), # it requires decimal time in hex... like 0x22 for 10 p.m.
        minute = ((current_time[4] / 10) * 16) + (current_time[4] % 10),
        second = ((current_time[5] / 10) * 16) + (current_time[5] % 10),
)
out = open("./tdt.sec", "wb")
out.write(tdt.pack())
out.close
out = open("./tdt.sec", "wb") # python   flush bug
out.close
os.system('/usr/local/bin/sec2ts 20 < ./tdt.sec > ./tdt.ts')

current_local_time = time.localtime()

from dateutil.rrule import *
from dateutil.parser import *
from datetime import *

# italian rules
current_offset_polarity = 0x0 # positive
if current_local_time.tm_isdst == 1:
	start = 'DTSTART:%(year)04d%(month)02d%(day)02dT%(hour)02d%(minute)02d%(second)02d\n' % { "year": current_time[0], "month": current_time[1], "day": current_time[1], "hour" : 3, "minute": 0, "second": 0}
	start += 'RRULE:FREQ=YEARLY;BYDAY=-1SU;BYMONTH=10;COUNT=1' # last sunday of october
	current_offset = 0x02
	new_offset = 0x01
else :
	start = 'DTSTART:%(year)04d%(month)02d%(day)02dT%(hour)02d%(minute)02d%(second)02d\n' % { "year": current_time[0], "month": current_time[1], "day": current_time[1], "hour" : 2, "minute": 0, "second": 0}
	start += 'RRULE:FREQ=YEARLY;BYDAY=-1SU;BYMONTH=3;COUNT=1' # last sunday of march
	current_offset = 0x01
	new_offset = 0x02

change_time = list(rrulestr(start))[0]
#
tot = time_offset_section(
        year = current_time[0]-1900,
        month = current_time[1],
        day = current_time[2],
        hour = ((current_time[3] / 10) * 16) + (current_time[3] % 10), # it requires decimal time in hex... like 0x22 for 10 p.m.
        minute = ((current_time[4] / 10) * 16) + (current_time[4] % 10),
        second = ((current_time[5] / 10) * 16) + (current_time[5] % 10),
        descriptor_loop = [
    		local_time_offset_descriptor (
    			local_time_offset_loop = [
    				local_time_offset_loop_item (
    					ISO_639_language_code = "ITA",
    					country_region_id = 0, # italy has only 1 region
    					local_time_offset_polarity = current_offset_polarity, 
    					local_time_offset_hour = current_offset,
    					local_time_offset_minute = 0x00,
    					year_of_change = change_time.year-1900,
    					month_of_change = change_time.month,
    					day_of_change = change_time.day,
    					hour_of_change = ((change_time.hour / 10) * 16) + (change_time.hour % 10),
    					minute_of_change = ((change_time.minute / 10) * 16) + (change_time.minute % 10),
    					second_of_change = ((change_time.second / 10) * 16) + (change_time.second % 10),
    					next_time_offset_hour = new_offset,
    					next_time_offset_minute = 0x00,
    				),
    			],
    		),
        ],
)

out = open("./tot.sec", "wb")
out.write(tot.pack())
out.close
out = open("./tot.sec", "wb") # python   flush bug
out.close
os.system('/usr/local/bin/sec2ts 20 < ./tot.sec >> ./tdt.ts')
