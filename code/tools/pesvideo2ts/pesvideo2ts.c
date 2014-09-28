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

#define _BSD_SOURCE 1

#include <stdio.h> 
#include <stdio_ext.h> 
#include <unistd.h> 
#include <netinet/ether.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define MAX_PID 8192
#define TS_HEADER_SIZE 4 
#define TS_PACKET_SIZE 188
#define PES_HEADER_SIZE 19 /* check also for pts and dts */
#define PCR_DELTA 39 /* ms */

#define SYSTEM_CLOCK 90000.0
#define PTS_MAX 8589934592LL

#define TIME_STAMP_SIZE 5

#define PACK_HEADER_SIZE 4

#define MAX_FD 256

const long long unsigned system_frequency = 27000000;


int ts_payload;				/* TS packet payload counter */
unsigned short pid;				/* Pid for the TS packets */
unsigned char ts_continuity_counter;		/* Continuity counter */
unsigned char ts_packet[TS_PACKET_SIZE];	/* TS packet */
unsigned char look_ahead_buffer[PES_HEADER_SIZE];
unsigned char null_ts_packet[TS_PACKET_SIZE];
unsigned char pcr_ts_packet[TS_PACKET_SIZE];
unsigned char look_ahead_size;
int frame_rate = 0;
unsigned long long last_frame_index = 0L;
unsigned long long last_frame_popped = 0L;
unsigned long long old_pcr = 0L;
unsigned long long frames	= 0L;		/* frames counter */	
int vbv_buffer_size		= 0L;		/* size of the video buffer verifier in bit */
unsigned long long es_input_bytes = 0;		/* input bytes of the elementary stream */
unsigned long long es_input_bytes_old = 0;	/* input bytes of the elementary stream at previous analysis */
unsigned long long ts_output_bytes = 0;		/* output bytes of the transport stream */
int es_bitrate = 0;				/* bitrate of the input elementary stream */
int ts_bitrate = 0;				/* bitrate of the output transport stream */
int vbv_max = 0;				/* video buffer verifier maximum size */
int vbv_current_size = 0;			/* video buffer verifier current size */
int first_frame_ts_size = 0;
unsigned long long int pts; 			/* PTS last value */
unsigned long long int pts_delta; 		/* PTS delta increment */
unsigned char pts_index;			/* PTS index  for the TS packets */
int first_pts = 0;
char filename[PATH_MAX];
int pcr_repetition_rate = PCR_DELTA;

typedef struct queue {
	unsigned long long	size;
	struct queue*		next;
} queue;

queue* g_head_queue = 0;
queue* g_tail_queue = 0;

unsigned long long pop_frame(void){

	int result;
	queue* temp;
	
	if (g_head_queue) {
		result = g_head_queue->size;
		temp = g_head_queue->next;
		free(g_head_queue);
		g_head_queue = temp;
	} else {
		result = 0;
	}
	
	return result;
}

void enqueue_frame(unsigned long long size) {
    
	queue* new_queue;
	
	new_queue = (queue*) malloc(sizeof(queue));
	new_queue->size = size;
	new_queue->next = 0;
	if (g_head_queue == 0) {				/* first element */ 
		g_head_queue = new_queue; 
		g_tail_queue = new_queue;
	} else {
		g_tail_queue->next = new_queue;		/* last element on tail */
		g_tail_queue = new_queue;
	}
}

void emulate_vbv_tick(void) {

	if (frames > 0) { /* first frame was sent on output, second frame header found on input */
		
		if (last_frame_popped == 0 && g_head_queue != 0) { /* first frame is flushed as soon as received */
			
			/* fprintf(stderr, "vbv size before frame %llu is %d\n", last_frame_popped + 1, vbv_current_size); */
			vbv_current_size -= pop_frame() * 8;
			last_frame_popped = 1;
			/* fprintf(stderr, "vbv size after frame %llu is %d\n", last_frame_popped, vbv_current_size); */
			
		} else {
			
			/* others frames are flushed at the receiver every 1/fps after the first frame receiving time */
			/* if output time is bigger then first frame time + frames * frame rate, it's time to flush a frame */
			
			if (frame_rate == 24 || frame_rate == 25 || frame_rate == 50 || frame_rate == 30 || frame_rate == 60) {
				if (ts_output_bytes >= (first_frame_ts_size + (((1.0 / frame_rate) * ts_bitrate * (last_frame_popped + 1)) / 8)) ) {
					/* fprintf(stderr, "vbv size before frame %llu is %d\n", last_frame_popped + 1, vbv_current_size); */
					vbv_current_size -= pop_frame() * 8;
					last_frame_popped++;
					/* fprintf(stderr, "vbv size after frame %llu is %d\n", last_frame_popped, vbv_current_size);  */
				}
			} else if (frame_rate == 23 || frame_rate == 29 || frame_rate == 59) {
				/* fprintf(stderr, "output bytes are %llu, evaluation is %f\n", ts_output_bytes, (first_frame_ts_size + ((f_frame_rate * ts_bitrate * (last_frame_popped + 1)) / 8)));  */
				if (ts_output_bytes >= (first_frame_ts_size + ((1001.0 * ts_bitrate * (last_frame_popped + 1)) / (8 * (frame_rate + 1) * 1000.0)))) {
					/* fprintf(stderr, "vbv size before frame %llu is %d\n", last_frame_popped + 1, vbv_current_size); */
					vbv_current_size -= pop_frame() * 8;
					last_frame_popped++;
					/* fprintf(stderr, "vbv size after frame %llu is %d\n", last_frame_popped, vbv_current_size); */
				}
			}
			
		}
	}
	
}

void send_pcr_packet() {

	unsigned long long pcr_base;
	unsigned long long pcr_ext;
	unsigned long long new_pcr;
	
	new_pcr = ((ts_output_bytes + 10) * 8 * system_frequency) / ts_bitrate;
	
	/* fprintf(stderr, "pcr is %llu, time is %llu.%04llu sec\n", new_pcr, new_pcr / system_frequency, (new_pcr % system_frequency) / (system_frequency / 10000)); */
	if (new_pcr >  2576980377600ll) {
		new_pcr -=  2576980377600ll; 
		if (frame_rate == 24 || frame_rate == 25 || frame_rate == 50 || frame_rate == 30 || frame_rate == 60) {
			/*
			fprintf(stderr, "ts_output_bytes before %llu\n", ts_output_bytes);
			fprintf(stderr, "vbv evaluation before %llu\n", (unsigned long long)(first_frame_ts_size + (((1.0 / frame_rate) * ts_bitrate * (last_frame_popped + 1)) / 8)));
			fprintf(stderr, "pcr loop\n");
			*/
			unsigned long long vbv_diff = ts_output_bytes - (first_frame_ts_size + (((1.0 / frame_rate) * ts_bitrate * (last_frame_popped + 1)) / 8));
			/* adjust the vbv fullness to the new ts_output_bytes value */
			ts_output_bytes = (((new_pcr * ts_bitrate) + (80ll * system_frequency)) / (system_frequency * 8)) - 20;
			last_frame_popped = 1;
			first_frame_ts_size = ts_output_bytes - vbv_diff - (((1.0 / frame_rate) * ts_bitrate * 2) / 8);
			emulate_vbv_tick();
			/*
			fprintf(stderr, "ts_output_bytes after %llu\n", ts_output_bytes);
			fprintf(stderr, "vbv evaluation after %llu\n",(unsigned long long)(first_frame_ts_size + (((1.0 / frame_rate) * ts_bitrate * (last_frame_popped + 1)) / 8)));
			*/
		} else if (frame_rate == 23 || frame_rate == 29 || frame_rate == 59) {
			float f_frame_rate = (frame_rate + 1 ) * 1000;
			/*
			fprintf(stderr, "ts_output_bytes before %llu\n", ts_output_bytes);
			fprintf(stderr, "vbv evaluation before %llu\n", (unsigned long long)(first_frame_ts_size + (((f_frame_rate / 1001) * ts_bitrate * (last_frame_popped + 1)) / 8)));
			fprintf(stderr, "pcr loop\n");
			*/
			unsigned long long vbv_diff = ts_output_bytes - (first_frame_ts_size + (((f_frame_rate / 1001) * ts_bitrate * (last_frame_popped + 1)) / 8));
			/* adjust the vbv fullness to the new ts_output_bytes value  */
			ts_output_bytes = (((new_pcr * ts_bitrate) + (80ll * system_frequency)) / (system_frequency * 8)) - 20;
			last_frame_popped = 1;
			first_frame_ts_size = ts_output_bytes - vbv_diff - (((f_frame_rate / 1001) * ts_bitrate * 2) / 8);
			emulate_vbv_tick();
			/*
			fprintf(stderr, "ts_output_bytes after %llu\n", ts_output_bytes);
			fprintf(stderr, "vbv evaluation after %llu\n",(unsigned long long)(first_frame_ts_size + (((f_frame_rate / 1001) * ts_bitrate * (last_frame_popped + 1)) / 8)));
			*/
		}
		
	}
	
	/* marshal pcr */
	pcr_base = new_pcr / 300;
	pcr_ext = new_pcr % 300;
	pcr_ts_packet[6] = (0xFF & (pcr_base >> 25));
	pcr_ts_packet[7] = (0xFF & (pcr_base >> 17));
	pcr_ts_packet[8] = (0xFF & (pcr_base >> 9));
	pcr_ts_packet[9] = (0xFF & (pcr_base >> 1));
	pcr_ts_packet[10] = ((0x1 & pcr_base) << 7) | 0x7E | ((0x100 & pcr_ext) >> 8);
	pcr_ts_packet[11] = (0xFF & pcr_ext);
	
	pcr_ts_packet[3] = ts_continuity_counter | 0x20; /* continuity counter, no scrambling, only adaptation field */
	ts_continuity_counter = (ts_continuity_counter + 1) % 0x10; /* inc. continuity counter */
	write(STDOUT_FILENO, pcr_ts_packet, TS_PACKET_SIZE);
	ts_output_bytes += TS_PACKET_SIZE;
	
}

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

void restamp_ptsdts_and_output(void) {

	/* check PTS and DTS */
	unsigned char adapt = (ts_packet[3] >> 4) & 0x03;
	unsigned long long time = 0;
	unsigned long long int dts; 
	unsigned char timestamp[TIME_STAMP_SIZE];
	int ts_header_size = 0;
	int pes_header_size = 0;
	int first_packet_of_an_iframe = 0;
	
	if (adapt == 0) {
		ts_header_size = TS_PACKET_SIZE; /* the packet is invalid ?*/
		;
	} else if (adapt == 1) {
		ts_header_size = TS_HEADER_SIZE; /* only payload */
	} else if (adapt == 2) { 
		ts_header_size = TS_PACKET_SIZE; /* only adaptation field */
	} else if (adapt == 3) {
		ts_header_size = TS_HEADER_SIZE + ts_packet[4] + 1; /* jump the adaptation field */
	} else {
		ts_header_size = TS_PACKET_SIZE; /* not managed */
	}
	
	/* check the time difference between first two pts and offset all the others */
	time = 0;
	if (ts_header_size + 20 < TS_PACKET_SIZE) {
		if ((ts_packet[ts_header_size] == 0x00) && (ts_packet[ts_header_size + 1] == 0x00) && (ts_packet[ts_header_size + 2] == 0x01)) { 
			pes_header_size = ts_packet[ts_header_size + 8];
			if ((ts_packet[ts_header_size + 3] >> 4) == 0x0E) { /* PES video stream */
				memcpy(timestamp, ts_packet + ts_header_size + 9, TIME_STAMP_SIZE);
				time = parse_timestamp(timestamp);
				/* fprintf(stderr, "Old Video Presentation Time Stamp is: %llu, %llu.%04llu sec.\n", time,  time / (system_frequency / 300), (time % (system_frequency / 300)) / ((system_frequency / 300) / 10000)); */
				if (pes_header_size > 5) {
					memcpy(timestamp, ts_packet + ts_header_size + 14, TIME_STAMP_SIZE);
					time = parse_timestamp(timestamp);
					/* fprintf(stderr, "Old Video Decode Time Stamp is: %llu, %llu.%04llu sec.\n", time,  time / (system_frequency / 300), (time % (system_frequency / 300)) / ((system_frequency / 300) / 10000)); */
				}
				
				if (pts_index == 0) {
					if (pes_header_size > 5) { /* if there are both dts and pts, get dts */
						memcpy(timestamp, ts_packet + ts_header_size + 14, TIME_STAMP_SIZE);
						time = parse_timestamp(timestamp);
					} else { /* othewise they are the same */
						memcpy(timestamp, ts_packet + ts_header_size + 9, TIME_STAMP_SIZE);
						time = parse_timestamp(timestamp);
					}
					pts_index = 1;
					pts = time;
				} else if (pts_index == 1) {
					if (pes_header_size > 5) { /* if there are both dts and pts, get dts */
						memcpy(timestamp, ts_packet + ts_header_size + 14, TIME_STAMP_SIZE);
						time = parse_timestamp(timestamp);
					} else { /* othewise they are the same */
						memcpy(timestamp, ts_packet + ts_header_size + 9, TIME_STAMP_SIZE);
						time = parse_timestamp(timestamp);
					}
					pts_index = 2;
					pts_delta = time - pts;
					pts = time;
				} else {
					if (pes_header_size > 5) { /* if there are both dts and pts */
						memcpy(timestamp, ts_packet + ts_header_size + 9, TIME_STAMP_SIZE);
						time = parse_timestamp(timestamp);
						memcpy(timestamp, ts_packet + ts_header_size + 14, TIME_STAMP_SIZE);
						time -= parse_timestamp(timestamp);
						pts += pts_delta; /* dts goes up 1 step */
						dts = pts;
						stamp_ts(dts % PTS_MAX, ts_packet + ts_header_size + 14);
						ts_packet[ts_header_size + 14] &= 0x0F; 
						ts_packet[ts_header_size + 14] |= 0x30; 
						/* pts goes up the same gap there was before */
						stamp_ts((pts + time) % PTS_MAX, ts_packet + ts_header_size + 9);
						ts_packet[ts_header_size + 9] &= 0x0F; 
						ts_packet[ts_header_size + 9] |= 0x20; 
						first_packet_of_an_iframe = ((ts_packet[ts_header_size + 25] & 0x38) >> 3) == 1;
						
					} else {
						memcpy(timestamp, ts_packet + ts_header_size + 9, TIME_STAMP_SIZE);
						time = parse_timestamp(timestamp);
						pts += pts_delta;
						stamp_ts(pts % PTS_MAX, ts_packet + ts_header_size + 9);
						ts_packet[ts_header_size + 9] &= 0x0F; 
						ts_packet[ts_header_size + 9] |= 0x20;
					}
					
				}
				
				memcpy(timestamp, ts_packet + ts_header_size + 9, TIME_STAMP_SIZE);
				time = parse_timestamp(timestamp);
				if (first_pts) {
					fprintf(stderr, "pesvideo2ts sync: %s new presented video frame is at: %llu, %llu.%04llu sec.", filename, time,  time / (system_frequency / 300), (time % (system_frequency / 300)) / ((system_frequency / 300) / 10000)); 
				}	
				if (pes_header_size > 5) {
					memcpy(timestamp, ts_packet + ts_header_size + 14, TIME_STAMP_SIZE);
					time = parse_timestamp(timestamp);
					if (first_pts) {
						fprintf(stderr, ", decode time stamp is at: %llu, %llu.%04llu sec.\n", time,  time / (system_frequency / 300), (time % (system_frequency / 300)) / ((system_frequency / 300) / 10000));
					}
				} else {
					if (first_pts) {
						fprintf(stderr, "\n");
					}
				}
				if (first_pts) {
					first_pts = 0;
				}
			}
		}
	}
	
	write(STDOUT_FILENO, ts_packet, TS_PACKET_SIZE);
	
}

void send_current_packet(unsigned long long es_payload) {
	
	int i;
	unsigned char temp;
	
	if (TS_HEADER_SIZE + ts_payload == TS_PACKET_SIZE) { /* payload fits */
		
		ts_packet[3] = ts_continuity_counter | 0x10; /* continuity counter, no scrambling, only payload */
 		ts_continuity_counter = (ts_continuity_counter + 1) % 0x10; /* inc. continuity counter */
		restamp_ptsdts_and_output();
		ts_output_bytes += TS_PACKET_SIZE;
		ts_payload = 0;
		vbv_current_size += es_payload * 8;
	
	} else if (TS_HEADER_SIZE + ts_payload + 1 == TS_PACKET_SIZE) { /* payload too big: two packets are necessary */
		
		temp = ts_packet[TS_HEADER_SIZE + ts_payload - 1]; /* copy the exceeding byte */ 
		ts_payload--;
		send_current_packet(es_payload - 1); /* ts payload is es payload because pes header can't occuer at the end of the pes... */
		memcpy(ts_packet + 1, &pid, 2); /* pid, no pusu */
		ts_packet[4] = temp;
		ts_payload = 1;
		send_current_packet(1);
		
	} else { /* payload too small: padding is necessary */
		
		ts_packet[3] = ts_continuity_counter | 0x30; /* continuity counter, no scrambling, adaptation field and payload */
		ts_continuity_counter = (ts_continuity_counter + 1) % 0x10; /* inc. continuity counter */
		
		for (i = 0; i < ts_payload; i++) { /* move the payload at the end */
			ts_packet[TS_PACKET_SIZE - 1 - i] = ts_packet[TS_HEADER_SIZE + ts_payload - 1 - i];
		}
		ts_packet[4] = TS_PACKET_SIZE - ts_payload - TS_HEADER_SIZE - 1; /* point to the first payload byte */
		ts_packet[5] = 0x00; /* no options */
		for ( i = TS_HEADER_SIZE + 2 ; i < TS_PACKET_SIZE - ts_payload; i++) { /* pad the packet */
			ts_packet[i] = 0xFF;
		}
		restamp_ptsdts_and_output();
		ts_output_bytes += TS_PACKET_SIZE;
		ts_payload = 0;
		vbv_current_size += es_payload * 8;
	}
	
	/* emulate vbv on the receiver */
	emulate_vbv_tick();
	
	/* check if it is time for a pcr packet */
	if ( (ts_output_bytes + TS_PACKET_SIZE - old_pcr) * 8 * 1000 > pcr_repetition_rate * ts_bitrate ) { 
		send_pcr_packet();
		old_pcr = ts_output_bytes;
		emulate_vbv_tick();
	}
	
	/* vbv control: check if the receiver video buffer can receiver the next packet, if it can't output null packets until a frame is flushed  */
	while (vbv_current_size + ((TS_PACKET_SIZE - TS_HEADER_SIZE) * 8) > vbv_max) {
		write(STDOUT_FILENO, null_ts_packet, TS_PACKET_SIZE);
		ts_output_bytes += TS_PACKET_SIZE;
		emulate_vbv_tick();
		if ( (ts_output_bytes + TS_PACKET_SIZE - old_pcr) * 8 * 1000 > pcr_repetition_rate * ts_bitrate ) {
			send_pcr_packet();
			old_pcr = ts_output_bytes;
			emulate_vbv_tick();
		}
	}
	
}

FILE* openStream(char* argv[], int open_counter, FILE* file_pes[]) {

	FILE* result = NULL;
	struct stat file_stat;
	
	/* fprintf(stderr, "pesvideo2ts: opening %s...  ", argv[6 + open_counter]); */
	if (lstat(argv[6 + open_counter], &file_stat) == 0) {
		if (!S_ISFIFO(file_stat.st_mode)) {
			result = fopen(argv[6 + open_counter], "rb");
			file_pes[open_counter] = result;
			/* fprintf(stderr, "open\n"); */
		} else {
			result = file_pes[open_counter];
			/* fprintf(stderr, "is a fifo\n"); */
		}
	}
	
	return result;
}

void  closeStream(char* argv[], int open_counter, FILE* file_pes[]) {

	struct stat file_stat;
	
	/* fprintf(stderr, "pesvideo2ts: closing %s...  ", argv[6 + open_counter]); */
	if (lstat(argv[6 + open_counter], &file_stat) == 0) {
		if (!S_ISFIFO(file_stat.st_mode)) {
			fclose(file_pes[open_counter]);
			/* fprintf(stderr, "closed\n"); */
		} else {
		        /* fprintf(stderr, "is a fifo\n"); */
		}
	} else {
		fclose(file_pes[open_counter]);
		/* fprintf(stderr, "closed but now is missing\n"); */
	}
	
}

int main(int argc, char *argv[])
{
	int i = 0;
	int open_counter = 0;
	int loop_on = 0;
	int byte_read = 0;
	int total_bytes = 0;
	FILE* current_file_pes = NULL;
	pid = MAX_PID;
	FILE* file_pes[MAX_FD];
	memset(file_pes, 0, sizeof(FILE*) * MAX_FD);
	
	if (argc >= 7) {
		pid = atoi(argv[1]);
		frame_rate = atoi(argv[2]);
		if (argv[2][2] == ':') {
			pcr_repetition_rate = atoi(argv[2]+3);
			pcr_repetition_rate -= 1; // stay under
		}
		fprintf(stderr, "frame rate is %d\n", frame_rate);
		if (argv[3][0] == 'b') {
			vbv_max = atoi(argv[3]+1);
		} else {
			vbv_max = atoi(argv[3]) * 16 * 1024; // bits
		}
		fprintf(stderr, "vbv size in bit is %d\n", vbv_max);
		ts_bitrate = atoi(argv[4]);
		loop_on = atoi(argv[5]) > 0;
		file_pes[0] = fopen(argv[6], "rb");
		if (file_pes[0] == NULL) {
			fprintf(stderr, "pesvideo2ts: failed to open %s\n", argv[6]);
			return 0;
		} else {
			snprintf(filename, PATH_MAX, "%s", argv[6]);
			current_file_pes = file_pes[0];
		}
		if (pid >= MAX_PID) {
			fprintf(stderr, "pesvideo2ts: pid has to be smaller than %d\n", MAX_PID);
		}
	}
	
	/* fifo are all open at the begin */
	for (i = 7; i < argc; i++) {
		struct stat file_stat;
		if (lstat(argv[i], &file_stat) == 0) { 
			if (S_ISFIFO(file_stat.st_mode)) {
				file_pes[i - 6] = fopen(argv[i], "rb");
				if (file_pes[i - 6] == NULL) {
					fprintf(stderr, "pesvideo2ts: failed to open %s\n", argv[i]);
					return 0;
				} else {
					fprintf(stderr, "pesvideo2ts: opens also fifo %s\n", argv[i]);
				}
			} else {
				file_pes[i - 6] = NULL;
			}
		} else {
			file_pes[i - 6] = NULL;
		}
	}
	
	if (argc < 7) { 
		fprintf(stderr, "Usage: 'pesvideo2ts pid es_framerate[:pcr_rate] es_video_vbv ts_video_bitrate loop_on input1.pes [input2.pes ... input%d.pes]'\n", MAX_FD);
		fprintf(stderr, " where pid is bounded from 1 to 8191\n");
		fprintf(stderr, " es_framerate is elementary stream frame rate\n");
		fprintf(stderr, " pcr_rate is pcr insertion repetetion in milliseconds\n");
		fprintf(stderr, " es_video_vbv is elementary stream vbv size example: 112 are vbv units (16 * 1024 bit), b1024 is 1024 bit \n");
		fprintf(stderr, " ts_video_bitrate is the output bitrate desired\n");
		fprintf(stderr, " if loop_on is 1 input file are read on loop, works only for mpeg2 video files\n");
		fprintf(stderr, " if the first file of the loop is missing the loop ends\n");
		fprintf(stderr, " more than 1 file is only for mpeg2 videos\n");
		fprintf(stderr, " input files can be changed at runtime if not fifo\n"); /* this limit comes from linux system calls, I am not sure but aix or freebsd won't have it */
		fprintf(stderr, "framerate available are 23, 24, 25, 29, 30, 50, 59 and 60\n");
		fprintf(stderr, "29 is actually 30000÷1001 as from standard and so on\n");
		return 0;
	} 
	
	
	/* Set some init. values */
	pid = htons(pid);
	ts_payload = 0;
	look_ahead_size = 0;
	ts_continuity_counter = 0x0; 
	ts_packet[0] = 0x47; /* sync byte */ 
	memcpy(ts_packet + 1, &pid, 2); /* pid */
	ts_packet[1] |= 0x40; /* payload unit start indicator */
	byte_read = 1;

	/* Init null packet */
	memset(null_ts_packet, 0, TS_PACKET_SIZE);
	null_ts_packet[0] = 0x47;
	null_ts_packet[1] = 0x1F;
	null_ts_packet[2] = 0xFF;
	null_ts_packet[3] = 0x10;

	/* PCR packet setup */
	memset(pcr_ts_packet, 0xFF, TS_PACKET_SIZE);
	pcr_ts_packet[0] = 0x47;
	memcpy(pcr_ts_packet + 1, &pid, 2); /* pid */
	pcr_ts_packet[3] = 0x20; /* only adaptation field, cc starts from zero */
	pcr_ts_packet[4] = 183; /* again because there is only adaptation field */
	pcr_ts_packet[5] = 0x10; /* there is only pcr */
	
	ts_output_bytes = 0;
	
	/* Set the pcr as the first packets helps decoder to learn the new time as soon as possible */
	/* also minimize the distance from a previous pcr */
	send_pcr_packet();
	
	/* Process the PES file */
	byte_read = fread(look_ahead_buffer, 1, PES_HEADER_SIZE, current_file_pes);
	if (byte_read > 0) {
		total_bytes += byte_read;
	}
	es_input_bytes = byte_read;
	look_ahead_size = byte_read;
	last_frame_index = 0;
	int head_missing = 0;
	while (!head_missing) { /* first file, aka head, was complety read and loop mode is off or it was deleted*/
		
		if (look_ahead_size == 0 && loop_on == 0 && byte_read == 0) {
			head_missing = 1;
		}
		
		while ((byte_read || look_ahead_size) && !head_missing) {
		
			/* Fill the look ahead buffer */
			if (look_ahead_size < PES_HEADER_SIZE && current_file_pes != NULL) {
				
				/*fprintf(stderr, "1: look_ahead_size is %d\n", look_ahead_size); */
				byte_read = fread(look_ahead_buffer + look_ahead_size, 1, 1, current_file_pes);
				if (byte_read > 0) {
					total_bytes += byte_read;
				} /* else {
					fprintf(stderr, "pesvideo2ts: warning: processing read was %d, look ahead is %d\n", byte_read, look_ahead_size);
				} */
			
				if (byte_read <= 0 && loop_on) { /* try to get bytes from the next file on the list */
			
					closeStream(argv, open_counter, file_pes);
					current_file_pes = NULL;
					open_counter++;
			
					/* fprintf(stderr, "pesvideo2ts: %s total bytes %d\n", filename, total_bytes);  */
					if (argc >= 7 + open_counter) {
						current_file_pes = openStream(argv, open_counter, file_pes);
						if (current_file_pes != NULL) {
							snprintf(filename, PATH_MAX, "%s", argv[6 + open_counter]);
						} /* else {
							fprintf(stderr, "pesvideo2ts: failed to open %s\n", argv[6 + open_counter]);
						} */
					}
				
					if (current_file_pes == NULL) {
						open_counter = 0;
						current_file_pes = openStream(argv, open_counter, file_pes);
						if (current_file_pes != NULL) {
							snprintf(filename, PATH_MAX, "%s", argv[6 + open_counter]);
						} /* else {
							fprintf(stderr, "pesvideo2ts failed to open %s\n", argv[6 + open_counter]);
						} */
					}
					
					total_bytes = 0;
					if (current_file_pes != NULL) {
						byte_read = fread(look_ahead_buffer + look_ahead_size, 1, 1, current_file_pes);
						if (byte_read > 0) {
							total_bytes += byte_read; 
							first_pts = 1;
						} else {
							/* fprintf(stderr, "pesvideo2ts: restarting read was %d, look ahead is %d\n", byte_read, look_ahead_size); */
							head_missing = 1;
						}
					} else {
						fprintf(stderr, "pesvideo2ts: first file of the loop is missing\n");
						byte_read = 0;
						head_missing = 1;
					}
				}
				look_ahead_size += byte_read;
				es_input_bytes += byte_read; /* it will overflown at 100mbps after countless years */
			}
		
		
			/* PES header detected? */
			/* fprintf(stderr, "look_ahead_size is %d\n", look_ahead_size); */
			if (look_ahead_size == PES_HEADER_SIZE && 
				look_ahead_buffer[0] == 0x00 && 
				look_ahead_buffer[1] == 0x00 && 
				look_ahead_buffer[2] == 0x01 && 
				((look_ahead_buffer[3] == 0xE0) || ((look_ahead_buffer[3] >> 5) == 0x06))) {
				
				/* decrease pts header from elementary stream frame size byte counts */
				/* fprintf(stderr, "es_input_bytes is %llu, byte is %d\n", es_input_bytes, look_ahead_buffer[8]); */
				es_input_bytes -= (9 + look_ahead_buffer[8]);
				
				/* we learned the previous frame size so we enqueue it */
				if (es_input_bytes > 0) {
					enqueue_frame(es_input_bytes - last_frame_index);
					/* fprintf(stderr, "frame %llu size is %llu\n", frames, es_input_bytes - last_frame_index); */
					/* get the output size of the first frame */
					if (first_frame_ts_size == 0) {
						first_frame_ts_size = ts_output_bytes;
						/*
						ts_offset = (ts_output_bytes * 8 * SYSTEM_CLOCK) / ts_bitrate;
						fprintf(stderr, "pts offset is %llu\n", ts_offset);
						*/
					}
				}
				
				if (frames == 0) {
					frames++;
				}
				last_frame_index = es_input_bytes;
				
				/* Send current packet if there's anything ready */
				if (ts_payload) {
					send_current_packet(es_input_bytes - es_input_bytes_old);
					es_input_bytes_old = es_input_bytes;
				}
			
				/* Set pusu for the next packet */
				memcpy(ts_packet + 1, &pid, 2); /* pid */
				ts_packet[1] |= 0x40; /* payload unit start indicator */
			}
			
			/* Fill the current packet */
			if (look_ahead_size > 0 && ((loop_on && byte_read > 0) || (!loop_on)) ) {
			
				/* Move a packet from the lookahead to the current packet */
				ts_packet[TS_HEADER_SIZE + ts_payload] = look_ahead_buffer[0];
				ts_payload++;
				for (i = 0; i < PES_HEADER_SIZE-1; i++){
					look_ahead_buffer[i] = look_ahead_buffer[i + 1];
				}
				look_ahead_size--;
				
				/* Send the packet if it's filled */
				if (TS_HEADER_SIZE + ts_payload == TS_PACKET_SIZE) {
				
					send_current_packet(es_input_bytes - es_input_bytes_old);
					es_input_bytes_old = es_input_bytes;
				
					/* Unset pusu for the next packet */
					memcpy(ts_packet + 1, &pid, 2); /* pid */
				}
			}
			
			/* Send the last packet with the last bytes if any */
			if (byte_read == 0 && look_ahead_size == 0 && ts_payload) {
				send_current_packet(es_input_bytes - es_input_bytes_old);
				es_input_bytes_old = es_input_bytes;
			}
		}
	
		/* if are processing fifo and we are on loop, let's block */
		for (i = 6; i < argc && loop_on; i++) {
			struct stat file_stat;
			if (lstat(argv[i], &file_stat) == 0) { 
				if (S_ISFIFO(file_stat.st_mode)) {
					fclose(file_pes[i - 6]);
				}
			}
		}
		for (i = 6; i < argc && loop_on; i++) {
			struct stat file_stat;
			if (lstat(argv[i], &file_stat) == 0) { 
				if (S_ISFIFO(file_stat.st_mode)) {
					file_pes[i - 6] = fopen(argv[i], "rb");
					if (file_pes[i - 6] == NULL) {
						fprintf(stderr, "pesvideo2ts: failed to re-open %s\n", argv[i]);
						return 0;
					} else {
						fprintf(stderr, "pesvideo2ts: re-opens fifo %s\n", argv[i]);
						if (i == 6) {
							head_missing = 0;
						}
					}
				} else {
					file_pes[i - 6] = NULL;
				}
			} else {
				file_pes[i - 6] = NULL;
			}
		}
		
		current_file_pes = file_pes[0];
		
		if (!head_missing) {
			first_pts = 1;
		}
	}
	
	return 0;
}
