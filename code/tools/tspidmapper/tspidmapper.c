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
	unsigned int i;
	u_short temp;
	int byte_read;
	int fd_ts;			/* File descriptor of ts file */
	u_short pid;
	unsigned int buffer_size;
	u_short pid_map_table[MAX_PID];	/* PID map table for the TS packets */	
	unsigned char* packet_buffer;
	unsigned char* current_packet;

	/* Open ts file */
	buffer_size = 0;
	if (argc >= 5)
		fd_ts = open(argv[1], O_RDONLY);
	else {
		fprintf(stderr, "Usage: 'tspidmapper input.ts [b:buffer_size_in_packets] PID1 to PID2 and PID3 to PID4 and ... '\n");
		return 2;
	}
	if (fd_ts < 0) {
		fprintf(stderr, "Can't find file %s\n", argv[1]);
		return 2;
	}
	for (i = 0; i < MAX_PID; i++) {
		pid_map_table[i] = MAX_PID;
	}
	
	if ((argv[2] != 0) && (argv[2][0] == 'b') && (argv[2][1] == ':')) {
		buffer_size = atoi(&(argv[2][2]));
		if (buffer_size == 0) {
			fprintf(stderr, "invalid buffer size\n");
			return 2;
		}
		i = 3;
	} else {
		i = 2;
	}
	for (; i < argc ; i+=4 ) {
		temp = atoi(argv[i]);
		if (temp < MAX_PID) {
			pid_map_table[temp] = atoi(argv[i+2]);
			fprintf(stderr, "Change %d to %d\n", temp, pid_map_table[temp]);
		}	
	}
	if (buffer_size == 0)
		buffer_size = 1;
	
	/* Start to process the file */	
	buffer_size *= TS_PACKET_SIZE;
	packet_buffer = malloc(buffer_size);
	if (packet_buffer == NULL) {
		fprintf(stderr, "out of memory\n");
		return 2;
	}
	byte_read = 1;
	while(byte_read) {
	
		/* Read next packets */
		byte_read = read(fd_ts, packet_buffer, buffer_size);
	    
		/* change pid */
		for (i = 0; i < buffer_size; i += TS_PACKET_SIZE) {
			current_packet = packet_buffer + i;	
			memcpy(&pid, current_packet + 1, 2);	
			pid = ntohs(pid);
			pid = pid & 0x1fff;
			if (pid < MAX_PID) {
				if (pid_map_table[pid] != MAX_PID) {
					memcpy(&temp, current_packet + 1, 2);
					temp = ntohs(temp);
					temp = temp & 0xe000;
					temp = (pid_map_table[pid] & 0x1fff) | temp;
					temp = htons(temp);
					memcpy(current_packet + 1, &temp, 2);
				}
			}
		}
	    
		/* Write packet buffer */
		if (byte_read)
			write(STDOUT_FILENO, packet_buffer, buffer_size);
	}

	return 0;
}
