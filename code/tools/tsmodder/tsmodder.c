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


void Usage (void) {
	fprintf(stderr, "Usage: 'tsmodder input.ts [b:buffer_size] +0 pat.ts +pid pmt.ts +pid nit.ts  ... '\n");
	fprintf(stderr, "'+pid file.ts' change the packets with 'pid' with the packets from the file.ts\n");
	fprintf(stderr, "'b:buffer_size' set size of the internal buffer to many packets as buffer_size\n");
}

int main(int argc, char *argv[])
{
	unsigned int i;
	u_short pid;
	int input_read;
	int buffer_size;
	int fd_ts;			/* File descriptor of ts input file */	
	unsigned char* packet_buffer;
	unsigned char* current_packet;
	unsigned char adaptation_field;
	int pid_table[MAX_PID];
	char* pid_table_filename[MAX_PID];
	unsigned char pid_cc_table[MAX_PID];	/* PID table for the continuity counter of the TS packets */	
	unsigned char previous_cc_table[MAX_PID]; /* two packets can have the same continuity counter under some conditions... */	
	int byte_read;

	/* Open ts files */
	for (i = 0; i < MAX_PID; i++) {
		pid_table[i] = -1;
		pid_table_filename[i] = 0;
	}
	memset(pid_cc_table, 0x10,  MAX_PID);	
	memset(previous_cc_table, 0x10,  MAX_PID);
	if (argc <= 2) {
		Usage();
		return 2;
	}
	fd_ts = open(argv[1], O_RDONLY);
	if (fd_ts < 0) {
		fprintf(stderr, "Can't find file %s\n", argv[1]);
		return 2;
	}
	buffer_size = 0;
	i = 2;
	while (i<argc) {
		if (argv[i][0] == 'b' && argv[i][1] == ':') {
			buffer_size = atoi(&(argv[i][2])); 
			i++;
		} else {
			pid = atoi(&(argv[i][1]));
			if (pid < MAX_PID ) {
				fprintf(stderr, "Modify pid %d with %s\n", pid, argv[i+1]);
				pid_table[pid] = open(argv[i+1], O_RDONLY);
				pid_table_filename[pid] = argv[i+1];
				if (pid_table[pid] < 0) {
					fprintf(stderr, "Can't find file %s\n", argv[i+1]);
					return 2;	
				}
			}
			i+=2;
		}
	}
	if (buffer_size <= 0)
		buffer_size = 1;
	buffer_size *= TS_PACKET_SIZE;
	packet_buffer = malloc(buffer_size);
	if (packet_buffer == NULL) {
		fprintf(stderr, "Out of memory!\n");
		return 2;
	}
	/* Start to process the input */
	
	input_read = 1;
	while(input_read) {
	
		/* Read packets */
		input_read = read(fd_ts, packet_buffer, buffer_size);
	  
		/* Check if it's a packet to change */ 
		for (i = 0; i < buffer_size && input_read; i += TS_PACKET_SIZE) {
			current_packet = packet_buffer + i;
			memcpy(&pid, current_packet + 1, 2);	
			pid = ntohs(pid);
			pid = pid & 0x1fff;
			if (pid < MAX_PID && pid_table[pid] != -1) { /* replace packet */
				byte_read = read(pid_table[pid], current_packet, TS_PACKET_SIZE);
				if (byte_read <= 0) {
					close(pid_table[pid]);
					pid_table[pid] = open(pid_table_filename[pid], O_RDONLY);
					byte_read = read(pid_table[pid], current_packet, TS_PACKET_SIZE);
				}
				if (pid_cc_table[pid] == 0x10) { 
					pid_cc_table[pid] = current_packet[3] & 0x0f; /* new stream to track cc */
					previous_cc_table[pid] = current_packet[3] & 0x0f;
				} else { 
					adaptation_field = (current_packet[3] & 0x30) >> 4;
					if (adaptation_field == 0x0 || adaptation_field == 0x2) { /* reserved, no increment */
						current_packet[3] = (pid_cc_table[pid] | (current_packet[3] & 0xf0));
					} else if ((adaptation_field == 0x1) && ((current_packet[3] & 0x0f) == previous_cc_table[pid])) { /* double packet accepted only once */
						current_packet[3] = (pid_cc_table[pid] | (current_packet[3] & 0xf0));
						previous_cc_table[pid] = 0x10;
					} else if ((adaptation_field == 0x3) && ((current_packet[3] & 0x0f) == previous_cc_table[pid])) { /* double packet accepted only once */
						current_packet[3] = (pid_cc_table[pid] | (current_packet[3] & 0xf0));
						previous_cc_table[pid] = 0x10;
					} else { /* increase the cc and restamp */
						previous_cc_table[pid] = current_packet[3] & 0x0f;
						pid_cc_table[pid] = (pid_cc_table[pid] + 1) % 0x10; 
						current_packet[3] = (pid_cc_table[pid] | (current_packet[3] & 0xf0));
					}
				}
			}
		}
		/* Write packets */
		if (input_read) {
			write(STDOUT_FILENO, packet_buffer, buffer_size);
		}

	}
	
	return 0;
}

