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
import pprint

######################################################################
def CDR(s, alignment = 4, gap_byte = 0xFF):
    """If necessary, append to 's' a trailing NUL byte
    and fill with 'gap_byte' until properly aligned.
    """

    if len(s) % alignment == 0 and s[-1] in ("\x00", "\xFF"):
        return s
    s = s + "\x00"

    while len(s) % alignment:
        s = s + chr(gap_byte)
    return s					    
					    
######################################################################
class DVBobject:
    """The base class for many protocol data units.

    Basically it provides functionality similar to a C-struct.
    Members are set via keyword arguments either in the constructor
    or via the set() method. Other (rather static) attributes
    can be defined as (sub-)class attributes.

    Subclasses must implement a 'pack()' method which returns
    the properly packed byte string.

    Attributes may come from the following sources:

    1. (Static) class attributes;

    2. Keyword arguments given in the constructor;

    3. Keyword arguments given in the 'set()' method;

    4. Direct assignment to instance attributes (NOT recommended).
    """

    #
    # Default attribute value.
    # Subclasses can do that, too!
    #
    ISO_639_language_code = "deu"

    def __init__(self, **kwargs):
        """Initialize instance attributes from keyword arguments.
        """
        apply(self.set, (), kwargs)

    def set(self, **kwargs):
        """Add (more) instance attributes from keyword arguments.
        """
        for k, v in kwargs.items():
            setattr(self, k, v)

    def __repr__(self):
        """Used for debugging."""
        def hilite(s):
            return "### %s ###" % s
        return pprint.pformat(
            (hilite(self.__class__.__name__),
             self.__dict__))

    def dump(self):
        """Print a simple hexdump of this object to stdout.
        """
        BYTES_PER_LINE = 16
        bytes = self.pack()
        i = 0
        for byte in bytes:
            if i % BYTES_PER_LINE == 0:
                if i: print             # start on a fresh line...
                print "%04x " % i,
            print "%02X" % ord(byte),
            i = i+1
        print                           # dump is done => NL
        
    def test(self):
        """Used for debugging."""
        if not self.__dict__:
            self.sample()
        self.dump()
        print self

