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

void Usage(void) {
	fprintf(stderr, "Usage: 'tsmask filename.ts -pid1 -pid2 ... -pidn [buffer_size_in_packets]'\n");
	fprintf(stderr, "-pid removes 'pid' packets\n");
}

int main(int argc, char *argv[])
{
	int byte_read;
	int fd_ts;			/* File descriptor of ts file */
	unsigned short pid;
	unsigned int i;
	unsigned char* packet_buffer;
	unsigned char* current_packet;
	unsigned int buffer_size;
	unsigned char pid_table[MAX_PID];	/* valid PID table */	

	/* Parse input arguments */
	buffer_size = 0;
	memset(pid_table, 0,  MAX_PID);	
	if (argc >= 2) {
		fd_ts = open(argv[1], O_RDONLY);
		if (fd_ts < 0) {
			fprintf(stderr, "Can't find file %s\n", argv[1]);
			return 2;
		}
	} else {
		Usage();
		return 2;
	}
	i = 2;
	while (i < argc) {
		if (argv[i][0] == '-') {
			pid =  atoi(&(argv[i][1]));
			if ( pid < MAX_PID) {
				fprintf(stderr, "remove pid %d\n", pid);
				pid_table[pid] = 1;
				i++;
			} else {
				fprintf(stderr, "pid range should be from 0 to %d\n", MAX_PID);
				return 2;
			}
		} else {
			buffer_size = atoi(&(argv[i][0]));
			i++;
		}
	}
	if (buffer_size == 0) {
		buffer_size = 1;
	}
	
	/* Allocate packet buffer */
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
	    
		/* filter packets on their pids */
		for (i = 0; i < buffer_size; i+=TS_PACKET_SIZE) {
			current_packet = packet_buffer+i;
			memcpy(&pid, current_packet + 1, 2);
			pid = ntohs(pid);
			pid = pid & 0x1fff;
			if (pid < MAX_PID) {
				if (pid_table[pid] == 0) { 
					write(STDOUT_FILENO, current_packet, TS_PACKET_SIZE);
				} 
			}
		}
	}

	return 0;
}

