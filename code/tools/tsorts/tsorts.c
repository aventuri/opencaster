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
	int byte_read;
	int index;

	unsigned char ts_packet[TS_PACKET_SIZE];
	if (argc >= 3) {
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
	} else {
		fprintf(stderr, "Usage: 'tsorts inputfile1.ts inputfile2.ts > output.ts'\n");
		fprintf(stderr, "ts packets are read from inputfile2.ts if they are not immediatly available from inputfile1.ts'\n");
		return 0;
	}

	fcntl (input1_file, F_SETFL, fcntl (input1_file, F_GETFL, 0) | O_NONBLOCK);
	index = 0;
	while (1) {
		byte_read = read(input1_file, ts_packet + index, TS_PACKET_SIZE - index);
		if (byte_read > 0) {
			index += byte_read;
			if (index == TS_PACKET_SIZE) {
			    write(STDOUT_FILENO, ts_packet, TS_PACKET_SIZE);
			    index = 0;
			}
		} else if (byte_read <= 0 && index == 0) {
			byte_read = read(input2_file, ts_packet, TS_PACKET_SIZE);
			if (byte_read == 0) {
				close(input2_file);
				input2_file = open(argv[2], O_RDONLY);
				if (input2_file < 0) {
					fprintf(stderr, "Can't open file %s\n", argv[2]);
					return 0;
				} else {
					byte_read = read(input2_file, ts_packet, TS_PACKET_SIZE);
					if (byte_read > 0) {
						write(STDOUT_FILENO, ts_packet, TS_PACKET_SIZE);
					}
				}
			} else {
				write(STDOUT_FILENO, ts_packet, TS_PACKET_SIZE);
			}
		}
	}
	
	return 0;
	
}
