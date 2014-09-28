/*  
 * Copyright (C) 2009-2013  Lorenzo Pallara, l.pallara@avalpa.com
 
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

#define MULTICAST

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/reboot.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>


#define TS_PACKET_SIZE 188

int main(int argc, char *argv[])
{
	int input1_file;
	int input2_file;
	int input3_file;
	int byte_read1;
	int byte_read2;
	int byte_read3;
	int index1;
	int index2;

	unsigned char ts_packet1[TS_PACKET_SIZE];
	unsigned char ts_packet2[TS_PACKET_SIZE];
	unsigned char ts_packet3[TS_PACKET_SIZE];
	
	if (argc >= 4) {
		input1_file = open(argv[1], O_RDONLY);
		if (input1_file < 0) {
			fprintf(stderr, "Can't open file %s\n", argv[1]);
			return 0;
		}
		input2_file = open(argv[2], O_RDONLY);
		if (input2_file < 0) {
			fprintf(stderr, "Can't open file %s\n", argv[2]);
			return 0;
		}
		input3_file = open(argv[3], O_RDONLY);
		if (input3_file < 0) {
			fprintf(stderr, "Can't open file %s\n", argv[3]);
			return 0;
		}
	} else {
		fprintf(stderr, "Usage: 'tsororts inputfile1.ts inputfile2.ts inputfile3.ts > output.ts'\n");
		fprintf(stderr, "ts packets are read from inputfile3.ts if they are not immediatly available from inputfile1.ts or inputfile2.ts'\n");
		return 0;
	}

	fcntl (input1_file, F_SETFL, fcntl (input1_file, F_GETFL, 0) | O_NONBLOCK);
	fcntl (input2_file, F_SETFL, fcntl (input2_file, F_GETFL, 0) | O_NONBLOCK);
	index1 = 0;
	index2 = 0;
	while (1) {
	
		byte_read1 = read(input1_file, ts_packet1 + index1, TS_PACKET_SIZE - index1);
		
		byte_read2 = read(input2_file, ts_packet2 + index2, TS_PACKET_SIZE - index2);
		
		if (byte_read1 > 0) {
			index1 += byte_read1;
			if (index1 == TS_PACKET_SIZE) {
			    write(STDOUT_FILENO, ts_packet1, TS_PACKET_SIZE);
			    index1 = 0;
			}
		}
		
		if (byte_read2 > 0) {
			index2 += byte_read2;
			if (index2 == TS_PACKET_SIZE) {
			    write(STDOUT_FILENO, ts_packet2, TS_PACKET_SIZE);
			    index2 = 0;
			}
		}
		
		if (byte_read1 <= 0 && byte_read2 <= 0 && index1 == 0 && index2 == 0) {
			byte_read3 = read(input3_file, ts_packet3, TS_PACKET_SIZE);
			if (byte_read3 == 0) {
				close(input3_file);
				input3_file = open(argv[3], O_RDONLY);
				if (input3_file < 0) {
					fprintf(stderr, "Can't open file %s\n", argv[2]);
					return 0;
				} else {
					byte_read3 = read(input3_file, ts_packet3, TS_PACKET_SIZE);
					if (byte_read3 > 0) {
						write(STDOUT_FILENO, ts_packet3, TS_PACKET_SIZE);
					}
				}
			} else {
				write(STDOUT_FILENO, ts_packet3, TS_PACKET_SIZE);
			}
		}
		
	}
	
	return 0;
	
}
