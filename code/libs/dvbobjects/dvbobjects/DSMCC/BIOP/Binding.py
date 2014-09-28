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

BINDING_TYPE_NOBJECT  = 0x01
BINDING_TYPE_NCONTEXT = 0x02

######################################################################
class Binding(DVBobject):

    nameComponents_count = 1            # DVB

    def __init__(self, **kwargs):

        # Initialize SuperClass
        apply(DVBobject.__init__, (self,), kwargs)

        assert self.bindingType in (    # DVB
            BINDING_TYPE_NOBJECT,
            BINDING_TYPE_NCONTEXT)

    def pack(self):

        ior = self.IOR.pack()

        FMT = (
            "!"
            "B"                         # nameComponents_count
            "B%ds"                      # id
            "B%ds"                      # kind
            "B"                         # bindingType
            "%ds"                       # IOP::IOR()
            "H%ds"                      # objectInfo
            ) % (
            len(self.nameId),
            len(self.nameKind),
            len(ior),
            len(self.objectInfo),
            )

        return pack(
            FMT,
            self.nameComponents_count,
            len(self.nameId), 
	    self.nameId,
            len(self.nameKind), 
	    self.nameKind,
            self.bindingType,
            ior,
            len(self.objectInfo), 
	    self.objectInfo,
            )

    def __repr__(self):
        """Overrides DVBobject.__repr_, which is noooiiisy"""
        loc = self.IOR.profile.objectLocation
        mod = loc.moduleId
        key = loc.objectKey
        return `(self.nameId,
                 (mod, key),
                 )`

######################################################################
class ObjectFileBinding(Binding):

    bindingType = BINDING_TYPE_NOBJECT
    nameKind = CDR("fil")
    
    def __init__(self, **kwargs):

        # Initialize SuperClass
        apply(Binding.__init__, (self,), kwargs)

       	self.objectInfo = pack("!LL", 0, self.contentSize)
        
######################################################################
class ObjectStreamEventBinding(Binding):

    bindingType = BINDING_TYPE_NOBJECT
    nameKind = CDR("ste")
    objectInfo = ""

    def __init__(self, **kwargs):

        # Initialize SuperClass
        apply(Binding.__init__, (self,), kwargs)

######################################################################
class ContextBinding(Binding):

    bindingType = BINDING_TYPE_NCONTEXT
    nameKind    = CDR("dir")            # MHP
    objectInfo  = ""                    # MHP

    def __init__(self, **kwargs):

        # Initialize SuperClass
        apply(Binding.__init__, (self,), kwargs)

