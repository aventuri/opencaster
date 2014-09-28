#! /usr/bin/env python

# This file is part of the dvbobjects library.
# 
# Copyright  2000-2001, GMD, Sankt Augustin
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

import string
from dvbobjects.utils import *
from dvbobjects.MPEG.Descriptor import Descriptor


######################################################################
class GroupInfoIndication(DVBobject):

    privateData = ""

    def __init__(self, superGroup):

        self.numberOfGroups = len(superGroup.groups)

        self.groupInfos = []
        for group in superGroup.groups:
            self.groupInfos.append(group.getGroupInfo())

    def pack(self):
        infos = string.join(map(lambda i: i.pack(),
                                self.groupInfos),
                            "")
        FMT = ("!"
               "H"                      # NumberOfGroups
               "%ds"                    # group infos
               "H%ds"                   # privateData
               ) % (len(infos),
                    len(self.privateData))

        return pack(FMT,
                    len(self.groupInfos),
                    infos,
                    len(self.privateData),
                    self.privateData,
                    )
    
######################################################################
class GroupInfo(DVBobject):

    def pack(self):
        self.groupInfo = string.join(map(lambda d: d.pack(),
                                         self.descriptors),
                                     "")

        FMT = ("!"
               "L"                      # groupId
               "L"                      # groupSize
               "H%ds"                   # groupCompatibility
               "H%ds"                   # groupInfo
               ) % (
            len(self.groupCompatibility),
            len(self.groupInfo),
            )

        return pack(FMT,
                    self.groupId,
                    self.groupSize,
                    len(self.groupCompatibility),
                    self.groupCompatibility,
                    len(self.groupInfo),
                    self.groupInfo,
                    )

######################################################################
class ModuleInfoIndication(DVBobject):

    def __init__(self, group):

        self.numberOfModules = len(group.modules)

        self.moduleInfos = []
        for module in group.modules:
            self.moduleInfos.append(module.getModuleInfo())

    def pack(self):
        infos = string.join(map(lambda i: i.pack(),
                                self.moduleInfos),
                            "")
        FMT = ("!"
               "H"                      # NumberOfModules
               "%ds"                    # module infos
               ) % len(infos)

        return pack(FMT,
                    len(self.moduleInfos),
                    infos,
                    )
    
######################################################################
class ModuleInfo(DVBobject):

    def pack(self):
        if self.moduleInfo:
            moduleInfoBytes = self.moduleInfo.pack()
	elif self.moduleInfo == "" and len(self.descriptors) == 0:
            moduleInfoBytes = ""	    
        else:
            moduleInfoBytes = string.join(
                map(lambda d: d.pack(),
                    self.descriptors),
                "")

        FMT = ("!"
               "H"                      # moduleId
               "L"                      # moduleSize
               "B"                      # moduleVersion
               "B%ds"                   # moduleInfo
               ) % (
            len(moduleInfoBytes),
            )

        return pack(FMT,
                    self.moduleId,
                    self.moduleSize,
                    self.moduleVersion,
                    len(moduleInfoBytes),
                    moduleInfoBytes,
                    )

###################################################################
class name_descriptor(Descriptor):

    descriptor_tag = 0x02

    def bytes(self):
        self.name_length = len(self.name)
        fmt ="!%ds" % self.name_length
        return pack(fmt,
                    self.name,
                    )

###################################################################
class compressed_descriptor(Descriptor):

    descriptor_tag = 0x09

    def bytes(self):

        compression_method = 0x08

	sizeFile = "%s.size" % self.name
	items = string.split(open(sizeFile).readline())
	original_size = eval(items[0])
	# print("module size %d" % original_size)
        fmt ="!BL" 
        return pack(fmt,
	            compression_method,
                    original_size,
                    )
###################################################################
class crc32_descriptor(Descriptor):
    descriptor_tag = 0x05

    def bytes(self):
        fmt = "!L"
        return pack(fmt, self.calc_crc32)
###################################################################
class ssu_module_type_descriptor(Descriptor):
    descriptor_tag = 0x0A

    def bytes(self):
        fmt = "!B"
        return pack(fmt, self.module_type)

