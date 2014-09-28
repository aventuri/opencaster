#! /usr/bin/env python

# 
# Copyright 2011, Lorenzo Pallara l.pallara@avalpa.com
#

# country codes
# 000  English  or  Polish
# 001  German
# 010  Swedish
# 011  Italian
# 100  French
# 101  Spanish  or  Serbo-croat
# 110  Turkish  or  Czechoslovakia
# 111  Rumanian

import string
import sys
from dvbobjects.MPEG.EBUTeletext import *
from dvbobjects.utils import *

INPUT_FILE = sys.argv[1];

INPUT_FILE = (string.split(INPUT_FILE, ".", 1))[0];

magazine_number = 0x7
page_number = 0x77
fake_magazine_number = 0x7
fake_page_number = 0x78
print "subtitle page is 777"
print "empty page used for timing is 778"

out = open(INPUT_FILE + ".es", "wb");
teletextunits = EBUTeletextUnits(
	unit_loop = [
		EBUTeletext(
			data_unit_id = 0x03, # 0x03 subtitles, 0x05 impaired
			field_parity = 01,
			line_offset = 0x8,
			magazine = magazine_number,
			row = 0x00, # 24 (0x17) lines per magazine, 0 is the page header
			page = page_number,
			subpage = 0,
			erase_page = 1,
			newsflash = 0,
			subtitle = 1,
			suppress_header = 1,
			update_indicator = 1,
			interrupted_sequence = 0,
			inhibit_display = 0,
			magazine_serial = 1,
			country_code = 0x03,
			chars = "777                             ",
		),
		EBUTeletext(
			data_unit_id = 0xFF, 
		),
		EBUTeletext(
			data_unit_id = 0xFF, 
		),
		EBUTeletext(
			data_unit_id = 0xFF, 
		),
		EBUTeletext(
			data_unit_id = 0xFF, 
		),
		EBUTeletext(
			data_unit_id = 0xFF, 
		),
		EBUTeletext(
			data_unit_id = 0xFF, 
		),
		EBUTeletext(
			data_unit_id = 0xFF, 
		),
		EBUTeletext(
			data_unit_id = 0xFF, 
		),
		EBUTeletext(
			data_unit_id = 0xFF, 
		),
		EBUTeletext(
			data_unit_id = 0xFF, 
		),
		EBUTeletext(
			data_unit_id = 0xFF, 
		),
		EBUTeletext(
			data_unit_id = 0xFF, 
		),
		EBUTeletext(
			data_unit_id = 0xFF, 
		),
		EBUTeletext(
			data_unit_id = 0xFF, 
		),
	]
)

print "start parsing:"

page_counter = 0
last_end_time = 0
payloadData = open(INPUT_FILE + ".srt", "rb")
number = payloadData.readline()
while (number != ""):
	
	print "number:" + number.strip("\x0A")
	# number is not necessary
	
	time = payloadData.readline()
	if (time != ""):
		striptime = time.strip("\x0A")
		splittime = time.split(":")
		start_hour = splittime[0]
		start_minute = splittime[1]
		start_second = splittime[2].split(",")[0]
		start_millisecond = (splittime[2].split(",")[1]).split()[0]
		end_hour = splittime[2].split("> ")[1]
		end_minute = splittime[3]
		end_second = splittime[4].split(",")[0]
		end_millisecond = splittime[4].split(",")[1].strip()
		print "start time:" + start_hour + ":" +  start_minute + ":" + start_second
		print "end time:" + end_hour + ":" +  end_minute + ":" + end_second 
		start_time = ( int(start_hour) * 3600 ) + ( int(start_minute) * 60 ) + int(start_second)
		end_time = ( int(end_hour) * 3600 ) + ( int(end_minute) * 60 ) + int(end_second)
		if ( int(start_millisecond) > 500 ):
			start_time = ( start_time * 2 ) + 1
		else:
			start_time = start_time * 2
		if ( int(end_millisecond) > 500 ):
			end_time = ( end_time * 2 ) + 1
		else:
			end_time = end_time * 2
		
		print "last time half seconds:" + str(last_end_time)
		print "start time half seconds:" + str(start_time)
		#1 page is 690 bytes -> 4 ts packets, we round to half second so: 188*4*8*2 -> 12032 bps
		#it is necessary to send stuffing pages up to start time
		for i in range( (start_time - 1 ) - last_end_time ) :
		
			if (i == 0):
				# now it is necessary to send an empty page to blank the previous one
				teletextunits.unit_loop[0].magazine = magazine_number
				teletextunits.unit_loop[0].page = page_number
				teletextunits.unit_loop[0].chars = "777                             "
				teletextunits.unit_loop[0].subtitle = 1
				teletextunits.unit_loop[0].interrupted_sequence = 1
				teletextunits.unit_loop[9] = EBUTeletext( data_unit_id = 0xFF )
				teletextunits.unit_loop[10] = EBUTeletext( data_unit_id = 0xFF )
			else:
				teletextunits.unit_loop[0].magazine = fake_magazine_number
				teletextunits.unit_loop[0].page = fake_page_number
				teletextunits.unit_loop[0].subtitle = 0
				teletextunits.unit_loop[0].interrupted_sequence = 0
				teletextunits.unit_loop[0].chars = "778                             "
				teletextunits.unit_loop[9] = EBUTeletext( data_unit_id = 0xFF )
				teletextunits.unit_loop[10] = EBUTeletext( data_unit_id = 0xFF )
			out.write(teletextunits.pack())
			page_counter += 1
			print "txt time:" + str(page_counter)
	
	# now send 2 text lines
	textline = payloadData.readline()
	i = 0
	teletextunits.unit_loop[0].magazine = magazine_number
	teletextunits.unit_loop[0].page = page_number
	teletextunits.unit_loop[0].chars = "777                             "
	teletextunits.unit_loop[0].subtitle = 1
	teletextunits.unit_loop[0].interrupted_sequence = 1
	teletextunits.unit_loop[9] = EBUTeletext( data_unit_id = 0xFF )
	teletextunits.unit_loop[10] = EBUTeletext( data_unit_id = 0xFF )
	while (textline != "\x0A" and textline != ""):
		text = ""
		if (i == 0 or i == 1):
			text = textline.strip("\x0A")[:35]
			print "insert text:" + text
			j = 0
			while (len(text) < 35):
				if (j == 0):
					text = text + " "
					j = 1
				else:
					text = " " +  text
					j = 0
		if (i == 0):
			teletextunits.unit_loop[9] = EBUTeletext(
					data_unit_id = 0x03,
					field_parity = 0,
					line_offset = 0x8,
					magazine = magazine_number,
					row = 0x14,
					chars = "\x0D\x0B\x0B" + text + "\x0A\x0A",
					)
			i = 1
		elif (i == 1):
			teletextunits.unit_loop[10] = EBUTeletext(
					data_unit_id = 0x03,
					field_parity = 0,
					line_offset = 0x9,
					magazine = magazine_number,
					row = 0x16,
					chars = "\x0D\x0B\x0B" + text + "\x0A\x0A",
					)
			i = 2
		else:
			print "drop text:" + textline.strip("\x0A")
		textline = payloadData.readline()
	out.write(teletextunits.pack());
	page_counter += 1
	print "txt time:" + str(page_counter)
	
	# now it is necessary to send stuffing pages up to end time minus 1
	print "end time half seconds:" + str(end_time)
	for i in range( end_time - start_time ) :
		teletextunits.unit_loop[0].magazine = fake_magazine_number
		teletextunits.unit_loop[0].page = fake_page_number
		teletextunits.unit_loop[0].subtitle = 0
		teletextunits.unit_loop[0].interrupted_sequence = 0
		teletextunits.unit_loop[0].chars = "778                             "
		teletextunits.unit_loop[9] = EBUTeletext( data_unit_id = 0xFF )
		teletextunits.unit_loop[10] = EBUTeletext( data_unit_id = 0xFF )
		if (page_counter + 1 <= end_time):
			out.write(teletextunits.pack());
			page_counter += 1
			print "txt time:" + str(page_counter)
	

	last_end_time = end_time
	number = payloadData.readline()
payloadData.close;
out.close;

