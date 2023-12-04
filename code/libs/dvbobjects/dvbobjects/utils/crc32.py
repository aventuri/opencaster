#! /usr/bin/env python

def CRC_32(data):

    return 0xFFFFFFFF


try:
    import crcmod

    CRC_32 = crcmod.predefined.mkPredefinedCrcFun('crc-32-mpeg')

except ImportError:
    print("### WRONG CRC32!!!")
    pass
