#! /usr/bin/env python

def CRC_32(data):

    return 0xFFFFFFFF

try:
    import _crc32
    CRC_32 = _crc32.sectioncrc
except ImportError:
    print "### WRONG CRC32!!!"
    pass
