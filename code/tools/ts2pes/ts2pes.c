/*  
 * Copyright (C) 2004-2013  Lorenzo Pallara, l.pallara@avalpa.com
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
#include <unistd.h> 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <inttypes.h>

#define TS_PACKET_SIZE 188
#define MAX_PID 8192

int main(int argc, char *argv[])
{
	int byte_read;
	int fd_ts;			/* File descriptor of ts file */
	int ts_header_size;
	unsigned char temp;
	unsigned short pid;
	unsigned short payload_pid;
	unsigned int i;
	unsigned char* packet_buffer;
	unsigned char* current_packet;
	unsigned int buffer_size;

	/* Open ts file */
	buffer_size = 0;
	if (argc >= 3) {
		fd_ts = open(argv[1], O_RDONLY);
		payload_pid = atoi(argv[2]);
		if (argc > 3) {
			buffer_size = atoi(argv[3]);
		}
	} else {
		fprintf(stderr, "Usage: 'ts2pes filename.ts payload_pid [buffer_size_in_packets]'\n");
		return 2;
	}
	if (fd_ts < 0) {
		fprintf(stderr, "Can't find file %s\n", argv[1]);
		return 2;
	}
	if (payload_pid < 2 || payload_pid > MAX_PID-2) {
		fprintf(stderr, "Invalid PID, range is [2..8190]\n");
	}
	if (buffer_size <= 0) {
		if (argv[3] != 0) {
			fprintf(stderr, "Buffer size is not valid\n");
			return 2;
		} else {
			buffer_size = 1;
		}
	}
	buffer_size *= TS_PACKET_SIZE;
	packet_buffer = malloc(buffer_size);
	if (packet_buffer == NULL) {
		fprintf(stderr, "Out of memory\n");
		return 2;
	}
		
	/* Start to process the file */	
	byte_read = 1;
	while(byte_read) {

		/* read packets */
		byte_read = read(fd_ts, packet_buffer, buffer_size);
	    
		/* check packets pid */
		for (i = 0; i < buffer_size && byte_read; i+=TS_PACKET_SIZE) {
			current_packet = packet_buffer+i;
			memcpy(&pid, current_packet + 1, 2);	
			pid = ntohs(pid);
			pid = pid & 0x1fff;
			if (pid == payload_pid) { /* got the pid we are interested into */
				ts_header_size = 4;
				
				/* check adaptation field */
				temp = (current_packet[3] >> 4) & 0x03;
				if (temp == 0) {
					ts_header_size = TS_PACKET_SIZE; /* jump the packet, is invalid ?*/
					fprintf(stderr, "invalid packet!\n");
				} else if (temp == 1) {
					; /* just payload */
				} else if (temp == 2) {
					ts_header_size = TS_PACKET_SIZE; /* only adaptation field */
					/* fprintf(stderr, "adapatation field only?\n"); */
				} else if (temp == 3) {
					ts_header_size += current_packet[4] + 1; /* jump the adaptation field */
				}
				
				write(STDOUT_FILENO, current_packet + ts_header_size, TS_PACKET_SIZE - ts_header_size);
			}
		}
	}

	return 0;
}
