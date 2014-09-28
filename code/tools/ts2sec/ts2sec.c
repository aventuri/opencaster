/*  
 * Copyright (C) 2008-2013 Lorenzo Pallara, l.pallara@avalpa.com
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.

 * Last modification :
 * Frederic Delagree, Frederic.delagree@orange.com, 4 April 2012
 * Take into account the value of pointer_field value into section. 
 */


#include <netinet/in.h>

#include <fcntl.h>
#include <unistd.h> 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <inttypes.h>

#define TS_PACKET_SIZE 188
#define MAX_TABLE_LEN 4096
#define MAX_PID 8192
#define SECTION_HEADER_SIZE 3

#include "../../libs/sectioncrc/sectioncrc.c"

unsigned char section[MAX_TABLE_LEN];
int section_len = 0;

/*
use this to output sections with padding for test and development
void add_payload(unsigned char* buffer, int size) {
        write(STDOUT_FILENO, buffer, size);
}
*/

void add_payload(unsigned char* buffer, int size) {

	unsigned short section_header_len = 0;
	unsigned int payload = 0;
	unsigned int crc = 0;
	int written = 0;
	
	while (size > 0) {
	
		while (section_len < SECTION_HEADER_SIZE) { /* current section is without a complete header */
		
			/* get header bytes */
			section[section_len] = buffer[0];
			section_len += 1;
			buffer += 1;
			size -= 1;
			
			/* check if it is header or padding */		
			if (section_len == SECTION_HEADER_SIZE) {
				if(section[0] == 0xFF) {
					section[0] = section[1];
					section[1] = section[2];
					section_len = 2;
				}
			}
		
			/* chek if there is more payload */
			if (size <= 0) {
				return;
			}
		
		}
		
		/* get section size from the header */
		memcpy(&section_header_len, section + 1, 2);
		section_header_len = ntohs(section_header_len);
		section_header_len &= 0x0FFF;
		section_header_len += SECTION_HEADER_SIZE;
	
		/* add payload without padding */
		if (section_header_len - section_len >= size) { 
			payload = size;
		} else {
			payload = section_header_len - section_len;
		}
		memcpy(section + section_len, buffer, payload);
		section_len += payload;
		buffer+= payload;
		size -= payload;
	
		/* output the section only if completed */
		if (section_len == section_header_len) {
			if (section_len >= 4) { 
				crc = sectioncrc(section, section_len - 4);
				if ( (section[section_len - 4] == ((crc >> 24) & 0xFF)) &&
					(section[section_len - 3] == ((crc >> 16) & 0xFF)) &&
					(section[section_len - 2] == ((crc >> 8) & 0xFF)) &&
					(section[section_len - 1] == (crc & 0xFF)) 
				)  { 
					written = write(STDOUT_FILENO, section, section_len);
					if (written != section_len ) {
						fprintf(stderr, "warning: written %d bytes instead of %d\n", written, section_len);
					}
					
				} else {
					fprintf(stderr, "crc failed: %x %x %x %x %x %x %x %x\n",
						section[section_len - 4], ((crc >> 24) & 0xFF),
						section[section_len - 3], ((crc >> 16) & 0xFF),
						section[section_len - 2], ((crc >> 8) & 0xFF),
						section[section_len - 1], (crc & 0xFF) 
					);
				}
				section_len = 0;
			} else {
				section_len = 0;
				size = 0;
			}
		}
	}
}

int main(int argc, char *argv[])
{
	int byte_read;
	int fd_ts;			/* File descriptor of ts file */
	int ts_header_size;
	unsigned char adapt_field;
	unsigned short pid;
	unsigned short payload_pid;
	unsigned char current_packet[TS_PACKET_SIZE];
	int pusi = 0;

	/* Open ts file */
	if (argc >= 3) {
		fd_ts = open(argv[1], O_RDONLY);
		payload_pid = atoi(argv[2]);
	} else {
		fprintf(stderr, "Usage: 'ts2sec filename.ts payload_pid > sections'\n");
		fprintf(stderr, "the tool manages corrupted ts, only complete sections are output\n");
		return 2;
	}
	if (fd_ts < 0) {
		fprintf(stderr, "Can't find file %s\n", argv[1]);
		return 2;
	}
	if (payload_pid > MAX_PID-2) {
		fprintf(stderr, "Invalid PID, range is [0..8190]\n");
	}
	
	/* Start to process the file */	
	byte_read = 1;
	while(byte_read) {
		
		/* read a ts packet */
		byte_read = read(fd_ts, current_packet, TS_PACKET_SIZE);
		
		/* check packet pid */
		memcpy(&pid, current_packet + 1, 2);	
		pid = ntohs(pid);
		pid = pid & 0x1fff;
		
		if (pid == payload_pid && byte_read > 0) { /* got the pid we are looking for */
			/* check for sync byte */
			if (current_packet[0] != 0x47) {
				fprintf(stderr, "missing sync byte, quitting\n");
				return 0;
			}
			/* check for pusi */
			pusi = (current_packet[1] & 0x40);
			/* check adaptation field size */
			ts_header_size = 4;
			adapt_field = (current_packet[3] >> 4) & 0x03;
			if (adapt_field == 0) {
				ts_header_size = TS_PACKET_SIZE; /* jump the packet, is invalid ? */
				 fprintf(stderr, "invalid packet!\n"); 			
			} else if (adapt_field == 1) {
				 /* fprintf(stderr, "regular case\n"); 			*/
				; /* regular case */
			} else if (adapt_field == 2) {
				/* fprintf(stderr, "adapt_field == 2\n"); 			*/
				ts_header_size = TS_PACKET_SIZE; /* this packet has only adaptation field */
				/* fprintf(stderr, "adapatation field only?\n"); */
			} else if (adapt_field == 3) {
				/* fprintf(stderr, "adapt_field == 3\n"); 			*/
				ts_header_size += current_packet[ts_header_size] + 1;
				/* fprintf(stderr, "adaptation field shouldn't be set for sections\n"); */
			}
			if (ts_header_size < TS_PACKET_SIZE) {
				if (pusi) {
/*				 fprintf(stderr, "new section, payload starts at %d,tid=%d\n", current_packet[ts_header_size],current_packet[ts_header_size+current_packet[ts_header_size]+1]); */
				if (current_packet[ts_header_size] > 0 ) 
					add_payload(current_packet + ts_header_size+1, current_packet[ts_header_size] );

					add_payload(current_packet + ts_header_size + 1 +current_packet[ts_header_size], TS_PACKET_SIZE - ts_header_size - 1 -current_packet[ts_header_size]); 
				/*	add_payload(current_packet + ts_header_size + 1, TS_PACKET_SIZE - ts_header_size - 1); */
				} else {
					add_payload(current_packet + ts_header_size, TS_PACKET_SIZE - ts_header_size);
				}
			}
		}
	}	
	return 0;
}
