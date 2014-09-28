#! /usr/bin/env python

# This file is part of the dvbobjects library.
# 
# Copyright © 2005-2013 Lorenzo Pallara l.pallara@avalpa.com
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

from math import floor

def MJD_convert(year, month, day):

	if (month == 1) or (month == 2):
	    l = 1
	else:
	    l = 0
	return 14956 + day + (floor((year - l) * 365.25)) + (floor((month + 1 + l * 12) * 30.6001))
