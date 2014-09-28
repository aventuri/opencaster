#!/bin/bash

if [ $# -lt 3 ]
then
    /bin/echo "Usage:"
    /bin/echo "ssu-update.sh dvbssu_section_directory dsmcc_pid padding_on"
    /bin/echo "	dvbssu_section_directory: the directory to marshal in an object carousel"
    /bin/echo "	association_tag aka common tag, referenced by PMTs and AITs, every carousel has its own"
    /bin/echo "	pid, referenced by PMTs using this carousel"
    /bin/echo "	padding_on, every section is padded, usuful with some buggy decoder, waste bandwith, default off"
    /bin/echo
    /bin/echo "Example:"
    /bin/echo "ssu-update.sh dir1 2001 0"
    /bin/echo "	carousel_directory: dir1"
    /bin/echo "	pid: 2001"
    /bin/echo "	don't pad"
    exit 65
fi

#Parameters passing
OCDIR=$1
PID=$2
PAD_ON=$3

# Check if it is necessary to pad every sections or not, unluckly we have found some decoders having buggy section filtering that needed this
if [ "$PAD_ON" = "1" ]
then	
    #Every section will be padded to the minimum number of packets needed to contain it, all the packets are enqueued into the output ts file
    for file in $OCDIR/*.sec
    do
        /usr/local/bin/sec2ts $PID < $file >> $OCDIR/temp_ts
    done
    /usr/local/bin/tsfixcc $OCDIR/temp_ts > $OCDIR.ts
    rm $OCDIR/temp_ts 
else
    # All the single section files are enqueued in a single file, padding will occur only at the end of the last section
    for file in $OCDIR/*.sec
    do
	/bin/cat $file >> $OCDIR/temp_sec
    done
    /usr/local/bin/sec2ts $PID < $OCDIR/temp_sec > $OCDIR.ts
    rm $OCDIR/temp_sec
fi

