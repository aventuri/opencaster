#!/bin/bash

if [ $# -lt 5 ]
then
	/bin/echo "Usage:"
	/bin/echo "oc-update.sh object_carousel_directory association_tag module_version dsmcc_pid carousel_id [compress_mode] [padding_on] [clean_off] [DDB_size] [update_flag] [mount_frequency]"
	/bin/echo "	- carousel_directory: the directory to marshal in an object carousel"
	/bin/echo "	- association_tag aka common tag, referenced by PMTs and AITs, every carousel has one"
	/bin/echo "	- modules_version, all the modules will have the same version, you need to change this to notify to the box files are changed, goes from 0 to 15"
	/bin/echo "	- pid, referenced by PMTs using this carousel"
	/bin/echo "	- carousel_id, referenced by PMTs using this carousel, every carousel has its own, it is an alternative for association_tag, they have the same function"
	/bin/echo "	- compress_mode, 0: don't compress, 1:compress all, 2:smart compress, file with .solo extension are set in an uncompressed module alone to allow use cases like quick image file update, default is 2"
	/bin/echo "	- padding_on, every section is padded, was useful with some buggy decoder, waste bandwidth, default off, unsupported since OpenCaster 2.4.8"
	/bin/echo "	- clean_off, don't delete temp file, default off, used for debug"
	/bin/echo "	- DDB_size, Use custom size for DDB payload, default = max = 4066"
	/bin/echo "	- sets the Update flag in the TransactionID of DSI and DII to the value given (0 or 1)"
	/bin/echo "	- mount_frequency, set how often insert DII/DSI/SGW to speed up carousel mount, default is twice per carousel period"
	/bin/echo
	/bin/echo "Example:"
	/bin/echo "oc-update.sh ocdir1 0xB 5 2003 7 2 0 0 4066 0 2"
	/bin/echo "	carousel_directory: ocdir1"
	/bin/echo "	association_tag: 0xB (11)"
	/bin/echo "	modules_version: 0x5 (5)"
	/bin/echo "	pid: 2001"
	/bin/echo "	carousel_id: 7"
	/bin/echo "	smart compress the carousel"
	/bin/echo "	don't pad"
	/bin/echo "	delete temp files"
	/bin/echo "	use 4066 bytes for DDB size (usual and maximum size)"
	/bin/echo "	sets update flag to 0 in DSI/DII"
	/bin/echo "	insert DSI/DII/SGW twice per carousel length"
	exit 65
fi

#Parameters passing
OCDIR=$1
ASSOCIATION_TAG=$2
MODULE_VERSION=$3
PID=$4
CAROUSEL_ID=$5
COMPRESS_MODE="2"
PAD_ON="0"
NO_DELETE_TEMP="0"
BLOCK_SIZE="4066"
UPDATE_FLAG="0"
MOUNT_PERIOD="2"
if [ $# -gt 5 ]
then
COMPRESS_MODE=$6
fi
if [ $# -gt 6 ]
then
PAD_ON=$
fi
if [ $# -gt 7 ]
then
NO_DELETE_TEMP=$8
fi
if [ $# -gt 8 ]
then
BLOCK_SIZE=$9
fi
if [ $# -gt 9 ]
then
UPDATE_FLAG=${10}
fi
if [ $# -gt 10 ]
then
MOUNT_PERIOD=${11}
fi

#Generate temp directories
TEMP_DIR_MOD=`/bin/mktemp -d`
TEMP_DIR_SEC=`/bin/mktemp -d`

#Generate the modules from the directory, the modules are stored into a tmp directory TEMP_DIR_MOD
/usr/local/bin/file2mod.py $OCDIR $TEMP_DIR_MOD $CAROUSEL_ID $CAROUSEL_ID $ASSOCIATION_TAG $MODULE_VERSION $BLOCK_SIZE $UPDATE_FLAG $COMPRESS_MODE

#Compress all modules
if [ "$COMPRESS_MODE" = "1" ]
then
	for file in $TEMP_DIR_MOD/*.mod
	do
		/usr/bin/du --apparent-size --block-size 1 $file > $file.size
		/usr/local/bin/zpipe < $file > $file.z 
		/bin/mv $file.z $file
	done
fi

#Compress only "not solo" modules (.jpg, .gif, .png and .solo)
if [ "$COMPRESS_MODE" = "2" ]
then
	for file in $TEMP_DIR_MOD/*.mod
	do
		if [ ! -f $file.solo ]
		then
			/usr/bin/du --apparent-size --block-size 1 $file > $file.size
			/usr/local/bin/zpipe < $file > $file.z 
			/bin/mv $file.z $file
		else
			/bin/rm $file.solo
		fi
	done
fi

#Generate sections from modules, the sections are stored into a tmp directory TEMP_DIR_SEC
/usr/local/bin/mod2sec.py $TEMP_DIR_MOD $TEMP_DIR_SEC

# Check if it is necessary to pad every sections or not, unluckly we have found some decoders having buggy section filtering that needed this
if [ "$PAD_ON" = "1" ]
then
	echo "UNSUPPORTED SINCE 2.4.8"
else
	# All the single section files are enqueued in a single file, padding will occur only at the end of the last section
	# DSI, DII and sgw are repeated more frequently
	file_number=0
	for file in $TEMP_DIR_SEC/*.sec
	do
		let file_number=$file_number+1
	done
	echo "total carousel section number is $file_number"
	period=$(($file_number/$MOUNT_PERIOD))
	echo "carousel head period is every $period section"
	counter=-1
	for file in $TEMP_DIR_SEC/*.sec
	do
		/bin/cat $file >> $TEMP_DIR_SEC/temp_sec
		let counter=$counter-1
		#echo "counter is $counter"
		if [ $counter -lt 0 ]; then
			let counter=$period
			/bin/cat $TEMP_DIR_SEC/DSI.sec >> $TEMP_DIR_SEC/temp_sec
			/bin/cat $TEMP_DIR_SEC/DII.sec >> $TEMP_DIR_SEC/temp_sec
			/bin/cat $TEMP_DIR_SEC/0001_000000.sec >> $TEMP_DIR_SEC/temp_sec # SGW is 1 section of module id 1
		fi
	done
	/usr/local/bin/sec2ts $PID < $TEMP_DIR_SEC/temp_sec > $OCDIR.ts
fi

# Delete temp files
if [ "$NO_DELETE_TEMP" = "0" ]
then
/bin/rm -rf $TEMP_DIR_MOD
/bin/rm -rf $TEMP_DIR_SEC
else
/bin/echo "Modules generated in $TEMP_DIR_MOD were not deleted"
/bin/echo "Sections generated in $TEMP_DIR_SEC were not deleted"
fi

