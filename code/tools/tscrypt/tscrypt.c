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

#include <stdio.h>

#include <netinet/in.h>

#include <fcntl.h>
#include <unistd.h> 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <inttypes.h>

#include <dvbcsa/dvbcsa.h>
//#include "dvbcsa_pv.h"


#define CW_SIZE 16 /* 8 bytes odd and 8 bytes even */
#define TS_PACKET_SIZE 188
#define TS_PAYLOAD_SIZE 184

unsigned char cw[CW_SIZE];

void read_cw(char* filename) {

	int fd_cw;			/* File descriptor of a control word file */

	/* Check for new cws */
	fd_cw = open(filename, O_RDONLY);
	if (fd_cw < 0) {
		fprintf(stderr, "Can't find file %s\n", filename);
		exit(2);
	}
	int byte_read = read(fd_cw, cw, CW_SIZE);
	close(fd_cw);
	if (byte_read <= 0) {
		fprintf(stderr, "Can't read file %s\n", filename);
		exit(2);
	} 
}

void Usage() {
	fprintf(stderr, "Usage: tscrypt input.ts file.cw > crypted.ts \n");
	fprintf(stderr, "file.cw is 16 byte, 8 bytes of even word and 8 bytes of odd word \n");
	exit(2);
}

int main(int argc, char *argv[])
{
	int fd_ts;
	int byte_read;
	int adaptation_field = 0;
	unsigned char packet[TS_PACKET_SIZE];
	struct dvbcsa_key_s	*key = dvbcsa_key_alloc();
	
	if (argc >= 3) {
		fd_ts = open(argv[1], O_RDONLY);
		read_cw(argv[2]);
	} else {
		Usage();
	}
	if (fd_ts < 0) {
		fprintf(stderr, "Can't find file %s\n", argv[2]);
		return 2;
	}
	
	
	dvbcsa_key_set(cw, key);
	
	byte_read = 1;
	while(byte_read) {
	
		byte_read = read(fd_ts, packet, TS_PACKET_SIZE);
		
		adaptation_field = (packet[3] & 0x30) >> 4;
		if (adaptation_field == 1) { // payload only, in OpenCaster pcr is without payload
			dvbcsa_encrypt(key, packet + 4, TS_PAYLOAD_SIZE);
			packet[3] |= 0x80; // set encryption into header for odd cw
		}
		write(STDOUT_FILENO, packet, TS_PACKET_SIZE);
	
	}
	
	dvbcsa_key_free(key);
	
	return 0;
}

