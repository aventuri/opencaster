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
 * Foundation, Inc., 59 Temple Place- Suite 330, Boston, MA  02111-1307, USA.
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
#define TS_EXTRA_HEADER 4
#define MAX_PID 8191
#define SYSTEM_CLOCK_FREQUENCY 27000000

int main(int argc, char *argv[])
{
	int fd_ts;
	unsigned short pid;
	int byte_read;	
	unsigned long long int pcr_base = 0ll;
	unsigned long long int input_ts_packet_count = 0ll;
	unsigned long long int new_pcr = 0ll;
	unsigned long long int new_pcr_index = 0ll;
	unsigned long long int old_pcr = 0ll;
	unsigned long long int old_pcr_index = 0ll;
	unsigned char null_output_ts_packet[TS_PACKET_SIZE];
	unsigned char input_ts_packet[TS_EXTRA_HEADER + TS_PACKET_SIZE];
	unsigned long long int ibits = 0ll;
	unsigned long long int obits = 0ll;
	unsigned long long int fcounter = 0ll;
	unsigned long long int ucounter = 0ll;
	unsigned long long int last_ucounter = 0ll;
	unsigned long long int ts_packet_output = 0ll;
	
	/* Parse args */
	if (argc > 1) {
		fd_ts = open(argv[1], O_RDONLY);
	} else {
		fprintf(stderr, "Usage: 'm2ts2cbrts input.ts [output_bit_rate]'\n");
		fprintf(stderr, "adds empty packets to input.ts to reach uniform output_bit/s, default is 48mbps\n");
		return 2;
	}
	if (fd_ts < 0) {
		fprintf(stderr, "Can't find file %s\n", argv[1]);
		return 2;
	}
	if (argc > 2) {
		obits = atol(argv[2]);
		if (obits == 0) {
			fprintf(stderr, "0 output_bit/s?\n");
			return 2;
		}
	} else {
		obits = 48000000L;
	}
	fprintf(stderr, "output bps is %llu\n", obits);

	/* Init null packet */
	memset(null_output_ts_packet, 0, TS_PACKET_SIZE);
	null_output_ts_packet[0] = 0x47;
	null_output_ts_packet[1] = 0x1F;
	null_output_ts_packet[2] = 0xFF;
	null_output_ts_packet[3] = 0x10;

	/* Init counters */
	fcounter = 0;
	ucounter = 0;	
	last_ucounter = 0;
	old_pcr = 0;
	old_pcr_index = 0;
	input_ts_packet_count = 0;
	ibits = obits; /* until ibit is guessed it's equal to output */

	byte_read = 1;
	while (byte_read) {
	
		/* read a packet */
		byte_read = read(fd_ts, input_ts_packet, TS_EXTRA_HEADER + TS_PACKET_SIZE);
		
		memcpy(&pid, input_ts_packet + 5, 2);
		pid = ntohs(pid);
		pid = pid & 0x1fff;
		if (pid < MAX_PID) {
			
			pcr_base = (input_ts_packet[0] & 0x3F) << 24;
			pcr_base += input_ts_packet[1] << 16;
			pcr_base += input_ts_packet[2] << 8;
			pcr_base += input_ts_packet[3];
			
			if (old_pcr_index == 0) {
				
				old_pcr = pcr_base;
				old_pcr_index = input_ts_packet_count * TS_PACKET_SIZE;
				write(STDOUT_FILENO, input_ts_packet +  TS_EXTRA_HEADER, TS_PACKET_SIZE);
				input_ts_packet_count++;
				
			} else {  /* we can guess a bit rate comparing a previous pcr */
				new_pcr = pcr_base;
				new_pcr_index = input_ts_packet_count * TS_PACKET_SIZE;
				
				if (new_pcr < old_pcr) {
					new_pcr += 0x40000000;
					ibits = (((double)(new_pcr_index - old_pcr_index)) * 8 * SYSTEM_CLOCK_FREQUENCY) /  ((double)(new_pcr - old_pcr)); 
					new_pcr -= 0x40000000;
				} else {
					ibits = (((double)(new_pcr_index - old_pcr_index)) * 8 * SYSTEM_CLOCK_FREQUENCY) /  ((double)(new_pcr - old_pcr)); 
				}
				
				if (ibits > obits) {
					fprintf(stderr, "at %llu: output bit rate %llu is smaller then input bit rate %llu\n", new_pcr_index, obits, ibits);
					return 2;
				}
				
				if (ibits != obits){
					
					/*
					fprintf(stderr, "%llu: pid %d, new pcr is %llu, pcr delta is %llu, instant ts bit rate is %.10f\n", 
						new_pcr_index,
						pid, 
						new_pcr,
						new_pcr - old_pcr,
						(((double)(new_pcr_index - old_pcr_index)) * 8 * SYSTEM_CLOCK_FREQUENCY) /  ((double)(new_pcr - old_pcr)) 
					);
					*/
					
					fcounter += TS_PACKET_SIZE * 8 * obits;
					ucounter = TS_PACKET_SIZE * 8 * ibits;
					
					while ( ucounter + (TS_PACKET_SIZE * 8 * ibits) < fcounter) {
						
						write(STDOUT_FILENO, null_output_ts_packet, TS_PACKET_SIZE);
						ts_packet_output++;
						
						last_ucounter = ucounter;
						ucounter += TS_PACKET_SIZE * 8 * ibits;
						if (last_ucounter > ucounter) {
							last_ucounter = last_ucounter - fcounter;
							fcounter = 0;
							ucounter = last_ucounter + TS_PACKET_SIZE * 8 * ibits;
						}
					}
					
					/*
					fprintf(stderr, "generated null bits for time %llu\n", fcounter-ucounter); 
					*/
					fcounter = fcounter - ucounter;
					ucounter = 0;
					last_ucounter = 0;
				
				}
				
				write(STDOUT_FILENO, input_ts_packet +  TS_EXTRA_HEADER, TS_PACKET_SIZE);
				input_ts_packet_count++;
				
				old_pcr = new_pcr;
				old_pcr_index = new_pcr_index;
			}
		} else {
			write(STDOUT_FILENO, input_ts_packet +  TS_EXTRA_HEADER, TS_PACKET_SIZE);
			input_ts_packet_count++;
		}
	}
	
	return 0;
}
