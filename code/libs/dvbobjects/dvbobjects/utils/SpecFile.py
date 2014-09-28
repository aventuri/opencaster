#! /usr/bin/env python

# 
# Copyright © 2000-2001, GMD, Sankt Augustin
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

######################################################################
class SuperGroupSpec(DVBobject):

    def read(self, filename):
        self.PATH = filename
        file = open(filename)

        items = string.split(file.readline())
        self.transactionId = eval(items[0])
	self.version       = eval(items[1])
        fn = items[1]
        if fn == "None":
            self.srg_ior = None
        else:
            self.srg_ior       = open(items[2]).read()

        self.groups = []
        
        while (1):
            line = file.readline()
            if not line: break

            group = GroupSpec()
            #print repr(line)
            group.read(line)
            self.groups.append(group)

    def write(self, outputDir):
        specFile      = "%s/DSI.spec" % outputDir
        dsi = open(specFile, "wb")
        dsi.write("0x%08X 0x%08X %s\n" % (
                  int(self.transactionId),
		  self.version,
                  self.srg_ior,
                  ))
        for group in self.groups:
            group.write(dsi, outputDir)

    def addGroup(self, **kwargs):
        group = apply(GroupSpec, [], kwargs)
        try:
            self.groups.append(group)
        except AttributeError:
            self.groups = [group]

    def addModule(self, **kwargs):
        curr = self.groups[-1]
        apply(GroupSpec.addModule, (curr,), kwargs)

######################################################################
class GroupSpec(DVBobject):

    def read(self, specline):
        items = string.split(specline)

        self.PATH=items[0]
        #print repr(items[0])
        diiSpecFile        = open(items[0])
        self.transactionId = eval(items[1])
	self.version       = eval(items[2])
        self.downloadId    = eval(items[3])
        if len(items) == 6:
            self.assocTag      = eval(items[4])
            self.blockSize     = eval(items[5])
        else:
            self.assocTag      = None
            self.blockSize     = eval(items[6])

        self.modules = []

        while 1:
            line = diiSpecFile.readline()
            if not line:
                break

            mod = ModuleSpec()
            mod.read(line)
            self.modules.append(mod)

    def write(self, specFile, outputDir):
        specFile.write(
            "%s 0x%08X 0x%08X 0x%08X 0x%04X %4d\n" % (
            "%s/DII.spec" % outputDir,
            int(self.transactionId),
	    self.version,
            self.downloadId,
            self.assocTag,
            self.blockSize,
            ))

    def addModule(self, **kwargs):
        mod = apply(ModuleSpec, (), kwargs)
        try:
            self.modules.append(mod)
        except AttributeError:
            self.modules = [mod]

######################################################################
class ModuleSpec(DVBobject):

    def read(self, specline):
            items = string.split(specline)
            #print repr(items[0])
            #print repr(items[1])
            self.INPUT          = items[0]
            self.tableid        = eval(items[1])
            self.moduleId       = eval(items[2])
            self.moduleVersion  = eval(items[3])

######################################################################


