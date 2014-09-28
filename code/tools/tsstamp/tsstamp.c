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

#define TS_HEADER_SIZE 4
#define TS_PACKET_SIZE 188
#define MAX_PID 8192
#define SYSTEM_CLOCK_FREQUENCY 27000000
#define PACK_HEADER_SIZE 4 
#define TIME_STAMP_SIZE 5
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
	int pusi = 0;
	int pcr_ext = 0;
	int ts_header_size = 0;
	unsigned int i;
	unsigned int buffer_size;
	unsigned char adapt = 0;
	unsigned char timestamp[TIME_STAMP_SIZE];
	unsigned char pes_header_size = 0;
	unsigned char* packet_buffer;
	unsigned char* current_packet;
	unsigned long long time = 0;
	unsigned long long int trbits = 0;
	unsigned long long int pcr_base = 0;					
	unsigned long long int ts_packet_count;
	unsigned long long int new_pcr = 0;	
	unsigned long long int new_pcr_index = 0;	
	unsigned long long int pcr_table[MAX_PID];	/* PCR table for the outgoing TS packets */
	unsigned long long int pcr_index_table[MAX_PID];/* PCR index table for the TS packets */
	unsigned long long int pts_table[MAX_PID]; /* PTS last value */
	unsigned long long int pts_delta_table[MAX_PID]; /* PTS delta increment */
	unsigned char pts_index_table[MAX_PID];/* PTS index table for the TS packets */

	/* Open ts file */
	buffer_size = 0;
	if (argc >= 3)
		fd_ts = open(argv[1], O_RDONLY);
	else {
		fprintf(stderr, "Usage: tsstamp input.ts transport_rate_bit/s\n");
		fprintf(stderr, "N.B: this tool will change the pcr/pts/dts values to fix loop conditions and jitter after multiplexing\n");
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
	memset(pcr_table, 0,  MAX_PID*(sizeof(long long int)));	
	memset(pcr_index_table, 0,  MAX_PID*(sizeof(long long int)));
	memset(pts_delta_table, 0,  MAX_PID*(sizeof(long long int)));
	memset(pts_table, 0,  MAX_PID*(sizeof(long long int)));
	memset(pts_index_table, 0,  MAX_PID*(sizeof(unsigned char)));
	ts_packet_count = 0;
	byte_read = 1;
	while(byte_read) {
	
		/* Read next packets */
		byte_read = read(fd_ts, packet_buffer, buffer_size);
	   
		for (i = 0; i < buffer_size; i += TS_PACKET_SIZE) {
			current_packet = packet_buffer + i;
			memcpy(&pid, current_packet + 1, 2);	
			pid = ntohs(pid);
			pid = pid & 0x1fff;
			
			pusi = current_packet[1] & 0x40;
			
			/* Check pcr */
			if ((pid < MAX_PID) && (current_packet[3] & 0x20) && (current_packet[4] != 0) && (current_packet[5] & 0x10) && pusi) { /* there is PCR */ 
			
				new_pcr_index =  (ts_packet_count * TS_PACKET_SIZE) + 10;
				if (pcr_index_table[pid] != 0) { 
			
					new_pcr = new_pcr_index - pcr_index_table[pid];
					new_pcr *= 8;
					new_pcr *= SYSTEM_CLOCK_FREQUENCY; 
					new_pcr /= trbits;
					new_pcr += pcr_table[pid];
/*
					fprintf(stderr, "pid %d: stamping pcr %llu, pcr_delta is %llu\n", pid, new_pcr, new_pcr - pcr_table[pid]); 
*/
/*					
					pcr_base = ((ts_packet[6] << 25) | (ts_packet[7] << 17) | (ts_packet[8] << 9) | (ts_packet[9] << 1)) + (ts_packet[10] >> 7);
					pcr_ext = ((ts_packet[10] & 1) << 8) | ts_packet[11];			
*/			

/*										
					fprintf(stderr, "%llu: pid %d, new pcr is %llu, pcr delta is %llu, (%f ms), indices delta is %llu bytes,( %f ms), pcr accuracy is %.10f, instant ts bit rate is %.10f\n", 
						new_pcr_index,
						pid, 
						new_pcr,
						new_pcr - pcr_table[pid],
						((double)((new_pcr - pcr_table[pid]) * 1000)) / SYSTEM_CLOCK_FREQUENCY,
						new_pcr_index - pcr_index_table[pid],
						((double)((new_pcr_index - pcr_index_table[pid]) * 8 * 1000)) / trbits,
						(((double) (new_pcr - pcr_table[pid])) / SYSTEM_CLOCK_FREQUENCY) - (((double)(new_pcr_index - pcr_index_table[pid])) * 8 / trbits),
						(((double)(new_pcr_index - pcr_index_table[pid])) * 8 * SYSTEM_CLOCK_FREQUENCY) /  ((double)(new_pcr - pcr_table[pid])) 
					);
*/					
					pcr_base = new_pcr / 300;
					pcr_ext = new_pcr % 300;
					current_packet[6] = (0xFF & (pcr_base >> 25));
					current_packet[7] = (0xFF & (pcr_base >> 17));
					current_packet[8] = (0xFF & (pcr_base >> 9));
					current_packet[9] = (0xFF & (pcr_base >> 1));
					current_packet[10] = ((0x1 & pcr_base) << 7) | 0x7E | ((0x100 & pcr_ext) >> 8);
					current_packet[11] = (0xFF & pcr_ext);				
					pcr_table[pid] = new_pcr;
				}
				pcr_index_table[pid] = new_pcr_index;				
			}

			/* check PTS and DTS */
			adapt = (current_packet[3] >> 4) & 0x03;
			if (adapt == 0) {
				ts_header_size = TS_PACKET_SIZE; /* the packet is invalid ?*/
				;
			} else if (adapt == 1) {
				ts_header_size = TS_HEADER_SIZE; /* only payload */
			} else if (adapt == 2) { 
				ts_header_size = TS_PACKET_SIZE; /* only adaptation field */
			} else if (adapt == 3) {
				ts_header_size = TS_HEADER_SIZE + current_packet[4] + 1; /* jump the adaptation field */
			} else {
				ts_header_size = TS_PACKET_SIZE; /* not managed */
			}
			
			/* check the time difference between first two pts and ... */
			pes_header_size = 0;
			time = 0;
			if (ts_header_size + 20 < TS_PACKET_SIZE && pid < MAX_PID && pusi) {
				if ((current_packet[ts_header_size] == 0x00) && (current_packet[ts_header_size + 1] == 0x00) && (current_packet[ts_header_size + 2] == 0x01)) { 
					pes_header_size = current_packet[ts_header_size + 8];
					if ((current_packet[ts_header_size + 3] >> 4) == 0x0E) { /* PES video stream */
					
						memcpy(timestamp, current_packet + ts_header_size + 9, TIME_STAMP_SIZE);
						time = parse_timestamp(timestamp);						
						// fprintf(stderr, "Pid %d old Video Presentation Time Stamp is: %llu, %llu.%04llu sec.\n", pid, time,  time / (system_frequency / 300), (time % (system_frequency / 300)) / ((system_frequency / 300) / 10000));
						if (pes_header_size > 5) {
							memcpy(timestamp, current_packet + ts_header_size + 14, TIME_STAMP_SIZE);
							time = parse_timestamp(timestamp);
							// fprintf(stderr, "Pid %d old Video Decode Time Stamp is: %llu, %llu.%04llu sec.\n", pid, time,  time / (system_frequency / 300), (time % (system_frequency / 300)) / ((system_frequency / 300) / 10000)); 
						}					
					
						if (pts_index_table[pid] == 0) {
							if (pes_header_size > 5) { /* if there are both dts and pts, get dts */
								memcpy(timestamp, current_packet + ts_header_size + 14, TIME_STAMP_SIZE);
								time = parse_timestamp(timestamp);
							} else { /* othewise they are the same */
								memcpy(timestamp, current_packet + ts_header_size + 9, TIME_STAMP_SIZE);
								time = parse_timestamp(timestamp);
							}
							pts_index_table[pid] = 1;
							pts_table[pid] = time;
						} else if (pts_index_table[pid] == 1) {
							if (pes_header_size > 5) { /* if there are both dts and pts, get dts */
								memcpy(timestamp, current_packet + ts_header_size + 14, TIME_STAMP_SIZE);
								time = parse_timestamp(timestamp);
							} else { /* othewise they are the same */
								memcpy(timestamp, current_packet + ts_header_size + 9, TIME_STAMP_SIZE);
								time = parse_timestamp(timestamp);
							}
							pts_index_table[pid] = 2;
							pts_delta_table[pid] = time - pts_table[pid];
							pts_table[pid] = time;
						} else {
							if (pes_header_size > 5) { /* if there are both dts and pts */
								memcpy(timestamp, current_packet + ts_header_size + 9, TIME_STAMP_SIZE);
								time = parse_timestamp(timestamp);
								memcpy(timestamp, current_packet + ts_header_size + 14, TIME_STAMP_SIZE);
								time -= parse_timestamp(timestamp);
								pts_table[pid] += pts_delta_table[pid]; /* dts goes up 1 step */
								stamp_ts(pts_table[pid], current_packet + ts_header_size + 14);
								current_packet[ts_header_size + 14] &= 0x0F; 
								current_packet[ts_header_size + 14] |= 0x10; 
								/* pts goes up the same gap there was before */
								stamp_ts(pts_table[pid] + time, current_packet + ts_header_size + 9);	
								current_packet[ts_header_size + 9] &= 0x0F;
								current_packet[ts_header_size + 9] |= 0x30;
							} else {
								pts_table[pid] += pts_delta_table[pid];
								stamp_ts(pts_table[pid], current_packet + ts_header_size + 9);
								current_packet[ts_header_size + 9] &= 0x0F; 
								current_packet[ts_header_size + 9] |= 0x20;
							}
						}
					
						memcpy(timestamp, current_packet + ts_header_size + 9, TIME_STAMP_SIZE);
						time = parse_timestamp(timestamp);						
						// fprintf(stderr, "Pid %d new Video Presentation Time Stamp is: %llu, %llu.%04llu sec.\n", pid, time,  time / (system_frequency / 300), (time % (system_frequency / 300)) / ((system_frequency / 300) / 10000));
						if (pes_header_size > 5) {
							memcpy(timestamp, current_packet + ts_header_size + 14, TIME_STAMP_SIZE);
							time = parse_timestamp(timestamp);
							// fprintf(stderr, "Pid %d new Video Decode Time Stamp is: %llu, %llu.%04llu sec.\n", pid, time,  time / (system_frequency / 300), (time % (system_frequency / 300)) / ((system_frequency / 300) / 10000)); 
						}
					
					} else if (((current_packet[ts_header_size + 3] >> 5) == 0x05) || ((current_packet[ts_header_size + 3] >> 5) == 0x06) ) { /* PES audio stream */
					
						memcpy(timestamp, current_packet + ts_header_size + 9, TIME_STAMP_SIZE);
						time = parse_timestamp(timestamp);
						// fprintf(stderr, "Pid %d old Audio Presentation Time Stamp is: %llu, %llu.%04llu sec.\n", pid, time,  time / (system_frequency / 300), (time % (system_frequency / 300)) / ((system_frequency / 300) / 10000));
						
						if (pts_index_table[pid] == 0) {
							pts_index_table[pid] = 1;
							memcpy(timestamp, current_packet + ts_header_size + 9, TIME_STAMP_SIZE);
							time = parse_timestamp(timestamp);
							pts_table[pid] = time;
						} else if (pts_index_table[pid] == 1) {
							pts_index_table[pid] = 2;
							memcpy(timestamp, current_packet + ts_header_size + 9, TIME_STAMP_SIZE);
							time = parse_timestamp(timestamp);
							pts_delta_table[pid] = time - pts_table[pid];
							pts_table[pid] = time;
						} else {
							pts_table[pid] += pts_delta_table[pid];
							stamp_ts(pts_table[pid], current_packet + ts_header_size + 9);
							current_packet[ts_header_size + 9] &= 0x0F; 
							current_packet[ts_header_size + 9] |= 0x20; 														
						}
					
						memcpy(timestamp, current_packet + ts_header_size + 9, TIME_STAMP_SIZE);
						time = parse_timestamp(timestamp);
						// fprintf(stderr, "Pid %d new Audio Presentation Time Stamp is: %llu, %llu.%04llu sec.\n", pid, time,  time / (system_frequency / 300), (time % (system_frequency / 300)) / ((system_frequency / 300) / 10000));
					
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
