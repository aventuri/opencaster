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
	int input_file;
	int output_file1;
	int output_file2;
	int byte_read;
	unsigned char packet[TS_PACKET_SIZE];
	
	if (argc >= 4) {
		input_file = open(argv[1], O_RDONLY);
		if (input_file < 0) {
			fprintf(stderr, "Can't open file %s\n", argv[1]);
			return 0;
		}
		output_file1 = open(argv[2], O_WRONLY);
		if (output_file1 < 0) {
			fprintf(stderr, "Can't open file %s\n", argv[2]);
			return 0;
		}
		output_file2 = open(argv[3], O_WRONLY);
		if (output_file2 < 0) {
			fprintf(stderr, "Can't open file %s\n", argv[3]);
			return 0;
		}

	} else {
		fprintf(stderr, "Usage: 'tsdoubleoutput inputfile.ts outputfile1.ts outputfile2.ts'\n");
		fprintf(stderr, "N.B. files must exist, touch them if needed'\n");
		return 0;
	}


	byte_read = 1;
	while(byte_read) {
		byte_read = read(input_file, packet, TS_PACKET_SIZE);
		write(output_file1, packet, TS_PACKET_SIZE);
		write(output_file2, packet, TS_PACKET_SIZE);
	}
	
	close(input_file);
	close(output_file1);
	close(output_file2);
	
	return 0;
	
}
