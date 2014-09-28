/*  
 * Copyright (C) 2005-2013  Lorenzo Pallara, l.pallara@avalpa.com
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

#include <netinet/in.h>

#include <fcntl.h>
#include <time.h>
#include <unistd.h> 
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <inttypes.h>

#include "../../libs/sectioncrc/sectioncrc.c"

#define TS_PACKET_SIZE 188
#define MAX_PIDS 8192
#define TDT_PID 20
#define TDT_TABLE_ID 0x70
#define TOT_TABLE_ID 0x73

void Usage (void) {
	fprintf(stderr, "Usage: 'tstdt input.ts [b:buffer_size] [t:start_time_stamp] '\n");
	fprintf(stderr, "'replace tdt packet with another tdt packe of the current time' \n");
	fprintf(stderr, "'b:buffer_size' set size of the internal buffer to many packets as buffer_size\n");
	fprintf(stderr, "'t:start_time_stamp' start the generated time from this GM timestamp for replay purposes\n");
}

int main(int argc, char *argv[])
{
	unsigned int i;
	time_t tim;
	time_t baseline = 0;
	time_t start_time_stamp = 0;
	struct tm * now;
	unsigned short l;
	unsigned short MJD;
	unsigned char hour;
	unsigned char minute;
	unsigned char second;
	u_short pid;
	int buffer_size;
	int fd_ts;			/* File descriptor of ts input file */	
	unsigned char* packet_buffer;
	unsigned char* current_packet;

	/* Open ts files */
	if (argc < 2) {
		Usage();
		return 2;
	}
	fd_ts = open(argv[1], O_RDONLY);
	if (fd_ts < 0) {
		fprintf(stderr, "Can't find file %s\n", argv[1]);
		return 2;
	}
	buffer_size = 0;
	if (argc == 3) {
		if (argv[2][0] == 'b' && argv[2][1] == ':') {
			buffer_size = atoi(&(argv[2][2])); 
		}
		if (argv[2][0] == 't' && argv[2][1] == ':') {
			start_time_stamp = (time_t) atoi(&(argv[2][2])); 
			baseline = time(NULL);
		}
	}
	if (buffer_size <= 0) {
		buffer_size = 1;
	}
	buffer_size *= TS_PACKET_SIZE;
	packet_buffer = malloc(buffer_size);
	if (packet_buffer == NULL) {
		fprintf(stderr, "Out of memory!\n");
		return 2;
	}

	/* Start to process the input */
	int bytes = 1;
	unsigned int crc = 0;
	unsigned short temp = 0;
	unsigned short section_size = 0;
	while(bytes != 0) {
	
		/* Read packets */
		bytes = read(fd_ts, packet_buffer, buffer_size);
	  
		/* Check if it's a tdt packet */ 
		for (i = 0; i < buffer_size; i += TS_PACKET_SIZE) {
			current_packet = packet_buffer + i;
			memcpy(&pid, current_packet + 1, 2);
			pid = ntohs(pid);
			pid = pid & 0x1fff;
			if (pid == TDT_PID) { 
				tim = time(NULL);
				if (start_time_stamp) {
					tim = start_time_stamp + (tim - baseline);
				}
				now = gmtime(&tim);
				if (now != NULL) {
					/* convert date into modified julian */
					if ((now->tm_mon + 1 == 1) ||  (now->tm_mon + 1  == 2)) {
						l = 1;
				} else {
						l = 0;
					}
					MJD = 14956 + now->tm_mday + (unsigned short)((now->tm_year - l) * 365.25f) + (unsigned short)((now->tm_mon + 1 + 1 + l * 12) * 30.6001f); 
					MJD = htons(MJD);
					memcpy(current_packet + 8, &MJD, 2); 
					/* convert time */
					hour = (now->tm_hour / 10) << 4 | (now->tm_hour % 10);
					minute = (now->tm_min / 10) << 4 | (now->tm_min % 10);
					second = (now->tm_sec / 10) << 4 | (now->tm_sec %10);
					current_packet[10] = hour;
					current_packet[11] = minute;
					current_packet[12] = second;
				}
				if (current_packet[5] == TOT_TABLE_ID) {
					memcpy(&temp, current_packet + 6, 2);
					temp = ntohs(temp);
					temp &= 0x0FFF;
					section_size = temp + 3;
					crc = sectioncrc(current_packet + 5, section_size - 4);
					current_packet[5 + section_size - 4] = (crc >> 24) & 0xFF;
					current_packet[5 + section_size - 3] = (crc >> 16) & 0xFF;
					current_packet[5 + section_size - 2] = (crc >> 8) & 0xFF;
					current_packet[5 + section_size - 1] = crc & 0xFF;
				}
			} 
			
		}

		/* Write packets */
		if (bytes != 0) {
			write(STDOUT_FILENO, packet_buffer, buffer_size);
		}

	}
	
	return 0;
}

