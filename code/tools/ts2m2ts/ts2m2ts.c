/*
 * Copyright (C) 2011-2013, Lorenzo Pallara l.pallara@avalpa.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <netinet/in.h>
#include <inttypes.h>

#define TS_PACKET_SIZE 188
#define TS_EXTRA_HEADER 4
#define MAX_PID 8191
#define SYSTEM_CLOCK_FREQUENCY 2700000


int main (int argc, char *argv[]) {

	int len = 0;
	char* tsfile = 0;
	unsigned short pid = MAX_PID;
	int transport_fd = -1;
	unsigned int bitrate = 48000000;
	int completed = 1;
	unsigned char copyright_bits = 0;
	unsigned char output_packet[TS_EXTRA_HEADER + TS_PACKET_SIZE];
	unsigned long long int ts_packet_count = 0;
	unsigned long long int timestamp = 0;
	unsigned long long int temp = 0;
	unsigned long long int carriage = 0;
	
	if(argc < 3 ) {
		fprintf(stderr, "Usage: %s file.ts bitrate_in_bps [copyright_bits_as_integer] > file.m2ts \n", argv[0]);
		return 0;
	} else {
		tsfile = argv[1];
		bitrate = atoi(argv[2]);
		if (argc > 3) {
			copyright_bits = atoi(argv[3]);
		}
	}
	transport_fd = open(tsfile, O_RDONLY);
	if(transport_fd < 0) {
		fprintf(stderr, "can't open file %s\n", tsfile);
		return 0;
	}
	
	completed = 0;
	ts_packet_count = 0;
	while (!completed) {
		len = read(transport_fd, output_packet + TS_EXTRA_HEADER, TS_PACKET_SIZE);
		if(len < 0) {
			fprintf(stderr, "ts file read error \n");
			completed = 1;
		} else if (len == 0) {
			completed = 1;
		} else {
			memcpy(&pid, output_packet + TS_EXTRA_HEADER + 1, 2);
			pid = ntohs(pid);
			pid = pid & 0x1fff;
			if (pid < MAX_PID) { /* drop null packets */
			
				temp = ts_packet_count * TS_PACKET_SIZE * 8 * SYSTEM_CLOCK_FREQUENCY;
				temp /= bitrate;
				temp += carriage;
				timestamp = temp;
				
				output_packet[0] = ((copyright_bits & 0x03) << 6) | ((timestamp >> 24) & 0x3F);
				output_packet[1] = (timestamp >> 16) & 0xFF;
				output_packet[2] = (timestamp >> 8) & 0xFF;
				output_packet[3] = timestamp & 0xFF;
				write(STDOUT_FILENO, output_packet, TS_EXTRA_HEADER + TS_PACKET_SIZE);
			}
			
			/* manage overflown */
			if (ts_packet_count * 2 >= 454263790) {
				ts_packet_count = 0;
				carriage = timestamp;
			}
			
			ts_packet_count++; 
		}
	}
	close(transport_fd);
	return 0;
}
