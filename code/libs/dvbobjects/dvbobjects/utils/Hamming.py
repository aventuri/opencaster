#! /usr/bin/env python

# This file is part of the dvbobjects library.
#
# Copyright (C) 2009-2013 Lorenzo Pallara l.pallara@avalpa.com
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

from numpy import *

ham84 = array([0x15, 0x02, 0x49, 0x5E, 0x64, 0x73, 0x38, 0x2F,
              0xD0, 0xC7, 0x8C, 0x9B, 0xA1, 0xB6, 0xFD, 0xEA])


def hamming84(nibble):

    nibble = nibble & 0xF
    return ham84[nibble]
