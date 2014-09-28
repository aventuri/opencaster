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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
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
#define OUTPUT_BUFFER_IN_PACKETS 2127 // 40mbps maximum dvb stream, pcr maximum 40ms -> 200Kbytes buffer, let's take 400K ;-) 

unsigned char null_ts_packet[TS_PACKET_SIZE];
u_char ts_packet[TS_PACKET_SIZE];/* TS packet */
unsigned char output_packet_buffer[OUTPUT_BUFFER_IN_PACKETS * TS_PACKET_SIZE];
unsigned char* current_output_packet = 0;
unsigned long long int ibits;
unsigned long long int obits;	
unsigned long long int fcounter;
unsigned long long int ucounter;
unsigned long long int last_ucounter;
unsigned long long int ts_packet_output;

void fill_buffer(void) {

	/* copy ts packet from input to output buffer */
	memcpy(current_output_packet, ts_packet, TS_PACKET_SIZE);
	current_output_packet += TS_PACKET_SIZE; 
	if (current_output_packet >= output_packet_buffer + TS_PACKET_SIZE * OUTPUT_BUFFER_IN_PACKETS) {
		fprintf(stderr, "buffer too small\n");
		exit(2);
	}

}			

unsigned long long flush_buffer(void) {

	unsigned char* output_packet = 0;
	unsigned long long result = 0;

	for (output_packet = output_packet_buffer; output_packet < current_output_packet; output_packet += TS_PACKET_SIZE) {	

		/* write input packet out */
		write(STDOUT_FILENO, output_packet, TS_PACKET_SIZE);
		ts_packet_output++;

		/* increase counters, every input packet generate free time and consume some*/
		fcounter += TS_PACKET_SIZE * 8 * obits;
		ucounter += TS_PACKET_SIZE * 8 * ibits;
				
		/* generate null packets if there's time */
		while ( ucounter + (TS_PACKET_SIZE * 8 * ibits) <= fcounter) { 

			/* write null packets */
			write(STDOUT_FILENO, null_ts_packet, TS_PACKET_SIZE);
			ts_packet_output++;
				
			/* increase output bit counter */
			last_ucounter = ucounter;
			ucounter += TS_PACKET_SIZE * 8 * ibits;
			if (last_ucounter > ucounter) { /* manage zeroed counter */
				last_ucounter = last_ucounter - fcounter;
				fcounter = 0;
				ucounter = last_ucounter + TS_PACKET_SIZE * 8 * ibits;
			}
		}
		
	}

	/* empty buffer */
	fprintf(stderr, "wasted time %llu\n", (fcounter-ucounter) / SYSTEM_CLOCK_FREQUENCY);
	current_output_packet = output_packet_buffer;
	result = (fcounter - ucounter) / SYSTEM_CLOCK_FREQUENCY;
	fcounter = fcounter - ucounter;
	ucounter = 0;
	last_ucounter = 0;
	
	return result;
}



int main(int argc, char *argv[])
{
	int fd_ts;
	u_short pid;
	int byte_read;	
	unsigned long long int pcr_base = 0ll;					
	unsigned long long int pcr_ext = 0ll;
	unsigned long long int ts_packet_count = 0ll;
	ts_packet_output = 0ll;
	unsigned long long int new_pcr = 0ll;	
	unsigned long long int new_pcr_index = 0ll;
	unsigned long long int pid_pcr_table[MAX_PID];	/* PCR table for the TS packets */
	unsigned long long int pid_pcr_index_table[MAX_PID];	/* PCR index table for the TS packets */

	/* Parse args */	
	if (argc >= 3) {
		fd_ts = open(argv[1], O_RDONLY);
	} else {
		fprintf(stderr, "Usage: 'tsvbcr2cbr input.ts output_bit/s'\n");
		fprintf(stderr, "adds empty packets to input.ts to reach uniform output_bit/s, it needs to read head a pcr to guess bit rate\n");
		fprintf(stderr, "input.ts should be a single program transport stream with only a video elementary stream and valid pcrs\n");
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

	/* Init null packet */
	memset(null_ts_packet, 0, TS_PACKET_SIZE);
	null_ts_packet[0] = 0x47;
	null_ts_packet[1] = 0x1F;
	null_ts_packet[2] = 0xFF;
	null_ts_packet[3] = 0x10;

	/* Init counters */
	fcounter = 0;
	ucounter = 0;	
	last_ucounter = 0;
	byte_read = 1;	
	memset(pid_pcr_table, 0,  MAX_PID * (sizeof(long long int)));	
	memset(pid_pcr_index_table, 0,  MAX_PID * (sizeof(long long int)));
	ts_packet_count = 0;
	current_output_packet = output_packet_buffer;
	ibits = obits; /* until ibit is guessed it's equal to output */

	while (byte_read) {

		/* read a packet */
		byte_read = read(fd_ts, ts_packet, TS_PACKET_SIZE);

	    	/* check packet PCR */
		memcpy(&pid, ts_packet + 1, 2);	
		pid = ntohs(pid);
		pid = pid & 0x1fff;
		if (pid < MAX_PID) { /* valid packet */
			if ((ts_packet[3] & 0x20) && (ts_packet[4] != 0) && (ts_packet[5] & 0x10)) { /* there is a pcr field */
				pcr_base = (ts_packet[6] << 25) & 0x1FFFFFFFFll;
				pcr_base += (ts_packet[7] << 17) & 0x1FFFFFFll; 
				pcr_base += (ts_packet[8] << 9) & 0x1FFFFll;
				pcr_base += (ts_packet[9] << 1) & 0x1FFll;
				pcr_base += ((ts_packet[10] & 0x80) >> 7) & 0x1ll;
				pcr_ext = ((ts_packet[10] & 0x01) << 8) & 0x1FFll;
				pcr_ext += ts_packet[11];
				if (pid_pcr_table[pid] == 0) { /* it's a new pcr pid */ 
					pid_pcr_table[pid] = pcr_base * 300ll + pcr_ext;
					pid_pcr_index_table[pid] = (ts_packet_count * TS_PACKET_SIZE) + 10;
				} else {  /* we can guess a bit rate comparing a previous pcr pid */
					new_pcr = pcr_base * 300ll + pcr_ext;
					new_pcr_index = (ts_packet_count * TS_PACKET_SIZE) + 10;
					fprintf(stderr, "%llu: pid %d, new pcr is %llu, pcr delta is %llu, (%f ms), indices delta is %llu bytes,( %f ms), instant ts bit rate is %.10f\n", 
						new_pcr_index,
						pid, 
						new_pcr,
						new_pcr - pid_pcr_table[pid],
						((double)((new_pcr - pid_pcr_table[pid]) * 1000)) / SYSTEM_CLOCK_FREQUENCY,
						new_pcr_index - pid_pcr_index_table[pid],
						((double)((new_pcr_index - pid_pcr_index_table[pid]) * 8 * 1000)) / ibits,
						(((double)(new_pcr_index - pid_pcr_index_table[pid])) * 8 * SYSTEM_CLOCK_FREQUENCY) /  ((double)(new_pcr - pid_pcr_table[pid])) 
					);
					ibits =	(((double)(new_pcr_index - pid_pcr_index_table[pid])) * 8 * SYSTEM_CLOCK_FREQUENCY) /  ((double)(new_pcr - pid_pcr_table[pid])); 
					if (ibits > obits) {
						fprintf(stderr, "at %llu: output bit rate %llu is smaller then input bit rate %llu\n", new_pcr_index, obits, ibits);
						return 2;  
					}
					flush_buffer();
				
				/*
					new_pcr = (((ts_packet_output * TS_PACKET_SIZE) + 10) * 8 * SYSTEM_CLOCK_FREQUENCY) / obits;
					fprintf(stderr, "new computed pcr is %llu\n" ,new_pcr);
					pcr_base = new_pcr / 300;
					pcr_ext = new_pcr % 300;
					ts_packet[6] = (0xFF & (pcr_base >> 25));
					ts_packet[7] = (0xFF & (pcr_base >> 17));
					ts_packet[8] = (0xFF & (pcr_base >> 9));
					ts_packet[9] = (0xFF & (pcr_base >> 1));
					ts_packet[10] = ((0x1 & pcr_base) << 7) | 0x7E | ((0x100 & pcr_ext) >> 8);
					ts_packet[11] = (0xFF & pcr_ext);	
				*/			
					pid_pcr_table[pid] = new_pcr;
					pid_pcr_index_table[pid] = new_pcr_index;
					
				}
			}
		}
		fill_buffer(); 
		ts_packet_count++;
	}

	return 0;

}
