/* 
 * Copyright (C) 2009-2013  Lorenzo Pallara, l.pallara@avalpa.com
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
#include <fcntl.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define MAX_PID 8192
#define PTS_TIME 90000LL
#define PES_HEADER_SIZE 4
#define TS_HEADER_SIZE 4 
#define TS_PACKET_SIZE 188
#define TS_PACKET_BODY (TS_PACKET_SIZE - TS_HEADER_SIZE)

#define SYSTEM_CLOCK_FREQUENCY 27000000
#define PTS_MAX 8589934592LL
#define PACK_HEADER_SIZE 4 
#define TIME_STAMP_SIZE 5
#define MAX_FD 256

const long long unsigned system_frequency = 27000000;

int ts_payload;				/* TS packet payload counter */
u_short pid;				/* Pid for the TS packets */
u_char ts_continuity_counter;		/* Continuity counter */
u_char ts_packet[TS_PACKET_SIZE];	/* TS packet */
u_char look_ahead_buffer[PES_HEADER_SIZE];
u_char look_ahead_size;
unsigned char null_ts_packet[TS_PACKET_SIZE];
unsigned long long int pts_offset = 0LL;
unsigned long long int pts_step = 3600LL;
unsigned long long int pts = 0ll; /* PTS last value */
/* unsigned long long int pts_delta = 0; */ /*PTS delta increment */
unsigned long long int frame_number = 0ll;
unsigned char pts_index = 0; /* PTS index table for the TS packets */
int frame_size = 0;
int pes_frame_size = 0;
int sample_rate = 0;
int sample_per_frame = 1152;
unsigned long long ts_packets = 0;
unsigned long long bitrate = 0;


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

void restamp_and_output(void) {

	unsigned char adapt = 0;
	unsigned char timestamp[TIME_STAMP_SIZE];
	int ts_header_size = 0;
	unsigned long long time = 0;
	
	adapt = (ts_packet[3] >> 4) & 0x03;
	if (adapt == 0) {
		ts_header_size = TS_PACKET_SIZE; /* the packet is invalid ?*/
	} else if (adapt == 1) {
		ts_header_size = TS_HEADER_SIZE; /* only payload */
	} else if (adapt == 2) { 
		ts_header_size = TS_PACKET_SIZE; /* only adaptation field */
	} else if (adapt == 3) {
		ts_header_size = TS_HEADER_SIZE + ts_packet[4] + 1; /* jump the adaptation field */
	} else {
		ts_header_size = TS_PACKET_SIZE; /* not managed */
	}
	
	if (ts_header_size < TS_PACKET_SIZE - 9 - TIME_STAMP_SIZE) {
		if ((ts_packet[ts_header_size] == 0x00) && (ts_packet[ts_header_size + 1] == 0x00) && (ts_packet[ts_header_size + 2] == 0x01) && ((ts_packet[ts_header_size + 3] >> 5) == 0x06)) {
			if (pes_frame_size == (ts_packet[ts_header_size + 4] << 8 | ts_packet[ts_header_size + 5])) {
				memcpy(timestamp, ts_packet + ts_header_size + 9, TIME_STAMP_SIZE);
				time = parse_timestamp(timestamp);
				/* fprintf(stderr, "Pid %d old Audio Presentation Time Stamp is: %llu, %llu.%04llu sec.\n", pid, time,  time / (system_frequency / 300), (time % (system_frequency / 300)) / ((system_frequency / 300) / 10000));  */
				if (pts_index == 0) {
					pts_index = 1;
					memcpy(timestamp, ts_packet + ts_header_size + 9, TIME_STAMP_SIZE);
					time = parse_timestamp(timestamp);
					pts = time;
					frame_number = 1;
				} else if (pts_index == 1) {
					pts_index = 2;
					memcpy(timestamp, ts_packet + ts_header_size + 9, TIME_STAMP_SIZE);
					time = parse_timestamp(timestamp);
					pts = time;
					frame_number = 2;
				} else {
					pts = pts_offset + ((frame_number * sample_per_frame * PTS_TIME) / sample_rate);
					stamp_ts(pts % PTS_MAX, ts_packet + ts_header_size + 9);
					frame_number++;
					ts_packet[ts_header_size + 9] &= 0x0F; 
					ts_packet[ts_header_size + 9] |= 0x20;
				}
				memcpy(timestamp, ts_packet + ts_header_size + 9, TIME_STAMP_SIZE);
				time = parse_timestamp(timestamp);
				/* fprintf(stderr, "Pid %d new Audio Presentation Time Stamp is: %llu, %llu.%04llu sec.\n", pid, time,  time / (system_frequency / 300), (time % (system_frequency / 300)) / ((system_frequency / 300) / 10000)); */
			} else {
				/* fprintf(stderr, "pesaudio2ts warning: error in audio payload\n");  */
			}
		}
	}
	
	write(STDOUT_FILENO, ts_packet, TS_PACKET_SIZE);
	ts_packets++;
}

void send_current_packet(void) {

	int i;
	u_char temp;

	if (TS_HEADER_SIZE + ts_payload == TS_PACKET_SIZE) { /* filled case */
	
		ts_packet[3] = ts_continuity_counter | 0x10; /* continuity counter, no scrambling, only payload */
		ts_continuity_counter = (ts_continuity_counter + 1) % 0x10; /* inc. continuity counter */
		restamp_and_output();
		ts_payload = 0;
		
	} else if (TS_HEADER_SIZE + ts_payload + 1 == TS_PACKET_SIZE) { /* payload too big: two packets are necessary */
	
		temp = ts_packet[TS_HEADER_SIZE + ts_payload - 1]; /* copy the exceeding byte */ 
		ts_payload--;
		send_current_packet();
		
		memcpy(ts_packet + 1, &pid, 2); /* pid, no pusu */
		ts_packet[4] = temp;
		ts_payload = 1;
		send_current_packet();
		
	} else { /* padding is necessary */
	
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
		restamp_and_output();
		ts_payload = 0;
	}

}

FILE* openStream(char* argv[], int open_counter, FILE* file_pes[]) {

	FILE* result = NULL;
	struct stat file_stat;

	/* fprintf(stderr, "pesaudio2ts: opening %s...  ", argv[6 + open_counter]); */
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

	/* fprintf(stderr, "pesaudio2ts: closing %s...  ", argv[6 + open_counter]); */
	if (lstat(argv[6 + open_counter], &file_stat) == 0) {
		if (!S_ISFIFO(file_stat.st_mode)) {
			fclose(file_pes[open_counter]);
			/* fprintf(stderr, "closed\n"); */
		} else {
			/* fprintf(stderr, "is fifo\n"); */
		}
	} else {
		fclose(file_pes[open_counter]);
		/* fprintf(stderr, "closed but now is missing\n"); */
	}
	
	
}

int main(int argc, char *argv[])
{

	int i;	
	int loop_on = 0;
	int byte_read;
	int total_bytes = 0; 
	unsigned long long left_packet = 0;
	int open_counter = 0;
	char filelength[PATH_MAX];
	FILE* file_pes[MAX_FD];
	FILE* current_file_pes;
	memset(file_pes, 0, sizeof(FILE*) * MAX_FD);
	
	/*  Parse args */
	pid = MAX_PID;
	current_file_pes = 0;
	if (argc >= 7) {
		pid = atoi(argv[1]);
		sample_per_frame = atoi(argv[2]);
		sample_rate = atoi(argv[3]);
		frame_size = atoi(argv[4]); /* es frame size */
		if (argv[4][2] == ':') {
			pts_step = atoi(argv[4]+3);
		}
		loop_on = atoi(argv[5]) > 0;
		current_file_pes = fopen(argv[6], "r");		
		file_pes[0] = current_file_pes;
		snprintf(filelength, PATH_MAX, "%s.length", argv[6]);		
		if (current_file_pes == NULL) {
			/* fprintf(stderr, "pesaudio2ts: failed to open file %s\n", argv[6]); */
			return 0;
		}
		if (pid >= MAX_PID) {
			/* fprintf(stderr, "pesaudio2ts: pid has to be smaller than %d\n", MAX_PID); */
			return 0;
		} else {
			pid = htons(pid);
		}
	} 
	pts_offset = pts_step;
	
	/* fifo are all open at the begin */
	for (i = 7; i < argc; i++) {
		struct stat file_stat;
		if (lstat(argv[i], &file_stat) == 0) { /* take stats of the real file not of the symbolic link */
			if (S_ISFIFO(file_stat.st_mode)) {
				file_pes[i - 6] = fopen(argv[i], "rb");
				if (file_pes[i - 6] == NULL) {
					/* fprintf(stderr, "pesaudio2ts: failed to open %s\n", argv[i]); */
					return 0;
				} else {
					/* fprintf(stderr, "pesaudio2ts: opens also fifo %s\n", argv[i]); */
				}
			} else {
				file_pes[i - 6] = NULL;
			}
		} else {
			file_pes[i - 6] = NULL;
		}
	}
	
	if (argc < 6) {
		fprintf(stderr, "Usage: 'pesaudio2ts pid sample_per_frame sample_rate es_frame_size[:video_pts_step] loop_on input1.pes [input2.pes ... ]', where pid is bounded from 1 to 8191\n");
		fprintf(stderr, "if loop_on is 1 after the last input.pes will start again from the first, video synch works only with mpeg2 video files\n");
		fprintf(stderr, "loop_on ends if the first file is missing\n");
		fprintf(stderr, "input*.pes.length is scan for a pts value, if present the value is used to adjust single pes length adding null packets to sync at that pts\n");
		return 0;
	}
	
	/* Init null packet */
	memset(null_ts_packet, 0, TS_PACKET_SIZE);
	null_ts_packet[0] = 0x47;
	null_ts_packet[1] = 0x1F;
	null_ts_packet[2] = 0xFF;
	null_ts_packet[3] = 0x10;
	
	/* Need to output some null packets to synch to the first pts */
	pes_frame_size = frame_size + 8; /* only payload */
	frame_size += 14; /* pes header */
	frame_size = ((frame_size / TS_PACKET_BODY) + ((frame_size % TS_PACKET_BODY) != 0) + ((frame_size % TS_PACKET_BODY) == 183)) * TS_PACKET_SIZE; /* ts encapsulation */
	bitrate = sample_rate * frame_size * 8 / sample_per_frame;
	/* fprintf(stderr, "pesaudio2ts: ts bit rate is %llu bps\n", bitrate); */
	int packets_to_start =  (pts_offset * bitrate) / (PTS_TIME * 8 * TS_PACKET_SIZE);/* packets to pts */
	packets_to_start -= (frame_size / TS_PACKET_SIZE) + 1 ; /* the first audio frame needs to be there at pts */
	/* fprintf(stderr, "output %d null packets to sync\n", packets_to_start); */
	i = 0;
	for (i = 0; i < packets_to_start; i++) {
		write(STDOUT_FILENO, null_ts_packet, TS_PACKET_SIZE);
		ts_packets++;
	}
	
	/* Set some init. values */
	left_packet = 0;
	ts_payload = 0;
	look_ahead_size = 0;
	ts_continuity_counter = 0x0; 
	ts_packet[0] = 0x47; /* sync byte */ 
	memcpy(ts_packet + 1, &pid, 2); /* pid */
	ts_packet[1] |= 0x40; /* payload unit start indicator */
	byte_read = 1;
	
	/* Process the PES file */
	byte_read = fread(look_ahead_buffer + look_ahead_size, 1, PES_HEADER_SIZE, current_file_pes);
	if (byte_read > 0) {
		total_bytes += byte_read;
	} else if (byte_read == 0){
		/* fprintf(stderr, "pesaudio2ts: audio read was zero at the start\n"); */
	}

	look_ahead_size = byte_read;
	
	int head_missing = 0;
	while (!head_missing) {
		while ((byte_read || look_ahead_size) && !head_missing) {
		
			/* Fill the look ahead buffer */
			if (look_ahead_size < PES_HEADER_SIZE && current_file_pes != NULL) {
				byte_read = fread(look_ahead_buffer + look_ahead_size, 1, 1, current_file_pes);
				
				if (byte_read > 0) {
					total_bytes += byte_read;
				} else {
					/* fprintf(stderr, "pesaudio2ts: audio read was %d in the processing, look_ahead is %d\n", byte_read, look_ahead_size);  */
				}
			
				if (byte_read <= 0 && loop_on) {
					closeStream(argv, open_counter, file_pes);
					current_file_pes = NULL;
					open_counter++;
					/* fprintf(stderr, "pesaudio2ts %s total bytes was %d\n", filelength, total_bytes); */
					total_bytes = 0;
				
					/* it is necesseary to set next pts time to a multiple of pts frame rate, frame rate is assumed 25fps */
					/* it also assume audio ends before video */
					unsigned long long next_pts = pts_offset + ((frame_number * sample_per_frame * PTS_TIME) / sample_rate);
					unsigned long long next_pts_rounded_to_next_video_frame = 0;
				
					/* NB. this is the pes length of the file just closed */
					/* fprintf(stderr, "Audio: looking for end file %s\n", filelength);  */
					FILE* file = fopen(filelength, "r");
					if (file != 0) {
						fscanf (file, "%llu", &next_pts_rounded_to_next_video_frame);
						fclose(file);
						next_pts_rounded_to_next_video_frame += pts_offset;
					} else {
						/* fprintf(stderr, "pesaudio2ts warning: missed %s file\n", filelength); */
						next_pts_rounded_to_next_video_frame = ((next_pts / pts_step) + 1) * pts_step;
					}
				
					if (argc >= 6 + open_counter) {
						current_file_pes = openStream(argv, open_counter, file_pes);
						if (current_file_pes == NULL) {
							/* fprintf(stderr, "pesaudio2ts: failed to open %s\n", argv[6 + open_counter]); */
						} else {
							snprintf(filelength, PATH_MAX, "%s.length", argv[6 + open_counter]);
						}
					}
				
					if (current_file_pes == NULL) {
						open_counter = 0;
						current_file_pes = openStream(argv, open_counter, file_pes);
						if (current_file_pes == NULL) {
							/* fprintf(stderr, "pesaudio2ts: failed to open %s\n", argv[6 + open_counter]); */
						} else {
							snprintf(filelength, PATH_MAX, "%s.length", argv[6]);
						}
					}
				
					if (current_file_pes != NULL) {
						fprintf(stderr, "pesaudio2ts sync: %s new presented audio frame will be at %llu, %llu.%04llu sec., last presented audio frame was at %llu, %llu.%04llu sec.\n",
						argv[6 + open_counter],
						next_pts_rounded_to_next_video_frame % PTS_MAX, 
						(next_pts_rounded_to_next_video_frame % PTS_MAX) / PTS_TIME, ((next_pts_rounded_to_next_video_frame % PTS_MAX) % PTS_TIME) / (PTS_TIME / 10000),
						next_pts % PTS_MAX,
						(next_pts % PTS_MAX) / PTS_TIME, ((next_pts % PTS_MAX) % PTS_TIME) / (PTS_TIME / 10000));
					}
				
					/*
					int null_packets =  (((next_pts_rounded_to_next_video_frame - next_pts) * bitrate) + left_packet) / (PTS_TIME * 8 * TS_PACKET_SIZE);
					left_packet = ((next_pts_rounded_to_next_video_frame - next_pts) * bitrate + left_packet) % (PTS_TIME * 8 * TS_PACKET_SIZE);
					fprintf(stderr, "need to output %d null packets to sync\n", null_packets);
					fprintf(stderr, "left %llu byte left to next round to sync\n", left_packet);
					*/
				
					/* expected overflown after 6.4 years at 1mbps */
					int null_packets =  (left_packet + (next_pts_rounded_to_next_video_frame * bitrate)) / (PTS_TIME * 8 * TS_PACKET_SIZE); /* packets to next pts */
					null_packets -= ts_packets; /* packets sent */
					null_packets -= (frame_size / TS_PACKET_SIZE) + 1 ; /* + 1 the first audio frame packets needs to be there at the next pts */
					/* null_packets -= 1 ; // the first audio frame packet needs to be there at the next pts */
					left_packet = (left_packet + (next_pts_rounded_to_next_video_frame * bitrate)) % (PTS_TIME * 8 * TS_PACKET_SIZE);
				
					/* fprintf(stderr, "need to output %d null packets to sync\n", null_packets); */
					int i = 0;
					/* fprintf(stderr, "sending on output %d null packets, frame size is %d packets, left packet is %llu\n", null_packets, (frame_size / TS_PACKET_SIZE), left_packet);  */
					for (i = 0; i < null_packets; i++) {
						write(STDOUT_FILENO, null_ts_packet, TS_PACKET_SIZE);
						ts_packets++;
					}
					/* fprintf(stderr, "sent on output %d null packets, frame size is %d packets, left packet is %llu\n", null_packets, (frame_size / TS_PACKET_SIZE), left_packet); */
				
					if (current_file_pes != NULL) {
						byte_read = fread(look_ahead_buffer + look_ahead_size, 1, 1, current_file_pes);
						if (byte_read > 0) {
							total_bytes += byte_read; 
							frame_number = 0;
							pts_offset = next_pts_rounded_to_next_video_frame;
						} else {
							fprintf(stderr, "pesaudio2ts: audio read was %d at the begin, look ahead is %d\n", byte_read, look_ahead_size);
							head_missing = 1;
						}
					} else {
						byte_read = 0;
						fprintf(stderr, "pesaudio2ts: first file of the loop is missing\n");
						head_missing = 1;
					}
				}
				look_ahead_size += byte_read;
			}
		
			/* PES header detected? */
			if (look_ahead_size == PES_HEADER_SIZE && 
				look_ahead_buffer[0] == 0x00 && 
				look_ahead_buffer[1] == 0x00 && 
				look_ahead_buffer[2] == 0x01 && 
				look_ahead_buffer[3] == 0xC0 ) { 
//				(((look_ahead_buffer[3] >> 4) == 0x0E) || ((look_ahead_buffer[3] >> 5) == 0x06))) { 
				/* Send current packet if there's anything ready */
				if (ts_payload) {
					send_current_packet();
				}
				/* Set pusu for the next packet */
				memcpy(ts_packet + 1, &pid, 2); /* pid */
				ts_packet[1] |= 0x40; /* payload unit start indicator */
			}
	
			/* Fill the current packet */
			if (look_ahead_size > 0 && ((!loop_on) || (loop_on && byte_read > 0))) {
				/* Move a packet from the lookahead to the current packet */
				ts_packet[TS_HEADER_SIZE + ts_payload] = look_ahead_buffer[0];
				ts_payload++;
				for (i = 0; i < PES_HEADER_SIZE-1; i++){
					look_ahead_buffer[i] = look_ahead_buffer[i + 1];
				}
				look_ahead_size--;
		
				/* Send the packet if it's filled */
				if (TS_HEADER_SIZE + ts_payload == TS_PACKET_SIZE) {
					
					send_current_packet();
					
					/* Unset pusu for the next packet */
					memcpy(ts_packet + 1, &pid, 2); /* pid */
				}
			}
	
			/* Send the last packet with the last bytes if any */
			if (byte_read == 0 && look_ahead_size == 0 && ts_payload) {
				send_current_packet();
			} 
		
		}
		
		/* try to re-open and block if fifo and on-loop */
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
			if (lstat(argv[i], &file_stat) == 0) { /* take stats of the real file not of the symbolic link */
				if (S_ISFIFO(file_stat.st_mode)) {
					file_pes[i - 6] = fopen(argv[i], "rb");
					if (file_pes[i - 6] == NULL) {
						/* fprintf(stderr, "pesaudio2ts: failed to open %s\n", argv[i]); */
						return 0;
					} else {
						/* fprintf(stderr, "pesaudio2ts: re-opens fifo %s\n", argv[i]); */
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
		
		if (!loop_on) {
			head_missing = 1;
		}
		
		current_file_pes = file_pes[0];
		
		/*
		if (!head_missing) {
			byte_read = fread(look_ahead_buffer + look_ahead_size, 1, PES_HEADER_SIZE, current_file_pes);
			if (byte_read > 0) {
				total_bytes += byte_read;
				look_ahead_size = byte_read;
			}
		}
		*/
	
	}
	return 0;
}
