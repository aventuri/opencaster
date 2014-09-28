#! /usr/bin/env python

#
# Copyright (C) 2006  Lorenzo Pallara, lpallara@cineca.it
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
import string

from dvbobjects.DSMCC.BIOP.Tap import *
from dvbobjects.DSMCC.STEO import *
from dvbobjects.utils import *

tap = str_event_use_tap()
tap.set(
        id = 0,
	assocTag = 0xD, # association tag defined into PMT for the Stream Event
	)

taps = Taps (
        taps_count = 1,
	tap_loop = [ tap,],
    )

event_count = 3 # number of events

event_names = Event_names (
        eventnames_count = event_count,
        event_name_loop = [ "event 1", "event 2", "event 3"],  # name of the events
    )

event_ids = Event_ids (
        eventids_count = event_count,
        event_id_loop = [ 1, 2, 3,], # id of the events
    )

out = open(".tap", "wb")
out.write(taps.pack())
out.close

out = open(".eid", "wb")
out.write(event_ids.pack())
out.close

out = open(".ename", "wb")
out.write(event_names.pack())
out.close

