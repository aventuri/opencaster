/*  
 * Copyright (C) 2009-2013  Lorenzo Pallara, l.pallara@avalpa.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#define _BSD_SOURCE 1

#include <stdio.h> 
#include <stdio_ext.h> 
#include <unistd.h> 
#include <netinet/ether.h>
#include <netinet/in.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <inttypes.h>

#define EBU_UNIT_SIZE 46


void stamp_ts (unsigned long long int ts, unsigned char* buffer) 
{
	if (buffer) {
		buffer[0] = ((ts >> 29) & 0x0F) | 0x21;
		buffer[1] = (ts >> 22) & 0xFF; 
		buffer[2] = ((ts >> 14) & 0xFF ) | 0x01;
		buffer[3] = (ts >> 7) & 0xFF;
		buffer[4] = ((ts << 1) & 0xFF ) | 0x01;
	}
}


int main(int argc, char *argv[])
{
	unsigned long long int pts_stamp = 3600;
	unsigned char* pes_packet;
	int byte_read = 0;
	int file_es = 0;
	int packet_index = 0;
	int txtunitperpes = 0;
	int pts_increment = 3600;
	
	/*  Parse args */
	if (argc > 2) {
		file_es = open(argv[1], O_RDONLY);
		txtunitperpes = atoi(argv[2]);
	} 
	if (argc > 3 ) {
		pts_stamp = strtoull(argv[3],0,0);
	}
	if (argc > 4) {
		pts_increment = atoi(argv[4]);
	}
	
	if (file_es == 0) {
		fprintf(stderr, "Usage: 'txt2pes txt.es txt_units_per_pes_packet [pts_offset [pts_increment]] > pes'\n");
		fprintf(stderr, "txt_unit_per_pes_packet increase bit rate, minimum is 1, max is 24\n");
		fprintf(stderr, "Default pts_offset and increment is 3600, means 2 fields or 1 frame\n");
		fprintf(stderr, "txt.es is 46 byte units of ebu teletext coding\n");
		return 2;
	} 
	
	unsigned short pes_size = ((txtunitperpes + 1) * EBU_UNIT_SIZE);
	pes_packet = malloc(pes_size);
	
	fprintf(stderr, "pes packet size without 6 byte header is %d\n", pes_size - 6);
	
	/* Set some init. values */
	memset(pes_packet, 0xFF, pes_size);
	pes_packet[0] = 0x00;
	pes_packet[1] = 0x00;
	pes_packet[2] = 0x01; /* prefix */
	pes_packet[3] = 0xBD; /* data txt */
	unsigned short temp = htons(pes_size - 6);
	memcpy(pes_packet + 4, &temp, sizeof(unsigned short)); 
	pes_packet[6] = 0x8F;
	pes_packet[7] = 0x80; /* flags */
	pes_packet[8] = 0x24; /* header size */
	/* 31 0xFF stuffing is here */
	pes_packet[45] = 0x10; /* ebu teletext */
	packet_index = EBU_UNIT_SIZE;
	
	/* Process the es txt file */
	byte_read = 1;
	while (byte_read) {
		/*
		byte_read = read(file_es, pes_packet + packet_index, 1);
		if (pes_packet[packet_index] == 0x10) {
			byte_read = read(file_es, pes_packet + packet_index, EBU_UNIT_SIZE);
		} else {
			byte_read = read(file_es, pes_packet + packet_index + 1, EBU_UNIT_SIZE - 1);
		}
		*/
		byte_read = read(file_es, pes_packet + packet_index, EBU_UNIT_SIZE);
		if (byte_read != 0) {
			packet_index += EBU_UNIT_SIZE;
			if (packet_index == pes_size) {
				stamp_ts(pts_stamp, pes_packet + 9);
				write(STDOUT_FILENO, pes_packet, pes_size);
				pts_stamp += pts_increment;
				packet_index = EBU_UNIT_SIZE;
			}
		} else {
			close(file_es);
			file_es = open(argv[1], O_RDONLY);
			byte_read = 1;
		}
	}
	
	if(pes_packet) {
		free(pes_packet);
	}
	
	return 0;
}

