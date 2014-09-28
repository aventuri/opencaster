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
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <inttypes.h>

#define TS_PACKET_SIZE 188
#define MAX_PID 8192

typedef struct queue {
	int			fd;
	char*			name;
	long long int		time;
	int			repeat_time;
	int			mandatory;
	struct queue*		next; 
} fd_queue;

fd_queue* g_fd_count_queue = 0;		/* cicular queue	*/
fd_queue* g_fd_count_queue_last = 0;	/* add cache circular queue	*/
fd_queue* g_fd_time_queue = 0;		/* time based queue	*/
fd_queue* g_fd_time_queue_last = 0;	/* add cache time based queue	*/
struct timespec g_start_time;

/* nsec difference between timespec */
long long int usecDiff(struct timespec* time_stop, struct timespec* time_start)
{
	long long int temp = 0;
	long long int utemp = 0;
		   
	if (time_stop && time_start) {
		if (time_stop->tv_nsec >= time_start->tv_nsec) {
			utemp = time_stop->tv_nsec - time_start->tv_nsec;    
			temp = time_stop->tv_sec - time_start->tv_sec;
		} else {
			utemp = time_stop->tv_nsec + 1000000000 - time_start->tv_nsec;       
			temp = time_stop->tv_sec - 1 - time_start->tv_sec;
		}
		if (temp >= 0 && utemp >= 0) {
			temp = (temp * 1000000000) + utemp;
        	} else {
			fprintf(stderr, "start time %ld.%ld is after stop time %ld.%ld\n", time_start->tv_sec, time_start->tv_nsec, time_stop->tv_sec, time_stop->tv_nsec);
			temp = -1;
		}
	} else {
		fprintf(stderr, "memory is garbaged?\n");
		temp = -1;
	}
        return temp / 1000;
}

int enqueue_time_fd(int fd, char* name, int repeat_time) {
    
	fd_queue* fd_new_queue;
	
	fd_new_queue = (fd_queue*) malloc(sizeof(fd_queue));
	fd_new_queue->fd = fd;
	fd_new_queue->time = 0;
	fd_new_queue->mandatory = 1;
	fd_new_queue->name = name;
	fd_new_queue->repeat_time = repeat_time;
	fd_new_queue->next = 0;
	if (g_fd_time_queue == 0) {
		g_fd_time_queue = fd_new_queue;
		g_fd_time_queue_last = fd_new_queue;
	} else {
		g_fd_time_queue_last->next = fd_new_queue;
		g_fd_time_queue_last = fd_new_queue;
	}
	
	return (fd < 0);
}

int enqueue_fd(int fd, char* name, int mandatory) {
    
	fd_queue* fd_new_queue;
    
	fd_new_queue = (fd_queue*) malloc(sizeof(fd_queue));
	fd_new_queue->fd = fd;
	fd_new_queue->mandatory = mandatory;
	fd_new_queue->name = name;
	if (g_fd_count_queue == 0) {
		fd_new_queue->next = fd_new_queue;
		g_fd_count_queue = fd_new_queue;
		g_fd_count_queue_last = fd_new_queue;
	} else {
		fd_new_queue->next = g_fd_count_queue;
		g_fd_count_queue_last->next = fd_new_queue;
		g_fd_count_queue_last = fd_new_queue;
	}
    
	return (fd < 0);
}

void choose_fd(fd_queue* chosen_fd) {
    
	fd_queue* fd_queue_temp;
	fd_queue* fd_queue_temp_choice = 0;
	long long int min_time = 0;
	long long int current_time = 0;
	struct timespec time;
	time.tv_sec = 0;
	time.tv_nsec = 0;
	
	if (chosen_fd != 0) {
		chosen_fd->fd = -1;

		/* check first time queue */
		clock_gettime(CLOCK_MONOTONIC, &time);
		current_time = usecDiff(&time, &g_start_time);
		if (current_time < 0) { /* there something fishy with the time measure ... */
			fprintf(stderr, "time measure overflow, get new starting time\n");
			fprintf(stderr, "start time %ld.%ld is after stop time %ld.%ld, current_time is %lld\n", g_start_time.tv_sec, g_start_time.tv_nsec, time.tv_sec, time.tv_nsec, current_time);
			clock_gettime(CLOCK_MONOTONIC, &g_start_time);
			fd_queue_temp = g_fd_time_queue;
			while(fd_queue_temp != 0) {  
				fd_queue_temp->time = 0;
				fd_queue_temp = fd_queue_temp->next;
			}
		} else {  
			fd_queue_temp = g_fd_time_queue;
			fd_queue_temp_choice = 0;
			while(fd_queue_temp != 0) {  
				if (current_time >= fd_queue_temp->time) {  
					if (fd_queue_temp_choice == 0) {
						fd_queue_temp_choice = fd_queue_temp;
						min_time = fd_queue_temp->time;
					} else  if (min_time > fd_queue_temp->time) {
						fd_queue_temp_choice = fd_queue_temp;
						min_time = fd_queue_temp->time;
					} 
				} 
				fd_queue_temp = fd_queue_temp->next;
			}
			if (fd_queue_temp_choice != 0) {
				fd_queue_temp_choice->time += fd_queue_temp_choice->repeat_time * 1000;
				*chosen_fd = *fd_queue_temp_choice;
			}
		}
 
		/* get from normal queue if no founds */ 
		if (g_fd_count_queue != 0 && chosen_fd->fd == -1) { 
			*chosen_fd = *g_fd_count_queue;
			g_fd_count_queue = g_fd_count_queue->next;
		} 
	}
}

void Usage (void) {
	fprintf(stderr, "Usage: 'tsnullshaper input.ts [b:packets_buffer] +file1.ts +file2.ts +file3.ts  ... t:1000 filex.ts ... s: ip.ts'\n");
	fprintf(stderr, "'+file.ts' replace null packets from input.ts with packets from another file.ts\n");
	fprintf(stderr, "'t:m_secs file.ts' replace a null packet from input.ts with a packet from file.ts every m_secs milliseconds of REAL TIME, this packet has priority on +file.ts packets\n");
	fprintf(stderr, "'s:file.ts' try to read from the file.ts, if fails move to next\n");
	fprintf(stderr, "'b:packets_buffer' sets how many packets to use for buffering\n");
	fprintf(stderr, "It was chosen to use REAL TIME instead of PCR LOGICAL TIME so it was possibile to manage TS without PCR\n");
	fprintf(stderr, "This suits anyway PSI insertion case that is not so time strict\n");
}

int main(int argc, char *argv[])
{
	int fd;
	fd_set set;
	u_short pid;
	int buffer_size;
	int fd_input_ts;			/* File descriptor of ts input file */	
	int repeat_time;
	int bytes_read;
	struct timeval tv;
	unsigned int i;
	fd_queue chosen_fd; 
	unsigned char* packet_buffer;
	unsigned char* current_packet;	
	unsigned char pid_cc_table[MAX_PID];	/* PID table for the continuity counter of the TS packets */	
	
	/* Parse command line */ 
	buffer_size = 0;
	if (argc >= 2) {
		fd_input_ts = open(argv[1], O_RDONLY);
		if (fd_input_ts < 0) {
			fprintf(stderr, "Can't find file %s\n", argv[1]);
			return 2;
		}
	} else {
		Usage();
		return 2;
	}
	i = 2;
	while (i < argc) {
		if (argv[i][0] == '+') {
			fd = open(&(argv[i][1]), O_RDONLY);
			if (fd < 0) {
				fprintf(stderr, "can't find file %s\n", &(argv[i][1]));
				return 2;
			} else {	
				if (enqueue_fd(fd, &(argv[i][1]), 1)) {
					return 2;
				}
			}
			i++;
		} else if (argv[i][0] == 's' && argv[i][1] == ':') {
			fd = open(&(argv[i][2]), O_RDONLY);
			if (fd < 0) {
				fprintf(stderr, "can't find file %s\n", &(argv[i][1]));
				return 2;
			} else {	
				if (enqueue_fd(fd, &(argv[i][2]), 0)) {
					return 2;
				}
			}
			i++;
		} else if (argv[i][0] == 't' && argv[i][1] == ':') {
			repeat_time = atoi(&(argv[i][2]));
			fd = open(argv[i+1], O_RDONLY);
			if (repeat_time > 0 && fd >= 0) {
				if(enqueue_time_fd(fd, argv[i+1], repeat_time)) {
					return 2;
				}
			} else if (repeat_time < 0){
				fprintf(stderr, "specified time is not positive\n");	
				return 2;
			} else if (fd < 0) {
				fprintf(stderr, "can't find file %s\n", argv[i+1]);
				return 2;
			}
			i+=2;
		} else if (argv[i][0] == 'b' && argv[i][1] == ':') {
			buffer_size = atoi(&(argv[i][2]));
			if (buffer_size <= 0) {
				fprintf(stderr, "buffer size is not valid\n");
				return 2;
			}
			i+=1;
		} else {
			Usage();
			return 2;
		}
	}
 
	/* Start to process the input */	
	memset(pid_cc_table, 0x10,  MAX_PID);	
	if (buffer_size <= 0)
		buffer_size = 1;
	buffer_size *= TS_PACKET_SIZE;
	packet_buffer = malloc(buffer_size);
	if (packet_buffer == NULL) {
		fprintf(stderr, "Out of memory!\n");
		return 2;
	}
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	g_start_time.tv_sec = 0;
	g_start_time.tv_nsec = 0;
	clock_gettime(CLOCK_MONOTONIC, &g_start_time);
	while(1) {

		/* Read packets */
		read(fd_input_ts, packet_buffer, buffer_size);
	
		/* Check for nulls */
		for(i = 0; i < buffer_size; i += TS_PACKET_SIZE) {
		
			current_packet = packet_buffer + i;
						
			/* check if it's a null packet */
			memcpy(&pid, current_packet + 1, 2);	
			pid = ntohs(pid);
			pid = pid & 0x1fff;
			if (pid == 8191) { /* replace */
				choose_fd(&chosen_fd);
				if (chosen_fd.fd >= 0) {
					if (chosen_fd.mandatory) {
						bytes_read = read(chosen_fd.fd, current_packet, TS_PACKET_SIZE);
						if (bytes_read < TS_PACKET_SIZE) {
							close(chosen_fd.fd);
							chosen_fd.fd = open(chosen_fd.name, O_RDONLY);
							bytes_read = read(chosen_fd.fd, current_packet, TS_PACKET_SIZE);
						}
					} else {
						FD_ZERO(&set);
						FD_SET(chosen_fd.fd,&set);
						if (select(chosen_fd.fd + 1, &set, NULL, NULL, &tv) > 0) {
							bytes_read = read(chosen_fd.fd, current_packet, TS_PACKET_SIZE);
							if (bytes_read < TS_PACKET_SIZE) {
								close(chosen_fd.fd);
								chosen_fd.fd = open(chosen_fd.name, O_RDONLY);
								bytes_read = read(chosen_fd.fd, current_packet, TS_PACKET_SIZE);
							} 
						} 
					}
				}
				/* check packets cc */
				memcpy(&pid, current_packet + 1, 2);	
				pid = ntohs(pid);
				pid = pid & 0x1fff;
				if (pid < MAX_PID) {
					if (pid_cc_table[pid] == 0x10) { 
						pid_cc_table[pid] = current_packet[3] & 0x0f; /* new stream to track cc */
					} else { 
						pid_cc_table[pid] = (pid_cc_table[pid] + 1) % 0x10; /* check last cc for this pid, increase and restamp */
						current_packet[3] = (pid_cc_table[pid] | (current_packet[3] & 0xf0));
					}
				}	
				
			}
			
		}
				
    		/* Write packets */
		write(STDOUT_FILENO, packet_buffer, buffer_size);
	}
	
	return 0;
}
