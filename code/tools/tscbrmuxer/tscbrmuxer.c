/*  
 * Copyright (C) 2005-2013  Lorenzo Pallara, l.pallara@avalpa.com
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
#include <sys/time.h>
#include <inttypes.h>

#define TS_PACKET_SIZE 188
#define MAX_PID 8192

typedef struct list {
	int			fd;
	char			close_fd;
	char*			filename;
	double			bps;
	double			output_packets; /* will take thousend of years to overflown at 100mbps */
	struct list*		next; 
} fd_list;

fd_list* g_fd_list = 0;
fd_list* g_fd_list_last = 0;

int enqueue_fd(int fd, char* filename, int bps, char close_fd) {
    
	fd_list* fd_new_list;
    
	fd_new_list = (fd_list*) malloc(sizeof(fd_list));
	fd_new_list->fd = fd;
	fd_new_list->bps = bps;
	fd_new_list->filename = filename;
	fd_new_list->output_packets = 0;
	fd_new_list->next = 0;
	fd_new_list->close_fd = close_fd;
	if (g_fd_list == 0) {
		g_fd_list = fd_new_list;
		g_fd_list_last = fd_new_list;
	} else {
		g_fd_list_last->next = fd_new_list;
		g_fd_list_last = fd_new_list;
	}
    
	return (fd < 0);
}

fd_list* choose_fd(void) {

	int not_chosen = 1;
	double min = 0;
	double temp = 0;
	fd_list* fd_list_index;
	fd_list* result;


	
	fd_list_index = g_fd_list;
	while(fd_list_index != 0) {
		
		/* search the min */
		temp = (fd_list_index->output_packets * TS_PACKET_SIZE * 8) / fd_list_index->bps;
		if (not_chosen || temp < min) {
			min = temp;
			result = fd_list_index;
			not_chosen = 0;
		}
		
		fd_list_index = fd_list_index->next;
	}
	
	result->output_packets++;
	return result;
}

void Usage (void) {
	fprintf(stderr, "Usage: 'tscbrmuxer b:bitrate_pat pat.ts b:bitrate_cat cat.ts b:pmt1_bitrate pmt1.ts b:nit_bitrate nit.ts ... where bitrate in 'b:bitrate file.ts' is in bit per second\n");
	fprintf(stderr, "Usage: 'o:bitrate null.ts can be used to specify final output ts bit rate, must be the last parameter\n");
	fprintf(stderr, "Usage: 'c:bitrate file.ts can be used to specify that when the file.ts ends the mux stops\n");
}

int main(int argc, char *argv[])
{
	int fd;
	int bps;
	int byte_read;
	unsigned short pid;
	unsigned char adaptation_field;
	double output_bitrate;
	unsigned int i;
	unsigned char ts_packet[TS_PACKET_SIZE];
	fd_list* chosen_fd; 
	unsigned char pid_cc_table[MAX_PID];	/* PID table for the continuity counter of the TS packets */	
	unsigned char previous_cc_table[MAX_PID]; /* two packets can have the same continuity counter under some conditions... */	

	memset(pid_cc_table, 0x10,  MAX_PID);	
	memset(previous_cc_table, 0x10,  MAX_PID);

	
	/* Parse command line */ 
	if (argc < 3) {
		Usage();
		return 2;
	}
	
	i = 1;
	while (i < argc) {
		if (argv[i][0] == 'b' && argv[i][1] == ':') {
			bps = atoi(&(argv[i][2]));
			fd = open(argv[i+1], O_RDONLY);
			if (bps > 0 && fd >= 0) {
				if(enqueue_fd(fd, argv[i+1], bps, 0))
					return 2;
			} else if (bps < 0){
				fprintf(stderr, "specified bps is not positive\n");	
				return 2;
			} else if (fd < 0) {
				fprintf(stderr, "can't find file %s\n", argv[i+1]);
				return 2;
			}
			output_bitrate += bps;
			i+=2;
		} else if (argv[i][0] == 'c' && argv[i][1] == ':') {
			bps = atoi(&(argv[i][2]));
			fd = open(argv[i+1], O_RDONLY);
			if (bps > 0 && fd >= 0) {
				if(enqueue_fd(fd, argv[i+1], bps, 1))
					return 2;
			} else if (bps < 0){
				fprintf(stderr, "specified bps is not positive\n");	
				return 2;
			} else if (fd < 0) {
				fprintf(stderr, "can't find file %s\n", argv[i+1]);
				return 2;
			}
			output_bitrate += bps;
			i+=2;
		} else if (argv[i][0] == 'o' && argv[i][1] == ':') {
			bps = atoi(&(argv[i][2]));
			if (bps <= output_bitrate) {
				fprintf(stderr, "total bitrate is less or equale than sum of current bitrates\n");
				return 2;
			}
			fd = open(argv[i+1], O_RDONLY);
			if (bps > 0 && fd >= 0) {
				if(enqueue_fd(fd, argv[i+1], bps - output_bitrate, 0))
					return 2;
				output_bitrate = bps;
			} else if (bps < 0){
				fprintf(stderr, "specified bps is not positive\n");	
				return 2;
			} else if (fd < 0) {
				fprintf(stderr, "can't find file %s\n", argv[i+1]);
				return 2;
			}
			i = argc;
		} else {
			Usage();
			return 2;
		}
	}
	fprintf(stderr, "output bitrate: %f\n", output_bitrate);

	/* mux them */
	while(1) {
	
		chosen_fd = choose_fd();
		
		/* read packet */
		byte_read = 0;
		byte_read = read(chosen_fd->fd, ts_packet, TS_PACKET_SIZE);
		if (byte_read < TS_PACKET_SIZE) {
			close(chosen_fd->fd);
			if (chosen_fd->close_fd) {
				return 0;
			}
			chosen_fd->fd = open(chosen_fd->filename, O_RDONLY);
			if (chosen_fd->fd < 0) {
				fprintf(stderr, "Can't find file %s\n", chosen_fd->filename);
				return 2;
			}
			byte_read = read(chosen_fd->fd, ts_packet, TS_PACKET_SIZE);
			if (byte_read == TS_PACKET_SIZE) {
			    memcpy(&pid, ts_packet + 1, 2);
			    pid = ntohs(pid);
			    pid = pid & 0x1fff;
			    previous_cc_table[pid] = 0x10;			
			}
		}
	    
		/* check packets cc */
		memcpy(&pid, ts_packet + 1, 2);	
		pid = ntohs(pid);
		pid = pid & 0x1fff;
		if (pid < MAX_PID) {
			if (pid_cc_table[pid] == 0x10) { 
				fprintf(stderr, "new pid entry %d\n", pid);
				pid_cc_table[pid] = ts_packet[3] & 0x0f; /* new stream to track cc */
				previous_cc_table[pid] = ts_packet[3] & 0x0f;
			} else { 
				adaptation_field = (ts_packet[3] & 0x30) >> 4;
				if (adaptation_field == 0x0 || adaptation_field == 0x2) { /* reserved, no increment */
					ts_packet[3] = (pid_cc_table[pid] | (ts_packet[3] & 0xf0));		
				} else if ((adaptation_field == 0x1) && ((ts_packet[3] & 0x0f) == previous_cc_table[pid])) { /* double packet accepted only once */
					ts_packet[3] = (pid_cc_table[pid] | (ts_packet[3] & 0xf0));
					previous_cc_table[pid] = 0x10;
				} else if ((adaptation_field == 0x3) && ((ts_packet[3] & 0x0f) == previous_cc_table[pid])) { /* double packet accepted only once */
					ts_packet[3] = (pid_cc_table[pid] | (ts_packet[3] & 0xf0));
					previous_cc_table[pid] = 0x10;
				} else { /* increase the cc and restamp */
					previous_cc_table[pid] = ts_packet[3] & 0x0f;
					pid_cc_table[pid] = (pid_cc_table[pid] + 1) % 0x10; 
					ts_packet[3] = (pid_cc_table[pid] | (ts_packet[3] & 0xf0));
				} 
			}
		}
	
		/* write packet */
		write(STDOUT_FILENO, ts_packet, TS_PACKET_SIZE);
		
		
	}
	
	return 0;
}
