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

from dvbobjects.utils import *
from dvbobjects.MPEG.Section import Section

######################################################################
class TransactionId(DVBobject):

    originator = 0x02                   # DVB

    def __init__(self, **kwargs):
        # Initialize SuperClass
        apply(DVBobject.__init__, (self,), kwargs)

        # debugging
        self.HEX = "0x%08X" % self.__int__()
        self.originator = self.originator

    def __int__(self):
        return (
            self.originator << 30
            | (self.version << 16)
            | (self.identification << 1)
	    | (self.updateFlag & 0x1)
            )

######################################################################
class DSMCCmessage(Section):

    private_indicator = 0 

    protocolDiscriminator = 0x11        # DSMCC
    dsmccType = 0x03                    # DSMCC U-N Message
    adaptationLength = 0x00             # ???

    compatibilityDescriptor = ""        # DVB
    privateData = None

    def __init__(self, **kwargs):
        # Initialize SuperClass
        apply(Section.__init__, (self,), kwargs)

        # debugging

    def pack_section_body(self):
        messageBody = self.pack_message_body()
        messageLength = len(messageBody) # +- something
        FMT = ("!"                      # Network Byte Order
               "B"                      # protocolDiscriminator
               "B"                      # dsmccType
               "H"                      # messageId
               "L"                      # transactionId
               "B"                      # reserved
               "B"                      # adaptationLength
               "H"                      # messageLength
               "%ds"                    # messageBody
               ) % messageLength

        return pack(FMT,
                    self.protocolDiscriminator,
                    self.dsmccType,
                    self.messageId,
                    self.transactionId,
                    0xFF,               # reserved
                    self.adaptationLength,
                    messageLength,
                    messageBody,
                    )

######################################################################
class DownloadServerInitiate(DSMCCmessage): # DSI

    # MPEG Section attributes
    version_number = 0
    section_number = 0
    last_section_number = 0

    table_id = 0x3B                     # DSMCC
    messageId = 0x1006                  # DSMCC

    serverId = "\xFF" * 20              # DVB / MHP

    def __init__(self, **kwargs):
        # Initialize SuperClass
        apply(DSMCCmessage.__init__, (self,), kwargs)

        # debugging

    def pack_message_body(self):
        self.table_id_extension = (0x0000FFFF & int(self.transactionId))
        privateDataBytes = self.privateData.pack()

        FMT = ("!"                      # Network Byte Order
               "%ds"                    # serverId
               "H%ds"                   # compatibilityDescriptor()
               "H%ds"                   # privateDataBytes
               ) % (
            len(self.serverId),
            len(self.compatibilityDescriptor),
            len(privateDataBytes),
            )


        return pack(
            FMT,
            self.serverId,
            len(self.compatibilityDescriptor),
            self.compatibilityDescriptor,
            len(privateDataBytes),
            privateDataBytes,
            )
               
        
######################################################################
class DownloadInfoIndication(DSMCCmessage): # DII

    # MPEG Section attributes
    version_number = 0
    section_number = 0
    last_section_number = 0

    table_id = 0x3B                     # DSMCC
    messageId = 0x1002                  # DSMCC

    windowSize = 0                      # DVB
    ackPeriod = 0                       # DVB
    tCDownloadWindow = 0                # DVB
    tCDownloadScenario = 0              # ???

    def __init__(self, **kwargs):
        # Initialize SuperClass
        apply(DSMCCmessage.__init__, (self,), kwargs)

        # debugging

    def pack_message_body(self):
        self.table_id_extension = (0x0000FFFF & int(self.transactionId))

        if self.privateData == None:
            privateDataBytes = ""

        miiBytes = self.moduleInfoIndication.pack()

        fmt = ("!"                      # Network Byte Order
               "L"                      # downloadId
               "H"                      # blockSize
               "B"                      # windowSize
               "B"                      # ackPeriod
               "L"                      # tCDownloadWindow
               "L"                      # tCDownloadScenario
               "H%ds"                   # compatibilityDescriptor
               "%ds"                    # miiBytes
               "H%ds"                   # privateDataBytes
               ) % (
            len(self.compatibilityDescriptor),
            len(miiBytes),
            len(privateDataBytes),
            )
        
        return pack(fmt,
                    self.downloadId,
                    self.blockSize,
                    self.windowSize,
                    self.ackPeriod,
                    self.tCDownloadWindow,
                    self.tCDownloadScenario,
                    len(self.compatibilityDescriptor),
                    self.compatibilityDescriptor,
                    miiBytes,
                    len(privateDataBytes),
                    privateDataBytes,
                    )
 
######################################################################
class DownloadDataBlock(DSMCCmessage): # DDB

    table_id = 0x3C                     # DSMCC
    messageId = 0x1003                  # DSMCC

    def pack_message_body(self):
        # section attributes
        self.table_id_extension = self.moduleId
        self.version_number = self.moduleVersion % 32

        # override transactionId with downloadId
        # i.e. dsmccMessageHeader := dsmccDownloadHeader

        self.transactionId = self.downloadId

        fmt ="!HBBH%ds" % len(self.data_block)
        return pack(fmt,
                    self.moduleId,
                    self.moduleVersion,
                    0xFF,
                    self.blockNumber,
                    self.data_block
                    )
 
