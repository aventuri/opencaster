/*
 * Copyright (C) 2011-2013  Lorenzo Pallara, l.pallara@avalpa.com
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
#define TIME_STAMP_SIZE 5
#define SYSTEM_CLOCK_FREQUENCY 27000000
const long long unsigned system_frequency = 27000000;

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
	int ts_header_size = 0;
	unsigned char PTS_DTS_flag = 0;
	unsigned char pes_header_size = 0;
	unsigned int buffer_size = 0;
	unsigned char adapt = 0; 
	unsigned char* packet_buffer = 0;
	unsigned char* current_packet = 0;
	unsigned char timestamp[TIME_STAMP_SIZE];
	unsigned long long int time = 0;
	unsigned long long int trbits = 0;
	unsigned long long int pcr_base = 0;
	unsigned long long int ts_packet_count;
	unsigned long long int new_pcr = 0;
	unsigned long long int new_pcr_module = 0;
	unsigned long long int pcr_index_rounding = 0;
	unsigned long long int pcr_table[MAX_PID]; /* PCR table for the outgoing TS packets */
	unsigned long long int pcr_index_table[MAX_PID];/* PCR index table for the TS packets */
	unsigned long long int pts_time_table[MAX_PID];
	unsigned short pts_pcr_reference[MAX_PID];

	/* Open ts file */
	buffer_size = 0;
	if (argc >= 3)
		fd_ts = open(argv[1], O_RDONLY);
	else {
		fprintf(stderr, "Usage: tspcrrestamp input.ts transport_rate_bit/s pes_pid1 pcr_pid1 pes_pid2 pcr_pid2 [...] pes_pidn pcr_pidn \n");
		fprintf(stderr, "N.B: this tool will change the pcr values taking as reference the first pcr seen");
		fprintf(stderr, "If pts/dts time in a given pes_pid goes backward the pcr in the reference pid is reset too");
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
	memset(pts_time_table, 0ll,  MAX_PID*(sizeof(unsigned long long int)));
	
	for (i = 0; i < MAX_PID; i++) {
		pts_pcr_reference[i] = MAX_PID;
	}
	
	for (i = 3; i < argc; i+=2) {
		pid = atoi(argv[i]);
		if (pid < MAX_PID && pid >= 0) {
			pts_pcr_reference[pid] = atoi(argv[i+1]);
		}
		fprintf(stderr,"pts of pid %d will reset pcr of pid %d\n", pid, pts_pcr_reference[pid]);
	}
	
	ts_packet_count = 0;
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
				if (pcr_table[pid] == 0) { 
					pcr_base = (((unsigned long long int)current_packet[6]) << 25) + (current_packet[7] << 17) + (current_packet[8] << 9) + (current_packet[9] << 1) + (current_packet[10] >> 7);
					pcr_ext = ((current_packet[10] & 1) << 8) + current_packet[11];
					pcr_table[pid] = (pcr_base * 300 + pcr_ext);
					pcr_index_table[pid] = ts_packet_count;
					//fprintf(stderr, "%llu: pid %d, new pcr is %llu (%f sec)\n",
					//	pcr_index_table[pid],
					fprintf(stderr, "pid %d start pcr is %llu (%f sec)\n",
						pid,
						pcr_table[pid],
						((double)(pcr_table[pid]) / SYSTEM_CLOCK_FREQUENCY)
					);
				} else {
					new_pcr = ((ts_packet_count - pcr_index_table[pid]) * TS_PACKET_SIZE) + 10; /* ts position in byte */
					new_pcr *= 8; /* bits */
					new_pcr *= SYSTEM_CLOCK_FREQUENCY; 
					new_pcr /= trbits;
					new_pcr += pcr_table[pid];
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
			
			/* check PTS and DTS */
			if (pid < MAX_PID) {
				
				if (pts_pcr_reference[pid] != MAX_PID) {
					adapt = (current_packet[3] >> 4) & 0x03;
					if (adapt == 0) {
						ts_header_size = TS_PACKET_SIZE; /* the packet is invalid ?*/
					} else if (adapt == 1) {
						ts_header_size = TS_HEADER_SIZE; /* only payload */
					} else if (adapt == 2) { 
						ts_header_size = TS_PACKET_SIZE; /* only adaptation field */
					} else if (adapt == 3) {
						ts_header_size = TS_HEADER_SIZE + current_packet[4] + 1; /* jump the adaptation field */
					} else {
						ts_header_size = TS_PACKET_SIZE; /* not managed */
					}
				} else {
					ts_header_size = TS_PACKET_SIZE; /* not interested into monitor this pid */
				}
				
				pes_header_size = 0;
				time = 0;
				if (ts_header_size + 20 < TS_PACKET_SIZE && pid < MAX_PID) {
					if (
					(current_packet[ts_header_size] == 0x00) && 
					(current_packet[ts_header_size + 1] == 0x00) && 
					(current_packet[ts_header_size + 2] == 0x01) && 
						(
							((current_packet[ts_header_size + 3] >> 4) == 0x0E) ||
							((current_packet[ts_header_size + 3] >> 5) == 0x05) || 
							((current_packet[ts_header_size + 3] >> 5) == 0x06)
						)
					){  /* pes packet prefix code for PES video and audio */
						PTS_DTS_flag = current_packet[ts_header_size + 7] >> 6;
						pes_header_size = current_packet[ts_header_size + 8];
						if (PTS_DTS_flag > 1 && pes_header_size) {
							memcpy(timestamp, current_packet + ts_header_size + 9, TIME_STAMP_SIZE);
							time = parse_timestamp(timestamp);
							//fprintf(stderr, "Pid %d Presentation Time Stamp is: %llu, %llu.%04llu sec.\n", pid, time,  time / (system_frequency / 300), (time % (system_frequency / 300)) / ((system_frequency / 300) / 10000));
							if (pes_header_size > 5 && PTS_DTS_flag > 2) {
								memcpy(timestamp, current_packet + ts_header_size + 14, TIME_STAMP_SIZE);
								time = parse_timestamp(timestamp);
								//fprintf(stderr, "Pid %d Decode Time Stamp is: %llu, %llu.%04llu sec.\n", pid, time,  time / (system_frequency / 300), (time % (system_frequency / 300)) / ((system_frequency / 300) / 10000)); 
							}
							if (time < pts_time_table[pid]) { /* pts loop or encoder crashed and time restarted */
								pcr_table[pid] = 0; /* reset pcr table */
							}
							pts_time_table[pid] = time;
						}
					}
				}
			}
			
			ts_packet_count++;
		}
		
		/* write packets out */
		write(STDOUT_FILENO, packet_buffer, buffer_size);
		
	}
	return 0;
}
