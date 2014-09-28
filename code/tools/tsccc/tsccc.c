/*  
 * Copyright (C) 2009-2013  Lorenzo Pallara, lpallara@avalpa.com
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
#define MAX_PID 8191

int main(int argc, char *argv[])
{
	int open_file;
	int byte_read;
	int fd_ts;			/* File descriptor of ts file */
	unsigned short pid;
	unsigned int adaptation_field;
	unsigned char packet[TS_PACKET_SIZE];
	unsigned char pid_cc_table[MAX_PID];	/* PID table for the continuity counter of the TS packets */
	unsigned char repeated_cc_table[MAX_PID];
	unsigned long long position = 0;

	/* Open first ts file */
	if (argc >= 2) {
		open_file = 1;
		fd_ts = open(argv[open_file], O_RDONLY);
		if (fd_ts < 0) {
			fprintf(stderr, "Can't find file %s\n", argv[open_file]);
			return 2;
		}
	} else {
		fprintf(stderr, "Usage: 'tsccc filename.ts '\n");
		fprintf(stderr, "Tsccc check continuity counter errors and warnings\n");
		return 2;
	}
		
	/* Start to process the file */
	memset(pid_cc_table, 0x10,  MAX_PID);
	memset(repeated_cc_table, 0,  MAX_PID);
	
	while(1) {
		
		/* read packets */
		byte_read = 0;
		byte_read = read(fd_ts, packet, TS_PACKET_SIZE);
		if (byte_read < TS_PACKET_SIZE) {
			return 0;
		} 
		
		/* check cc */
		memcpy(&pid, packet + 1, 2);
		pid = ntohs(pid);
		pid = pid & 0x1fff;
		if (pid < MAX_PID) {
			adaptation_field = (packet[3] & 0x30) >> 4;
			if (pid_cc_table[pid] == 0x10) { 
				fprintf(stderr, "new pid entry %d\n", pid);
			} else {
				if (((pid_cc_table[pid] + 1) % 16) != (packet[3] & 0xF)) {
					if (adaptation_field == 0x0 || adaptation_field == 0x2) { /* reserved, no increment */
						;
					} else if ((adaptation_field == 0x1) && ((packet[3] & 0x0f) == pid_cc_table[pid]) && (!repeated_cc_table[pid])) { /* double packet accepted only once */
						repeated_cc_table[pid] = 1;
					} else if ((adaptation_field == 0x3) && ((packet[3] & 0x0f) == pid_cc_table[pid]) && (!repeated_cc_table[pid])) { /* double packet accepted only once */
						repeated_cc_table[pid] = 1;
					} else {
						fprintf(stdout, "cc warning at pid %d packet: %llu, expected %d, found %d\n", pid, position, ((pid_cc_table[pid] + 1) % 0xF), (packet[3] & 0xF));
					}
				}
			}
			pid_cc_table[pid] = packet[3] & 0xF;
		}
		position++;
	}
	
}

