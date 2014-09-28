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

import os
import string

from dvbobjects.utils import *
from dvbobjects.DSMCC.Message import *
import dvbobjects.DSMCC.BIOP

from Loops import *

######################################################################
class SuperGroup(DownloadServerInitiate):
    
    def __init__(self, PATH, transactionId, version):
	self.groups =  []
	self.PATH = PATH
	self.transactionId = transactionId
	self.version_number = version
            
    def addGroup(self, group):
        self.groups.append(group)
    
    def generate(self, output_dir, SRG_IOR = None):
        for group in self.groups:
            group.generate(output_dir)

        if SRG_IOR <> None:
            self.privateData = dvbobjects.DSMCC.BIOP.ServiceGatewayInfo(
                srg_ior = SRG_IOR,
                )
        else:
            self.privateData = GroupInfoIndication(self)

        sec_file = open("%s/DSI.sec" % output_dir, "wb")
        sec_file.write(self.pack())
        sec_file.close()

######################################################################
class Group(DownloadInfoIndication):
    
    def __init__(self, PATH, transactionId, version, downloadId, blockSize):
	self.modules = []
	self.descriptors = []
	self.PATH = PATH
	self.downloadId = downloadId
	self.transactionId = transactionId
	self.version_number = version
	self.blockSize = blockSize

    def addModule(self, mod):
        self.modules.append(mod)

    def getGroupInfo(self):
        return GroupInfo(
            groupId   = self.transactionId,
            groupSize = self.groupSize,
            groupCompatibility = self.compatibilityDescriptor,
            descriptors = self.descriptors,
            )

    def generate(self, output_dir):
        self.groupSize = 0
        for module in self.modules:
            module.downloadId = self.downloadId
            module.generate(
                output_dir,
                self.transactionId,
                self.blockSize,
                )
            self.groupSize = self.groupSize + module.moduleSize

        self.__mii = mii = ModuleInfoIndication(self)
        self.moduleInfoIndication = mii
 
        path = os.path.basename(self.PATH)
        stem = os.path.splitext(path)[0]
        sec_file = open("%s/%s.sec" % (output_dir, stem), "wb")
        sec_file.write(self.pack())
        sec_file.close()

######################################################################
class Module(DVBobject):

    assocTag = None                     # i.e. NO Object Carousel
    descriptors = []

    def generate(self, output_dir, transactionId, blockSize):
        lastBlockNumber = -1
        self.moduleSize = os.stat(self.INPUT)[6]
        last_section_number = ((self.moduleSize - 1) / blockSize) % 256
        tmp = ((((self.moduleSize - 1) / blockSize) / 256) * 256) - 1

        input = open(self.INPUT, "rb")
        while 1:
            data = input.read(blockSize)
            if not data:
                break
            lastBlockNumber = lastBlockNumber + 1
            if tmp >= lastBlockNumber:
                block = DownloadDataBlock(
                    moduleId = self.moduleId,
                    moduleVersion = self.moduleVersion,
                    downloadId = self.downloadId,
                    data_block = data,
                    blockNumber = lastBlockNumber,
                    section_number = lastBlockNumber % 256,
                    last_section_number = 0xFF,
                )
            else:
                block = DownloadDataBlock(
                    moduleId = self.moduleId,
                    moduleVersion = self.moduleVersion,
                    downloadId = self.downloadId,
                    data_block = data,
                    blockNumber = lastBlockNumber,
                    section_number = lastBlockNumber % 256,
                    last_section_number = last_section_number,
                ) 

            ofn = self.__outputFilename(
                self.INPUT,
                output_dir,
                block.blockNumber)
            open(ofn, "wb").write(block.pack())

    def __outputFilename(self, input_path, output_dir, blockNumber):
        input_basename = os.path.basename(input_path)
        input_stem = os.path.splitext(input_basename)[0]
        output_basename = "%s_%06d.%s" % (
            input_stem,
            blockNumber,
            "sec",
            )
        output_path = os.path.join(
            output_dir, output_basename)
        return output_path

    def getModuleInfo(self):
        if self.assocTag <> None:
            moduleInfo = dvbobjects.DSMCC.BIOP.ModuleInfo(
                assocTag = self.assocTag,
		userInfo = self.descriptors,
                )
        else:
            moduleInfo = ""

        modInfo = ModuleInfo(
            moduleId = self.moduleId,
            moduleSize = self.moduleSize,
            moduleVersion = self.moduleVersion,
            descriptors   = self.descriptors,
            moduleInfo = moduleInfo,
            )

        return modInfo
