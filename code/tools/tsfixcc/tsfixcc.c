/*  
 * Copyright (C) 2008-2013  Lorenzo Pallara, l.pallara@avalpa.com
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
	int open_file;
	int byte_read;
	int fd_ts;			/* File descriptor of ts file */
	unsigned short pid;
	unsigned char packet[TS_PACKET_SIZE];
	unsigned char pid_cc_table[MAX_PID];	/* PID table for the continuity counter of the TS packets */	

	/* Open first ts file */
	if (argc >= 2) {
		open_file = 1;
		fd_ts = open(argv[open_file], O_RDONLY);
		if (fd_ts < 0) {
			fprintf(stderr, "Can't find file %s\n", argv[open_file]);
			return 2;
		}
	} else {
		fprintf(stderr, "Usage: 'tsfixcc filename.ts > filenamefixed.ts'\n");
		fprintf(stderr, "Tsfixcc increases countinuity counter of every packet following each other without adapatation field parsing'\n");
		fprintf(stderr, "Useful to link ts of sections in a single ts'\n");
		return 2;
	}
		
	/* Start to process the file */	
	memset(pid_cc_table, 0x10,  MAX_PID);	
	while(1) {

		/* read packets */
		byte_read = 0;
		byte_read = read(fd_ts, packet, TS_PACKET_SIZE);
		if (byte_read < TS_PACKET_SIZE) {
			return 0;
		}
	    
		/* stamp packet cc */
		memcpy(&pid, packet + 1, 2);	
		pid = ntohs(pid);
		pid = pid & 0x1fff;
		if (pid < MAX_PID) {
			if (pid_cc_table[pid] == 0x10) { 
				fprintf(stderr, "new pid entry %d\n", pid);
				pid_cc_table[pid] = 0;
			} else { 
				pid_cc_table[pid] = (pid_cc_table[pid] + 1) % 0x10; 
			}
			packet[3] = (pid_cc_table[pid] | (packet[3] & 0xf0));
		}
	
		/* write packet */
		write(STDOUT_FILENO, packet, TS_PACKET_SIZE);
	}
}

