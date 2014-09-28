/*
 * Copyright (C) 2009-2013  Lorenzo Pallara, l.pallara@avalpa.com
 * Copyright (C) 2011  Shane Andrusiak, blendz@shaw.ca
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
 */

#include <fcntl.h>
#include <unistd.h> 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <inttypes.h>

#define TS_HEADER_SIZE 4
#define TS_PACKET_SIZE 188
#define MAX_PID 8192
#define PACK_HEADER_SIZE 4 
#define TIME_STAMP_SIZE 5

int main(int argc, char *argv[])
{
	int fd_ts;
	u_short pid;
	int byte_read;
	int tsdiscont = 0;
	int haspcr = 0;
	int firstpcr = 0;
	int ts_header_size = 0;
	unsigned int i;
	unsigned int buffer_size;
	unsigned char adapt = 0;
	unsigned char* packet_buffer;
	unsigned char* current_packet;
	unsigned long long int ts_packet_count;
	/* Open ts file */
	buffer_size = 0;
	if (argc >= 2)
		fd_ts = open(argv[1], O_RDONLY);
	else {
		fprintf(stderr, "Usage: tsdiscont inputfile.ts > outputfile.ts\n");
		fprintf(stderr, "N.B: this tool will check for and fix discontinuity problems\n");
		fprintf(stderr, "If discontinuity is found on packet without pcr it is set to 0\n");
		fprintf(stderr, "If discontinuity is not found on first pcr packet it is set to 1\n");
		return 2;
	}
	if (fd_ts < 0) {
		fprintf(stderr, "Can't find file %s\n", argv[1]);
		return 2;
	}
	buffer_size = 1;
	/* Start to process the file */
	buffer_size *= TS_PACKET_SIZE; 
	packet_buffer = malloc(buffer_size);
	if (packet_buffer == NULL) {
		fprintf(stderr, "out of memory\n");
		return 2;
	}
	ts_packet_count = 0;
	byte_read = 1;
	while(byte_read) {
		/* Read next packets */
		byte_read = read(fd_ts, packet_buffer, buffer_size);
		for (i = 0; i < buffer_size; i += TS_PACKET_SIZE) {
			current_packet = packet_buffer + i;
			memcpy(&pid, current_packet + 1, 2);	
			pid = ntohs(pid);
			pid = pid & 0x1fff;
			if ((pid < MAX_PID) && (current_packet[3] & 0x20) && (current_packet[4] != 0) && (current_packet[5] & 0x10)) { 
				haspcr=1;
				if(!firstpcr){
					tsdiscont = (current_packet[5] & 0x80) >> 7;
					fprintf(stderr,"PID: %d, Pos: %lld  - First Pcr Discont : %X\n",pid, ts_packet_count,tsdiscont); 
					if(!tsdiscont){
						current_packet[5] = current_packet[5] | 0x80;
						fprintf(stderr,"PID: %d, Pos: %lld  - Fixed first pcr discontinuity flag\n",pid, ts_packet_count); 
					}
					firstpcr=1;
				}		
			}else{
				haspcr=0;
			}
			tsdiscont = 0;
			adapt = (current_packet[3] >> 4) & 0x03;
			if (adapt == 0) {
				/* the packet is invalid ?*/
				ts_header_size = TS_PACKET_SIZE;
				fprintf(stderr,"PID: %d, Pos: %lld  - Invalid packet found\n",pid, ts_packet_count); 
			} else if (adapt == 1) {
				/* only payload , we can skip this*/
				ts_header_size = TS_HEADER_SIZE; 
			} else if (adapt == 2) { 
				/* only adaptation field , if we have pcr, check flag*/
				tsdiscont = (current_packet[5] & 0x80) >> 7;
				ts_header_size = TS_PACKET_SIZE; 
			} else if (adapt == 3) {
				/* adapt and payload */
				tsdiscont = (current_packet[5] & 0x80) >> 7;
				ts_header_size = TS_HEADER_SIZE + current_packet[4] + 1; 
			} else {
				/* hmmmm */
				ts_header_size = TS_PACKET_SIZE;
				fprintf(stderr,"PID: %d, Pos: %lld  - Invalid adaptation field flag found : %X\n",pid, ts_packet_count,adapt); 
			}
			if(tsdiscont){
				if(haspcr){
					fprintf(stderr,"PID: %d, Pos: %lld  - discontinuity found with pcr : %X\n",pid, ts_packet_count,adapt); 
				}else{
					fprintf(stderr,"PID: %d, Pos: %lld  - Fixing discontinuity found WITHOUT pcr : %X\n",pid, ts_packet_count,adapt); 
					current_packet[5] = current_packet[5] & 0x7F;
				}
			}
			ts_packet_count++;		
		}
		write(STDOUT_FILENO, packet_buffer, buffer_size);
	}
	return 0;
}
