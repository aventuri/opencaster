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
#include <inttypes.h>

#define ES_HEADER_SIZE 4 
#define HEADER_MAX_SIZE 64 

const int rmax [4][5] = {
		{ -1,  80000000,  -1,  -1, 100000000}, /* Mbps */
		{ -1,  60000000,  -1,  60000000,  80000000},
		{ 15000000,  15000000,  15000000,  -1,  20000000},
		{ -1,   4000000,   4000000,  -1,  -1},
};

const int profile[8] = {
				-1,
				4, /* High */
				3, /* Spatially Scalable */
				2, /* SNR Scalable */
				1, /* Main */	
				0, /* Low */
				-1,
				-1,
};

const int level[16] = {
				-1,
				-1,
				-1,
				-1,
				0, /* High */
				-1,
				1, /* High 1440 */
				-1,
				2, /* Main */
				-1,
				3, /* Low */
				-1,
				-1,
				-1,
				-1,
				-1,
};

const double frame_sec[16] = {
				1.0, 
				1/23.9,
				1/24.0,
				1/25.0,
				1/29.9,
				1/30.0,
				1/50.0,
				1/59.9,
				1/60.0,
				1.0,
				1.0,
				1.0,
				1.0,
				1.0,
				1.0,
				1.0,
};

typedef struct queue {
	long long int		index;
	int			value;
	struct queue*		next;
} queue;

queue* g_queue = 0;

int pop_frame(void){

	int result;
	queue* temp;
	
	if (g_queue) {
		result = g_queue->value;
		temp = g_queue->next;
		free(g_queue);
		g_queue = temp;
	} else {
		result = 0;
	}

	return result;
}

void enqueue_frame(long long int index, int value) {
    
	queue* new_queue;
	queue* temp;
	
	new_queue = (queue*) malloc(sizeof(queue));
	new_queue->index = index;
	new_queue->value = value;
	new_queue->next = 0;
	if (g_queue == 0) {					/* unique element */ 
		g_queue = new_queue; 
	} else if (g_queue->index > new_queue->index) {		/* smallest element */
		new_queue->next = g_queue;
		g_queue = new_queue;
	} else {						/* other element */
		temp = g_queue;
		while ((temp->next != 0) && (temp->next->index < new_queue->index)) {
			temp = temp->next;
		}
		new_queue->next = temp->next;
		temp->next = new_queue;
	}
}

int main(int argc, char *argv[]) {

	double flush_delta_time 	= 0.;		/* mesaure time between 2 buffer flushes */
	double es_frame_rate		= -1.;		/* es frame rate */
	int temporal_reference		= -1;		/* temporal_reference of the current frame into the current GOP */
	int frame_size			= 0;		/* current frame size */
	int error_reported		= 0;		/* set if error is already reported for this frame */
	int error_counter		= 0;		/* count errors */
	long int total_frames		= 0L;		/* frames counter */	
	long int popped_frames		= 0L;		/* popped_frames */	
	long int flush_delta_byte	= 0L;		/* delta byte counter between 2 buffer flushes */
	int GOP_size			= 0;		/* current GOP size */
	int GOP_frames			= 0;		/* current GOP number of frames */
	int es_profile			= -1;		/* es profile */
	int es_level			= -1;		/* es level */
	int leakmethod			= -1;		/* leak or vbv_dealy ?*/
	double rbx 			= -1;		/* isntantaneous bit rate */
	double res 			= -1;		/* nominal bit rate */
	double real_rbx 		= -1;		/* real bit rate from GOP size measure */
	FILE* file_es			= 0;		/* es input */
	FILE* file_output		= 0;		/* data output */
	int byte_read			= 0;		/* byte read at every cycle */
	int vbv_delay			= -1;		/* vide buffer verifier delay */
	int vbv_buffer_size		= -1;		/* size of the video buffer verifier in bit */
	int vbv_current_size		= 0;		/* current fillness of the video buffer verifier in bit */
	unsigned char es_header[HEADER_MAX_SIZE];	/* es header parsing buffer */
	
	/*  Parse args */
	file_es = 0;
	if (argc > 1) {
		file_es = fopen(argv[1], "rb");
		file_output = fopen("vbvData.dat", "w+");
	} 
	if (file_es == 0 ) { 
		fprintf(stderr, "Usage: 'vbv videofile.es'\n");
		return 2;
	}
	
	/* Process ES file */
	byte_read = fread(es_header, 1, ES_HEADER_SIZE, file_es);
	while(byte_read) {

		/* Update counters */	
		GOP_size += byte_read;
		frame_size += byte_read;
		vbv_current_size += byte_read * 8; 
		flush_delta_byte += byte_read;
		
		/* Get the current time and flush the buffer if necessary */
		if (rbx != -1) {
			flush_delta_time = flush_delta_byte * 8 / rbx;
			if (flush_delta_time >= es_frame_rate) { 
				vbv_current_size -= pop_frame() * 8;
				fprintf(file_output, "%ld\t%d\n", popped_frames, vbv_current_size / 1000);
				popped_frames++;
				error_reported = 0;
				flush_delta_byte = (flush_delta_time - es_frame_rate) * rbx / 8;
			}
		}
		
		/* Check the buffer status */
		if (leakmethod != -1 && vbv_buffer_size != -1) {
			if (vbv_current_size > vbv_buffer_size) {
				if (error_reported == 0) {
					fprintf(stdout, "Overrun at frame %ld\n", total_frames);
					error_counter++;
				}
				vbv_current_size = vbv_buffer_size;
				error_reported = 1;
			} else if (vbv_current_size < 0) {
				if (error_reported == 0) {
					fprintf(stdout, "Underrun at frame %ld\n", total_frames);
					error_counter++;
				}
				vbv_current_size = 0;
				error_reported = 1;
			}
		}
		
		/* Check headers */
		if (es_header[0] == 0x00 && es_header[1] == 0x00 && es_header[2] == 0x01 && es_header[3] == 0x00) { /* Picture start header */

			if (total_frames > 0) {
				frame_size -= ES_HEADER_SIZE; 
				enqueue_frame(total_frames + temporal_reference, frame_size); 
				error_reported = 0;
				/* 
				 * I know, I know, tempral reference sometimes there is not, 
				 * complain with yout encoder or write field/frame interlaced/progressive management ;-) 
				 *
				 */
				frame_size = ES_HEADER_SIZE;
			}
			
			byte_read = fread(es_header, 1, 4, file_es);
                        temporal_reference = (es_header[0] << 2) | ((es_header[1] & 0xC0) >> 6);
			vbv_delay = ((es_header[1] & 0x07) << 13 ) | (es_header[2] << 5) | ((es_header[3] & 0xf8) >> 3) ;
			if (vbv_delay == 0xFFFF) { 
				/* rbx is fixed */
				if (es_profile != -1 && es_level != -1 && res != -1 ) {
					if (es_level == 2 || es_level == 3) { /* ML or LL */
						rbx = rmax[es_level][es_profile];
					} else { /* HL or H-14 */
						rbx = rmax[es_level][es_profile];
						if (rbx < 1.05 * res)
							rbx = 1.05 * res;
					}
					leakmethod = 1;
				}
			} else { 
				if (es_profile != -1 && es_level != -1 && res != -1 ) {
					rbx = res; 
					/*
					 *
					 * rbx should be a linear interpolation between bytes and time passed and vbv_delay value as
					 * shown in 13818-2 annex C, as a matter of fact encoders make it plain simple compliant to the nominal 
					 * bit rate and the buffer fillness to be standard without giving any futher real bit rate hint.
					 *
					 *
					 */
					if (rbx > rmax[es_level][es_profile]) {
						rbx = rmax[es_level][es_profile] ;
					}
					leakmethod = 0;
				}
			}
			GOP_frames++;
			total_frames++;
			
			byte_read += fread(es_header, 1, ES_HEADER_SIZE, file_es);
			
		} else if (es_header[0] == 0x00 && es_header[1] == 0x00 && es_header[2] == 0x01 && es_header[3] == 0xB3) { /* Sequence video header */

			byte_read = fread(es_header, 1, 8, file_es);

			es_frame_rate = frame_sec[(es_header[3] & 0x0F)];
				
			res = ((es_header[4] << 10) | (es_header[5] << 2) | ((es_header[6] & 0xC0) >> 6)) * 400;
			if (rbx == -1) {
				rbx = res; /* best guess until now and maybe the only one ... */
			}
			vbv_buffer_size = 16 * 1024 * (((es_header[6] & 0x1F) << 5) | ((es_header[7] & 0xF8) >> 3));
			if (es_header[7] & 0x02) {
				byte_read += fread(es_header, 1, 64, file_es); /* intra quantiser matrix */
				if (es_header[63] & 0x01) {
					byte_read += fread(es_header, 1, 64, file_es); /* there is also non-intra quantiser matrix */
				}
			} else if (es_header[7] & 0x01) {
				byte_read += fread(es_header, 1, 64, file_es); /* non-intra quantiser matrix */
			}
			
			byte_read += fread(es_header, 1, ES_HEADER_SIZE, file_es);
	
			if (es_header[0] == 0x00 && es_header[1] == 0x00 && es_header[2] == 0x01 && es_header[3] == 0xB5) { /* Sequence extension */
				byte_read += fread(es_header, 1, 6, file_es);
			
				es_profile = profile[es_header[0] & 0x07];
				es_level = level[(es_header[1] & 0xF0) >> 4];
			
				byte_read += fread(es_header, 1, ES_HEADER_SIZE, file_es);
			}

		} else if (es_header[0] == 0x00 && es_header[1] == 0x00 && es_header[2] == 0x01 && es_header[3] == 0xB8) { /* GOP video header */		

                        if (GOP_size > ES_HEADER_SIZE) { /* it's the second header so we have a GOP between them */
				if (es_frame_rate != -1) {
					real_rbx = (GOP_size * 8) / (GOP_frames * es_frame_rate);
				}
				GOP_size = 0;
				GOP_frames = 0;
			}
			byte_read = fread(es_header, 1, ES_HEADER_SIZE, file_es);
		
		} else { /* 1 byte step */
		
			es_header[0] = es_header[1];
			es_header[1] = es_header[2];
			es_header[2] = es_header[3];
			byte_read = fread(es_header + 3, 1, 1, file_es);
		}
	}
	
	fprintf(stdout, "check done, found %d errors.\n", error_counter);
	fclose(file_output);

	return 0;
}

