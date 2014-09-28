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
 * You should have received a copy of the General Public License
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
	unsigned char choice;
	int port;
	unsigned char packet[TS_PACKET_SIZE];

	if (argc >= 5) {
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
		port = atoi(argv[4]);
		if (port < 0) {
			fprintf(stderr, "Can't open port %s\n", argv[4]);
			return 0;
		}

	} else {
		fprintf(stderr, "Usage: 'tsoutputswitch inputfile.ts outputfile1.ts outputfile2.ts port'\n");
		fprintf(stderr, "0 output on outputfile1 \n");
		fprintf(stderr, "1 output on outputfile2 \n");
		fprintf(stderr, "2 output on both \n");
		fprintf(stderr, "N.B. if they are not fifo the file size will increase\n");
		fprintf(stderr, "Default is 0\n");
		return 0;
	}


	int listen_sd;
	listen_sd = socket(AF_INET, SOCK_DGRAM, 0);
	if(listen_sd < 0) {
		fprintf(stderr, "secsocket - socket() error\n");
		exit(-1);
	} else {
		fprintf(stderr, "secsocket - socket() is OK\n");
	}
	
	int flags;
	flags = fcntl(listen_sd,F_GETFL,0);
	fcntl(listen_sd, F_SETFL, flags | O_NONBLOCK);

	int reuse = 1;
	if (setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) {
		fprintf(stderr, "setting SO_REUSEADDR failed\n");
		close(listen_sd);
		exit(-1); 
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY; 
	addr.sin_port = htons(port);
	int result = bind(listen_sd, (struct sockaddr *)&addr, sizeof(addr));
	if(result < 0) {
		fprintf(stderr, "secsocket - bind() error\n");
		close(listen_sd);
		exit(-1);
	} else {
	//	fprintf(stderr, "secsocket - bind() is OK\n");
	}
	
	byte_read = 1;
	choice = 0;
	int new_choice = 0;
	unsigned char received = 0;
	while(byte_read) {
		if (recv(listen_sd, &received, 1, 0) > 0) {
		    fprintf (stderr, "received %d\n", received - 48);
		    new_choice = received - 48;
		}
		byte_read = read(input_file, packet, TS_PACKET_SIZE);
		if (byte_read > 0) {
		    if (new_choice != choice) {
			close(output_file1);
			output_file1 = open(argv[2], O_WRONLY);
			if (output_file1 < 0) {
				fprintf(stderr, "Can't open file %s\n", argv[2]);
				return 0;
			}
			close(output_file2);
			output_file2 = open(argv[3], O_WRONLY);
			if (output_file2 < 0) {
				fprintf(stderr, "Can't open file %s\n", argv[3]);
				return 0;
			}
			choice = new_choice;
		    }
		    if (choice == 0 || choice == 2) {
			write(output_file1, packet, TS_PACKET_SIZE);
		    } 
		    if (choice == 1 || choice == 2) {
			write(output_file2, packet, TS_PACKET_SIZE);
		    }
		}
	}
	
	close(input_file);
	close(output_file1);
	close(output_file2);
	close(listen_sd);
	
	return 0;
	
}
