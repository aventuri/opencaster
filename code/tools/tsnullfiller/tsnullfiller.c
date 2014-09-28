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
#define SYSTEM_CLOCK_FREQUENCY 27000000

int main(int argc, char *argv[])
{
	int fd_ts;
	int byte_read;
	unsigned int i;
	unsigned int input_buffer_size;
	unsigned int output_buffer_size;
	unsigned long long int ibits;
	unsigned long long int obits;	
	unsigned long long int icounter;
	unsigned long long int ocounter;
	unsigned long long int last_ocounter;
	unsigned char* input_packet_buffer;
	unsigned char* output_packet_buffer;
	unsigned char* current_input_packet;
	unsigned char* current_output_packet;
	unsigned char null_ts_packet[TS_PACKET_SIZE];

	/* Parse args */
	input_buffer_size = 0;
	if (argc >= 4)
	    fd_ts = open(argv[1], O_RDONLY);
	else {
	    fprintf(stderr, "Usage: 'tsnullfiller input.ts output_bit/s input_bit/s [packet_buffer]'\n");
	    fprintf(stderr, "adds empty packets to input.ts to reach output_bit/s\n");
	    return 2;
	}
	if (fd_ts < 0) {
	    fprintf(stderr, "Can't find file %s\n", argv[1]);
	    return 2;
	}
        obits = atol(argv[2]);
        if (obits == 0) {
            fprintf(stderr, "0 output_bit/s?\n");
            return 2;  
        }	
	ibits = atol(argv[3]);
	if (ibits == 0) {
	    fprintf(stderr, "0 input_bit/s?\n");
	    return 2;	
	}
	if (obits <= ibits) {
	    fprintf(stderr, "output rate has to be bigger\n");
	    return 2;
	}
	if (argv[4] != NULL) {	
		input_buffer_size = atoi(argv[4]);
		if (input_buffer_size == 0) {
			fprintf(stderr, "buffer size is not valid\n");
			return 2;
		}
	} else { 
		input_buffer_size = 1;
	}
	
	output_buffer_size = (input_buffer_size * obits / ibits) + 1; /* be sure to have space for null packets */
	input_buffer_size *= TS_PACKET_SIZE;
	output_buffer_size *= TS_PACKET_SIZE;
	input_packet_buffer = malloc(input_buffer_size);
	output_packet_buffer = malloc(output_buffer_size);
	if (input_packet_buffer == NULL || output_packet_buffer == NULL) {
		fprintf(stderr, "Out of memory\n");
		return 2;
	}
	
	/* Init null packet */
	memset(null_ts_packet, 0, TS_PACKET_SIZE);
	null_ts_packet[0] = 0x47;
	null_ts_packet[1] = 0x1F;
	null_ts_packet[2] = 0xFF;
	null_ts_packet[3] = 0x10;

	/* Start to process the file */
	icounter = 0;
	ocounter = 0;	
	byte_read = 1;	
	while(byte_read) {
		
		/* Read next packets */
		byte_read = read(fd_ts, input_packet_buffer, input_buffer_size);

		/* Generate output buffer */
		current_input_packet = input_packet_buffer;
		current_output_packet = output_packet_buffer;
		for (i = 0; i < input_buffer_size; i += TS_PACKET_SIZE) {
			current_input_packet = input_packet_buffer + i;
			
			/* copy ts packet */
			memcpy(current_output_packet, current_input_packet, TS_PACKET_SIZE);
			current_output_packet += TS_PACKET_SIZE; 
			
			/* increase bits counters */
			icounter += TS_PACKET_SIZE * 8;
			ocounter += TS_PACKET_SIZE * 8;
				
			/* generate null packets if there's time */
			while (((ocounter + TS_PACKET_SIZE * 8) * ibits) <= (icounter * obits)) { 
				/* copy null packet */	
				memcpy(current_output_packet, null_ts_packet, TS_PACKET_SIZE);
				current_output_packet += TS_PACKET_SIZE;
				
				/* increase bit counter */
				last_ocounter = ocounter;
				ocounter += TS_PACKET_SIZE * 8;
				if (last_ocounter > ocounter) { /* manage zeroed counter */
					last_ocounter = last_ocounter - icounter;
					icounter = 0;
					ocounter = last_ocounter + TS_PACKET_SIZE * 8;
				}
			}
		}

		/* Write output packets */
		write(STDOUT_FILENO, output_packet_buffer, current_output_packet-output_packet_buffer);

	}
	return 0;
}
