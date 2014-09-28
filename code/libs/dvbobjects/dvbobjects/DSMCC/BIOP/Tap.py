#! /usr/bin/env python

# This file is part of the dvbobjects library.
# 
# Copyright 2000-2001, GMD, Sankt Augustin
# -- German National Research Center for Information Technology 
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

from dvbobjects.utils import *

######################################################################
class _tap(DVBobject):

    BIOP_DELIVERY_PARA_USE   = 0x0016
    BIOP_OBJECT_USE          = 0x0017
    BIOP_ES_USE              = 0x0018
    BIOP_PROGRAM_USE         = 0x0019
    STR_NPT_USE              = 0x000B
    STR_STATUS_AND_EVENT_USE = 0x000C
    STR_EVENT_USE            = 0x000D
    STR_STATUS_USE           = 0x000E

    id  = 0x0000                        # MHP

######################################################################
class object_tap(_tap):

    use = _tap.BIOP_OBJECT_USE

    selector = ""                       # MHP ???

    def pack(self):
        assert self.selector == ""      # MHP ???
        
        FMT = (
            "!"
            "H"                         # id
            "H"                         # use
            "H"                         # assocTag
            "B"                         # selector_length
            )
        
        return pack(
            FMT,
            self.id,
            self.use,
            self.assocTag,
            len(self.selector),
            )

######################################################################
class elementary_stream_tap(_tap):

    use = _tap.BIOP_ES_USE

    selector = ""                       # MHP ???

    def pack(self):
        assert self.selector == ""      # MHP ???
        
        FMT = (
            "!"
            "H"                         # id
            "H"                         # use
            "H"                         # assocTag
            "B"                         # selector_length
            )
        
        return pack(
            FMT,
            self.id,
            self.use,
            self.assocTag,
            len(self.selector),
            )

######################################################################
class program_use_tap(_tap):

    use = _tap.BIOP_PROGRAM_USE

    selector = ""                       # MHP ???

    def pack(self):
        assert self.selector == ""      # MHP ???
        
        FMT = (
            "!"
            "H"                         # id
            "H"                         # use
            "H"                         # assocTag
            "B"                         # selector_length
            )
        
        return pack(
            FMT,
            self.id,
            self.use,
            self.assocTag,
            len(self.selector),
            )
	    
######################################################################
class str_event_use_tap(_tap):

    use = _tap.STR_EVENT_USE

    selector = ""                       # MHP ???

    def pack(self):
        assert self.selector == ""      # MHP ???
        
        FMT = (
            "!"
            "H"                         # id
            "H"                         # use
            "H"                         # assocTag
            "B"                         # selector_length
            )
        
        return pack(
            FMT,
            self.id,
            self.use,
            self.assocTag,
            len(self.selector),
            )

######################################################################
class delivery_para_tap(_tap):

    use = _tap.BIOP_DELIVERY_PARA_USE

    selector_length = 0x0A              # MHP
    selector_type   = 0x0001            # MHP
    
    def pack(self):
        FMT = (
            "!"
            "H"                         # id
            "H"                         # use
            "H"                         # assocTag
            "B"                         # selector_length
            "H"                         # selector_type
            "L"                         # transaction_id
            "L"                         # timeout
            )
        
        return pack(
            FMT,
            self.id,
            self.use,
            self.assocTag,
            self.selector_length,
            self.selector_type,
            int(self.transactionId),
	    int(self.timeout),
            )
