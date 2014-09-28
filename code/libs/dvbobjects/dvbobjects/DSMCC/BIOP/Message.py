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

import os
import string
from dvbobjects.utils import *
from dvbobjects.MHP.Descriptors import content_type_descriptor

######################################################################
class Message(DVBobject):

    magic = "BIOP"                      # ISO
    biop_version_major = 0x01           # ISO
    biop_version_minor = 0x00           # ISO
    byte_order = 0x00                   # DVB
    message_type = 0x00                 # ISO
    serviceContextList = ""             # MHP

    def pack(self):

        # sanity check
        assert self.byte_order      == 0x00 # DVB
        assert len(self.objectKind) == 0x04, `self.objectKind` # DVB

        MessageSubHeader_FMT = (
            "!"
            "B"
	    "L"                      # objectKey
            "L"
	    "%ds"                      # objectKind
            "H"
	    "%ds"                      # objectInfo
            "B"
	    "%ds"                      # serviceContextList
            ) % (
            len(self.objectKind),
            len(self.objectInfo),
            len(self.serviceContextList),
            )
        
        MessageSubHeader = pack(
            MessageSubHeader_FMT,
	    4,
	    self.objectKey,
            len(self.objectKind),         
	    self.objectKind,
            len(self.objectInfo),         
	    self.objectInfo,
	    len(self.serviceContextList),
	    self.serviceContextList,
            )
        
        messageBody = self.messageBody()

        message_size = (
            len(MessageSubHeader)
            + 4                         # messageBody_length field
            + len(messageBody)
            )
        
        MessageHeader = pack (
            "!"
            "4s"                        # magic
            "B"                         # biop_version_major
            "B"                         # biop_version_minor
            "B"                         # byte_order
            "B"                         # message_type
            "L"                         # message_size
            ,                           # end of FMT ;-)
            self.magic, 
            self.biop_version_major,
            self.biop_version_minor,
            self.byte_order,
            self.message_type,
            message_size,               # computed
            )
        
        FMT = ("!"
               "%ds"                    # MessageHeader
               "%ds"                    # MessageSubHeader
               "L"                      # messageBody_length
               "%ds"                    # messageBody
               ) % (
            len(MessageHeader),
            len(MessageSubHeader),
            len(messageBody)
            )
        
        return pack(
            FMT,                        # see above
            MessageHeader,
            MessageSubHeader,
            len(messageBody),           # messageBody
            messageBody,
            )

######################################################################
class FileMessage(Message):

    objectKind = CDR("fil")             # DVB

    def __init__(self, **kwargs):

        # Initialize SuperClass
        apply(Message.__init__, (self,), kwargs)

        # Initialize standard attributes
        self.set(
            objectInfo = pack("!LL", 0, self.contentSize),
            )

        # Maybe we're playing MHP...
        try:
            content_type = self.content_type
        except AttributeError:
            content_type = None         # i.e: UNKNOWN

            
        # if we know a content_type, add descriptor...
        if content_type:
            ctd = content_type_descriptor(content_type=content_type)
            self.objectInfo = self.objectInfo + ctd.pack()

    def messageBody(self):
        # Retrieve the file message body (i.e. file content)
        content = open(self.PATH).read()
        content_length = len(content)
        assert content_length == self.contentSize, self.PATH

        return pack("!L%ds" % content_length,
                    content_length,
                    content,
                    )

######################################################################
class StreamEventMessage(Message):

    objectKind = CDR("ste")             # DVB    
    objectInfo = ""

    def __init__(self, **kwargs):

        # Initialize SuperClass
        apply(Message.__init__, (self,), kwargs)
	
	info_t = pack("!BLLBBB" , 0, 0, 0, 0, 0, 1)
	# Hard coded DSM::Stream::Info_T.pack() for do it now
	event_names = open(self.PATH + "/.ename").read()
	self.objectInfo =  info_t + event_names

    def messageBody(self):

        # stream event do it now: BIOP.program_use_tap (id undefined) + eventIds
	taps = open(self.PATH + "/.tap").read()
	event_ids = open(self.PATH + "/.eid").read()
	return taps + event_ids


######################################################################
class DirectoryMessage(Message):

    objectKind = CDR("dir")             # DVB
    objectInfo = ""                     # MHP

    def __init__(self, **kwargs):

        # Initialize SuperClass
        apply(Message.__init__, (self,), kwargs)

        self.bindings_count = len(self.bindings)

        assert self.bindings_count < 512 # MHP

    def messageBody(self):
        separator = ""                  # for debugging
        bindings = "%s%s%s" % (
            separator,
            string.join(map(lambda binding: binding.pack(),
                            self.bindings),
                        separator),
            separator)

        FMT = (
            "!"
            "H"                         # bindings_count
            "%ds"                       # bindings
            ) % (len(bindings))

        messageBody = pack(
            FMT,
            len(self.bindings),
            bindings,
            )

        return messageBody

######################################################################
class ServiceGatewayMessage(DirectoryMessage):

    objectKind = CDR("srg")             # DVB

######################################################################
class ServiceGatewayInfo(DVBobject):

    userInfo = ""                       # MHP

    def pack(self):
        FMT = ("!"
               "%ds"                    # IOP::IOR
               "B"                      # downloadTaps_count
               "B"                      # serviceContextList_count
               "H%ds"                   # userInfo
               ) % (
            len(self.srg_ior),
            len(self.userInfo),
            )
        
        return pack(FMT,
                    self.srg_ior,       # IOP::IOR
                    0,                  # downloadTaps_count
                    0,                  # serviceContextList_count
                    len(self.userInfo),
                    self.userInfo,
                    )
