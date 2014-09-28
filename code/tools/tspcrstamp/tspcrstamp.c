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

#define TS_HEADER_SIZE 4
#define TS_PACKET_SIZE 188
#define MAX_PID 8192
#define SYSTEM_CLOCK_FREQUENCY 27000000

void stamp_ts (unsigned long long int ts, unsigned char* buffer) 
{
	if (buffer) {
		buffer[0] = ((ts >> 29) & 0x0F) | 0x01;
		buffer[1] = (ts >> 22) & 0xFF; 
		buffer[2] = ((ts >> 14) & 0xFF ) | 0x01;
		buffer[3] = (ts >> 7) & 0xFF;
		buffer[4] = ((ts << 1) & 0xFF ) | 0x01;
	}
}

unsigned long long parse_timestamp(unsigned char *buf)
{
	unsigned long long a1;
	unsigned long long a2;
	unsigned long long a3;
	unsigned long long ts;

	a1 = (buf[0] & 0x0F) >> 1;
	a2 = ((buf[1] << 8) | buf[2]) >> 1;
	a3 = ((buf[3] << 8) | buf[4]) >> 1;
	ts = (a1 << 30) | (a2 << 15) | a3;
	
	return ts;
}


int main(int argc, char *argv[])
{
	int fd_ts;			/* File descriptor of ts file */
	u_short pid;
	int byte_read;
	int pcr_ext = 0;
	unsigned int i;
	/*
	unsigned int j;
	*/
	unsigned int buffer_size;
	unsigned char* packet_buffer;
	unsigned char* current_packet;
	unsigned long long int trbits = 0;
	unsigned long long int pcr_base = 0;					
	unsigned long long int ts_packet_count;
	unsigned long long int new_pcr = 0;	
	unsigned long long int new_pcr_module = 0;	
	unsigned long long int pcr_index_rounding = 0;	
	unsigned long long int pcr_table[MAX_PID];	/* PCR table for the outgoing TS packets */
	unsigned long long int pcr_index_table[MAX_PID];/* PCR index table for the TS packets */

	/* Open ts file */
	buffer_size = 0;
	if (argc >= 3)
		fd_ts = open(argv[1], O_RDONLY);
	else {
		fprintf(stderr, "Usage: tspcrstamp input.ts transport_rate_bit/s\n");
		fprintf(stderr, "N.B: this tool will change the pcr values to fix jitter after multiplexing\n");
		return 2;
	}
	if (fd_ts < 0) {
		fprintf(stderr, "Can't find file %s\n", argv[1]);
		return 2;
	}
        trbits = atol(argv[2]);
	if (trbits == 0) {
		fprintf(stderr, "transport_rate is 0?\n");
		return 2;
	}
	if (argv[3] != NULL) {
		buffer_size = atoi(argv[3]);
		if (buffer_size == 0) {
			fprintf(stderr, "buffer size is invalid\n");
			return 2;
		}
	} else {
		buffer_size = 1;
	}

	/* Start to process the file */
	buffer_size *= TS_PACKET_SIZE; 
	packet_buffer = malloc(buffer_size);
	if (packet_buffer == NULL) {
		fprintf(stderr, "out of memory\n");
		return 2;
	}
	memset(pcr_table, 0ll,  MAX_PID*(sizeof(unsigned long long int)));	
	memset(pcr_index_table, 0ll,  MAX_PID*(sizeof(unsigned long long int)));
	ts_packet_count = 0;
	//pcr_index_rounding = 2576980370000ll;
	pcr_index_rounding = 0ll;
	byte_read = 1;
	while(byte_read) {
	
		/* Read next packets */
		byte_read = read(fd_ts, packet_buffer, buffer_size);
	   
		for (i = 0; i < buffer_size; i += TS_PACKET_SIZE) {
			current_packet = packet_buffer + i;
			memcpy(&pid, current_packet + 1, 2);	
			pid = ntohs(pid);
			pid = pid & 0x1fff;
			
			/* Check pcr */
			if ((pid < MAX_PID) && (current_packet[3] & 0x20) && (current_packet[4] != 0) && (current_packet[5] & 0x10)) { /* there is PCR */ 
				new_pcr = (ts_packet_count * TS_PACKET_SIZE) + 10; /* ts position in byte */
				new_pcr *= 8; /* bits */
				new_pcr *= SYSTEM_CLOCK_FREQUENCY; 
				new_pcr /= trbits;
				new_pcr += pcr_index_rounding;
				new_pcr_module = new_pcr % 2576980377600ll;
				pcr_base = new_pcr_module / 300;
				pcr_ext = new_pcr_module % 300;
				current_packet[6] = (0xFF & (pcr_base >> 25));
				current_packet[7] = (0xFF & (pcr_base >> 17));
				current_packet[8] = (0xFF & (pcr_base >> 9));
				current_packet[9] = (0xFF & (pcr_base >> 1));
				current_packet[10] = ((0x1 & pcr_base) << 7) | 0x7E | ((0x100 & pcr_ext) >> 8);
				current_packet[11] = (0xFF & pcr_ext);
				
				/* max packet count = 454263790 */
				/* multiplication will overflown but sum won't because is after div */
				/* ((454263790 * 188) + 10) * 8 * 27000000 < 2^64 */
				if (ts_packet_count * 2 >= 454263790) {
				    ts_packet_count = 0;
				    pcr_index_rounding = new_pcr;
				}
				
				/* 2^33 * 300 = 2576980377600 / 27000000 < 95444 secs at 100Mbps are < 6346010639 packets < 2^64 ts_packet_counter */	
				if (new_pcr >= 2576980377600ll ) { 
				    ts_packet_count = 0;
				    pcr_index_rounding = (new_pcr - 2576980377600ll);
				}
				
			}

			/*
			if (((ts_packet_count + 1) * TS_PACKET_SIZE) + 10 < ts_packet_count) { 
				unsigned long long int min = 0ll;
				for (j = 0; j < MAX_PID; j++) {
				    if (pcr_index_table[pid] > 0 && min == 0) {
					min = pcr_index_table[pid];
				    } else if (pcr_index_table[pid] > 0 && pcr_index_table[pid] < min) {
					min = pcr_index_table[pid];					
				    }
				}
				unsigned long long int ts_packet_delta = (min - 10) / TS_PACKET_SIZE;
				ts_packet_count -= ts_packet_delta;
				for (j = 0; j < MAX_PID; j++) {
				    pcr_index_table[pid] -= ts_packet_delta;
				    pcr_table[pid] = pcr_index_table[pid] * 8 * SYSTEM_CLOCK_FREQUENCY / trbits;
				}
			} 
			*/
			
			ts_packet_count++;
			
		}
		
		
		/* write packets out */
		write(STDOUT_FILENO, packet_buffer, buffer_size);
		
	}
	return 0;
}
