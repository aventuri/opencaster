#! /usr/bin/env python
 
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
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

import sys
import getopt

from dvbobjects.utils import *
from dvbobjects.utils.SpecFile import *
from dvbobjects.DVB.DataCarousel import *
from dvbobjects.DVB.Loops import *

######################################################################
def BuildCarousel(INPUT_DIR, OUTPUT_DIR):

    spec = SuperGroupSpec()
    spec.read("%s/DSI.spec" % INPUT_DIR)

    # DVB
    assert 0x0000 <= spec.transactionId & 0x0000FFFF <= 0x0001
    
    dsi = SuperGroup(
        PATH=spec.PATH,
        transactionId = spec.transactionId,
	version = spec.version,
        )

    for group in spec.groups:

        dii = Group(
            PATH = group.PATH,
            transactionId = group.transactionId,
            downloadId = group.downloadId,
            blockSize = group.blockSize,
	    version = group.version,
            )
	    
        # DVB
        assert 0x0001 <= group.transactionId & 0x0000FFFF <= 0xFFFF
	
        for mod in group.modules:
            if os.path.exists("%s.size" % mod.INPUT):
	        # print("Compressing %s") % mod.INPUT
                m = Module(INPUT="%s" % mod.INPUT,
                           moduleVersion= mod.moduleVersion,
                           moduleId = mod.moduleId,
                           assocTag = group.assocTag,
			   descriptors = [compressed_descriptor(name = mod.INPUT)],
                )
            else:
                m = Module(INPUT="%s" % mod.INPUT,
                           moduleVersion= mod.moduleVersion,
                           moduleId = mod.moduleId,
                           assocTag = group.assocTag,
                )	    
            dii.addModule(m)

    dsi.addGroup(dii)
	    
    dsi.generate(OUTPUT_DIR, spec.srg_ior)
    if spec.srg_ior == None:
        print dsi

######################################################################
OPTIONS = "h"
LONG_OPTIONS = [
    "help",
    ]

def Usage(return_code = 1):
    print ("Usage: %s"
           " [option...]"
           " <InputModuleDirectory>"
           " <OutputSectionsDirectory>") % (
        sys.argv[0])
    sys.exit(return_code)

def CheckArgs():

    try:
        opts, args = getopt.getopt(
            sys.argv[1:], OPTIONS, LONG_OPTIONS)
    except getopt.error:
        Usage()

    for opt_name, opt_val in opts:
        if opt_name in ['-h', '--help']:
            Usage(0)

    if len(args) <> 2:
        Usage()

    INPUT_DIR, OUTPUT_DIR = args
    
    BuildCarousel(INPUT_DIR, OUTPUT_DIR)

######################################################################
if __name__ == '__main__':
    CheckArgs()
