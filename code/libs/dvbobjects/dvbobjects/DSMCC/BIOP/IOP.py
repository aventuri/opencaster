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
from dvbobjects.DSMCC.BIOP import Tap

######################################################################
class IOR(DVBobject):

    def __init__(self, **kwargs):
        apply(DVBobject.__init__, (self,), kwargs)

        # Hard coded; TBD...
        objectLocation = BIOP_ObjectLocation(
            carouselId = self.carouselId,
            moduleId   = self.moduleId,
            objectKey  = self.objectKey,
            )

        tap = Tap.delivery_para_tap(
            assocTag      = self.assocTag,
            transactionId = self.transactionId,
            timeout       = self.timeout,
            )

        connBinder = DSM_ConnBinder(
            tap = tap,
            )

        self.profile = BIOPProfileBody(
            objectLocation = objectLocation,
            connBinder = connBinder,
            )
            
    def pack(self):

        taggedProfiles_count = 1        # MHP / TBD

        assert len(self.type_id) == 4, `self.type_id`

        profile_bytes = self.profile.pack()

        FMT = ("!"
               "L%ds"                   # type_id
               "L"                      # taggedProfiles_count
               "%ds"                    # IOP::taggedProfile()
               ) % (
            len(self.type_id),
            len(profile_bytes),
            )

        ior_bytes = pack(
            FMT,
            len(self.type_id),
            self.type_id,
            taggedProfiles_count,
            profile_bytes,
            )

        return ior_bytes

######################################################################
class BIOPProfileBody(DVBobject):

    profileId_tag = 0x49534F06
    profile_data_byte_order = 0x00

    def __init__(self, **kwargs):

        # Initialize SuperClass
        apply(DVBobject.__init__, (self,), kwargs)


    def pack(self):

        profile_data = (
            self.objectLocation.pack()
            + self.connBinder.pack()
            )
        
        lite_component_count = 2
        profile_data_length = len(profile_data) + 2

        FMT = ("!"
               "L"                      # profileId_tag
               "L"                      # profile_data_length
               "B"                      # profile_data_byte_order
               "B"                      # lite_component_count
               "%ds"                    # profile_data
               ) % (
            len(profile_data),
            )

        return pack(
            FMT,
            self.profileId_tag,
            profile_data_length,
            self.profile_data_byte_order,
            lite_component_count,
            profile_data,
            )


######################################################################
class BIOP_ObjectLocation(DVBobject):

    componentId_tag    = 0x49534F50
    biop_version_major = 0x01
    biop_version_minor = 0x00
    
    def pack(self):

        FMT = (
            "!"
            "L"                         # componentId_tag
            "B"                         # component_data_length
            "L"                         # carouselId
            "H"                         # moduleId
            "B"                         # major version
            "B"                         # minor version
            "B"                       	# objectKey length
	    "L"				# objectKey
            )  

        return pack(
            FMT,
            self.componentId_tag,
            calcsize(FMT) - 5,
            self.carouselId,
            self.moduleId,
            self.biop_version_major,
            self.biop_version_minor,
	    4,
	    self.objectKey,
            )

######################################################################
class DSM_ConnBinder(DVBobject):

    componentId_tag = 0x49534F40
    
    def pack(self):

        taps_count = 1
        taps_bytes = self.tap.pack()

        FMT = (
            "!"
            "L"                         # componentId_tag
            "B"                         # component_data_length
            "B"                         # taps_count
            "%ds"                       # taps_bytes
            ) % len(taps_bytes)

        return pack(
            FMT,
            self.componentId_tag,
            calcsize(FMT) - 5,          # component_data_length
            taps_count,
            taps_bytes,
            )
