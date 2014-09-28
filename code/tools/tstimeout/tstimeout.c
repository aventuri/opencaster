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
	int result;
	int stable;
	int backup_file;
	int byte_read;
	int timeout;
	int index;
	fd_set set;
	struct timeval tv;

	unsigned char ts_packet[TS_PACKET_SIZE];
	if (argc >= 4) {
		input_file = open(argv[1], O_RDONLY);
		if (input_file < 0) {
			fprintf(stderr, "Can't open file %s\n", argv[1]);
			return 0;
		}
		backup_file = open(argv[2], O_RDONLY);
		if (backup_file < 0) {
			fprintf(stderr, "Can't open file %s\n", argv[2]);
			return 0;
		}
		timeout = atoi(argv[3]);
		if (timeout == 0) {
			fprintf(stderr, "Timeout has to be bigger than 0\n");
			return 0;
		}
	} else {
		fprintf(stderr, "Usage: 'tstimeout inputfile.ts backupfile.ts timeout_in_ms'\n");
		return 0;
	}

	fcntl (input_file, F_SETFL, fcntl (input_file, F_GETFL, 0) | O_NONBLOCK);
	index = 0;
	tv.tv_sec = timeout/1000;
	tv.tv_usec = (timeout%1000) * 1000;
	stable = 1;
	while (1) {
	    FD_ZERO(&set);
	    FD_SET(input_file,&set);
	    if (stable) {
		tv.tv_sec = timeout/1000;
		tv.tv_usec = (timeout%1000) * 1000;
	    } else { 
		tv.tv_sec = 0LL;
		tv.tv_usec = 0LL;
	    }
	    result = select(input_file+ 1,  &set, NULL, NULL, &tv);
	    if (result > 0) {
		if (FD_ISSET(input_file, &set)) {
		    byte_read = read(input_file, ts_packet + index, TS_PACKET_SIZE - index);
		    if (byte_read > 0) {
			stable = 1;
			index += byte_read;
			if (index == TS_PACKET_SIZE) {
			    write(STDOUT_FILENO, ts_packet, TS_PACKET_SIZE);
			    index = 0;
			}
		    }
		}
	    } else {
		stable = 0;
		read(backup_file, ts_packet, TS_PACKET_SIZE);
		write(STDOUT_FILENO, ts_packet, TS_PACKET_SIZE);
	    }	
	}
	
	close(input_file);
	
	return 0;
	
}
