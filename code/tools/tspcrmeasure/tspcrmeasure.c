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
	int fd_ts;			/* File descriptor of ts file */
	u_short pid;
	int byte_read;
	unsigned int pcr_ext = 0;
	unsigned int ibits = 0;
	unsigned long long int pcr_base = 0;					
	unsigned long long int ts_packet_count;
	unsigned long long int new_pcr = 0;	
	unsigned long long int new_pcr_index = 0;	
	unsigned long long int pid_pcr_table[MAX_PID];	/* PCR table for the TS packets */
	unsigned long long int pid_pcr_index_table[MAX_PID];	/* PCR index table for the TS packets */
	unsigned char ts_packet[TS_PACKET_SIZE];/* TS packet */

	/* Open ts file */
	if (argc == 3)
	    fd_ts = open(argv[1], O_RDONLY);
	else {
	    fprintf(stderr, "Usage: 'tspcrmeasure input.ts expected_bit/s'\n");
	    return 2;
	}
	if (fd_ts < 0) {
	    fprintf(stderr, "Can't find file %s\n", argv[1]);
	    return 2;
	}
	ibits = atoi(argv[2]);
	if (ibits == 0) {
		fprintf(stderr, "Expected bit/s is zero\n");
		return 2;
	}
	
	/* Start to process the file */
	memset(pid_pcr_table, 0,  MAX_PID*(sizeof(unsigned long long int)));	
	memset(pid_pcr_index_table, 0,  MAX_PID*(sizeof(unsigned long long int)));
	ts_packet_count = 0;
	byte_read = 1;
	while(byte_read) {
	
	    /* Read next packet */
	    byte_read = read(fd_ts, ts_packet, TS_PACKET_SIZE);
	    
	    /* check packet */
	    memcpy(&pid, ts_packet + 1, 2);	
	    pid = ntohs(pid);
	    pid = pid & 0x1fff;
	    if (pid < MAX_PID) {
		if ((ts_packet[3] & 0x20) && (ts_packet[4] != 0) && (ts_packet[5] & 0x10)) { /* there is a pcr field */
		    pcr_base = (((unsigned long long int)ts_packet[6]) << 25) + (ts_packet[7] << 17) + (ts_packet[8] << 9) + (ts_packet[9] << 1) + (ts_packet[10] >> 7);
		    pcr_ext = ((ts_packet[10] & 1) << 8) + ts_packet[11];
		    if (pid_pcr_table[pid] == 0) { 
			pid_pcr_table[pid] = pcr_base * 300 + pcr_ext;
			pid_pcr_index_table[pid] = (ts_packet_count * TS_PACKET_SIZE) + 10;
			fprintf(stdout, "%llu: pid %d, new pcr is %llu (%f sec)\n",
					pid_pcr_index_table[pid],
					pid, 
					pid_pcr_table[pid],
					((double)(pid_pcr_table[pid]) / SYSTEM_CLOCK_FREQUENCY)
				);

		    } else { 
			new_pcr = pcr_base * 300 + pcr_ext;
			new_pcr_index = (ts_packet_count * TS_PACKET_SIZE) + 10;
			fprintf(stdout, "%llu: pid %d, new pcr is %llu (%f sec), pcr delta is %llu, (%f ms), indices delta is %llu bytes,( %f ms), pcr accuracy is %.10f, instant ts bit rate is %.10f\n", 
					new_pcr_index,
					pid, 
					new_pcr,
					((double)(new_pcr) / SYSTEM_CLOCK_FREQUENCY),
					new_pcr - pid_pcr_table[pid],
					((double)((new_pcr - pid_pcr_table[pid]) * 1000)) / SYSTEM_CLOCK_FREQUENCY,
					new_pcr_index - pid_pcr_index_table[pid],
					((double)((new_pcr_index - pid_pcr_index_table[pid]) * 8 * 1000)) / ibits,

					(((double) (new_pcr - pid_pcr_table[pid])) / SYSTEM_CLOCK_FREQUENCY) - (((double)(new_pcr_index - pid_pcr_index_table[pid])) * 8 / ibits),
					(((double)(new_pcr_index - pid_pcr_index_table[pid])) * 8 * SYSTEM_CLOCK_FREQUENCY) /  ((double)(new_pcr - pid_pcr_table[pid])) 
				);
			pid_pcr_table[pid] = new_pcr;
			pid_pcr_index_table[pid] = new_pcr_index;

		    }
		}		
		ts_packet_count++;		
	    }    
	}
	
	return 0;
}
