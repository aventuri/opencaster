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
#include <unistd.h>

#define ES_HEADER_SIZE 4 
#define HEADER_MAX_SIZE 64 

/* MPEG VIDEO */
const char* picture_coding[8] = {
				"forbidden", 
				"I-Frame",
				"P-Frame",
				"B-Frame",
				"invalid",
				"reserved",
				"reserved",
				"reserved", 
};

const char* profile[8] = {
				"reserved",
				"High",
				"Spatially Scalable",
				"SNR Scalable",
				"Main",
				"Simple",
				"reserved",
				"reserved",
};


const char* level[16] = {
				"reserved",
				"reserved",
				"reserved",
				"reserved",
				"High",
				"reserved",
				"High 1440",
				"reserved",
				"Main",
				"reserved",
				"Low",
				"reserved",
				"reserved",
				"reserved",
				"reserved",
				"reserved",
};

const char* aspect_ratio[16] = {
				"forbidden",
				"1:1", 
				"4:3", 
				"16:9",
				"2.21:1",
				"reserved",
				"reserved",
				"reserved",
				"reserved",
				"reserved",
				"reserved",
				"reserved",
				"reserved",
				"reserved",
				"reserved",
				"reserved",
};

const char* frame_rate[16] = {
				"forbidden", 
				"23.9", 
				"24", 
				"25",
				"29.9",
				"30",
				"50", 
				"59.9",
				"60",
				"reserved", 
				"reserved",
				"reserved",
				"reserved",
				"reserved",
				"reserved",
				"reserved",
};

const float frame_sec[16] = {
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


int main(int argc, char *argv[])
{
	int byte_read;
	int byte_count;
	int bitrated;
	float frames;
	unsigned int picture_time;
	unsigned char picture_type;
	unsigned int vbv_delay;
	int drop;
	int hour;
	int min;
	int sec;
	int pictures;
	int closed;
	int broken;
	int horizontal_size;
	int vertical_size;
	int aspect_ratioed;
	int frame_rated;
	int max_frame_size;
	int frame_size;
	int vbv_buffer_size;
	int constrained;
	int frame_counter;
	double max_bps;
	double current_bps;
	double average_bps;
	unsigned long int gop_count;
	unsigned long long int total_byte_count;
	unsigned long long int previous_byte_count;
	
	FILE* file_es;
	unsigned char es_header[HEADER_MAX_SIZE];
	
	/* Open es file */
	if (argc > 1) {
		file_es = fopen(argv[1], "rb");
	} else {
		fprintf(stderr, "Example: esvideompeg2info video.es\n");
		return 2;
	}
	if (file_es ==  0) {
		fprintf(stderr, "Can't find file %s\n", argv[1]);
		return 2;
	}
		
	/* Start to process the file */
	byte_count = 0;
	total_byte_count = 0;
	frame_counter = 0;
	gop_count = 0;
	max_bps = 0;
	frame_size = 0;
	max_frame_size = 0;
	previous_byte_count = 0;
	current_bps = 0;
	average_bps = 0;
	byte_read = fread(es_header, 1, ES_HEADER_SIZE, file_es);
	while(byte_read) {

		byte_count += byte_read;
		total_byte_count += byte_read;
		
		/* Search headers */
		if (es_header[0] == 0x00 && es_header[1] == 0x00 && es_header[2] == 0x01 && es_header[3] == 0x00) { /* Picture start header */

			frame_counter++;
			frame_size = total_byte_count - previous_byte_count;
			if (frame_size > max_frame_size) {
				max_frame_size = frame_size;
			}
			previous_byte_count = total_byte_count;
			
			fprintf(stdout, "frame size: %d\n", frame_size);

			fprintf(stdout, "Postion %llu, picture %d start header: ", total_byte_count, frame_counter);
			byte_read = fread(es_header, 1, 4, file_es);
	
			picture_time = (es_header[0] << 2) | ((es_header[1] & 0xC0) >> 6);
			fprintf(stdout, "temporal reference: %u, ", picture_time);
		
			picture_type = (es_header[1] & 0x38) >> 3;
			fprintf(stdout, "picture coding type: %s, ", picture_coding[picture_type]);
		
			vbv_delay = ((es_header[1] & 0x07) << 13 ) | (es_header[2] << 5) | ((es_header[3] & 0xf8) >> 3) ;
			fprintf(stdout, "vbv delay: %u\n", vbv_delay);

			frames++;
			byte_read += fread(es_header, 1, ES_HEADER_SIZE, file_es);

			
		} else if (es_header[0] == 0x00 && es_header[1] == 0x00 && es_header[2] == 0x01 && es_header[3] == 0xB3) { /* Sequence video header */

			byte_read = fread(es_header, 1, 8, file_es);

			horizontal_size = (es_header[0] << 4) | ((es_header[1] & 0xF0) >> 4);
			vertical_size = ((es_header[1] & 0x0F) << 8) | es_header[2];
			aspect_ratioed = (es_header[3] & 0xF0) >> 4;
			frame_rated = (es_header[3] & 0x0F);
			fprintf(stdout, "Sequence header: format: %dx%d, %s, %sfps, ", horizontal_size, vertical_size, aspect_ratio[aspect_ratioed], frame_rate[frame_rated]);
			
			bitrated = ((es_header[4] << 10) | (es_header[5] << 2) | ((es_header[6] & 0xC0) >> 6)) * 400;
			if (bitrated > 1000000) {
				fprintf(stdout, "bitrate: %0.2fMbs, ", (float) bitrated / 1000000.0);
			} else if (bitrated > 1000) {
				fprintf(stdout, "bitrate: %0.2fKbs, ", (float) bitrated / 1000.0);
			} else {
				fprintf(stdout, "bitrate: %dbs, ", bitrated);
			}
			
			vbv_buffer_size = ((es_header[6] & 0x1F) << 5) | ((es_header[7] & 0xF8) >> 3); 
			fprintf(stdout, "vbv buffer size: %d, ", vbv_buffer_size);
			
			constrained = (es_header[7] & 0x04) >> 2;
			fprintf(stdout, "constrained: %s\n", constrained ? "yes":"no");
			
			if (es_header[7] & 0x02) {
			    byte_read += fread(es_header, 1, 64, file_es); /* intra quantiser matrix */
			    if (es_header[63] & 0x01) {
				byte_read += fread(es_header, 1, 64, file_es); /* there is also non-intra quantiser matrix */
			    }
			} else if (es_header[7] & 0x01) {
			    byte_read += fread(es_header, 1, 64, file_es); /* non-intra quantiser matrix */
			}
			
			/* read next start code */
			byte_read += fread(es_header, 1, ES_HEADER_SIZE, file_es);
	
			if (es_header[0] == 0x00 && es_header[1] == 0x00 && es_header[2] == 0x01 && es_header[3] == 0xB5) { /* Sequence extension */
				byte_read += fread(es_header, 1, 6, file_es);
			
				fprintf(stdout, "Sequence header extension: profile is %s, level is %s\n", profile[es_header[0] & 0x07], level[(es_header[1] & 0xF0) >> 4]);
			
				byte_read += fread(es_header, 1, ES_HEADER_SIZE, file_es);
		
			}
		} else if (es_header[0] == 0x00 && es_header[1] == 0x00 && es_header[2] == 0x01 && es_header[3] == 0xB7) { /* Sequence End Code header */		
		
			fprintf(stdout, "Sequence End Code header\n");
			byte_read = fread(es_header, 1, ES_HEADER_SIZE, file_es);
			
		} else if (es_header[0] == 0x00 && es_header[1] == 0x00 && es_header[2] == 0x01 && es_header[3] == 0xB8) { /* GOP video header */		

			gop_count += 1;
			fprintf(stdout, "GOP header: ");
                        if (byte_count > ES_HEADER_SIZE) {
				fprintf(stdout, "measured size: %d bytes, ", byte_count);
				if ((frames * frame_sec[frame_rated]) > 0) {
					current_bps = (byte_count * 8) / (frames * frame_sec[frame_rated]);
				}
				fprintf(stdout, "bitrate from measured size (formula has rounds): %fbps, ", current_bps);
				if (current_bps > max_bps) {
					max_bps = current_bps;
				}
				average_bps = ((average_bps * (gop_count - 1)) + current_bps) / gop_count; 
				byte_count = 0;
				frames = 0;
			}
			
			byte_read = fread(es_header, 1, 4, file_es);
	
			drop = ((es_header[0] & 0x80) > 0);
			fprintf(stdout, "drop: %s, ", drop ? "yes":"no");
	
			hour = ((es_header[0] & 0x7C) >> 2);
			min = ((es_header[0] & 0x3) << 4) | ((es_header[1] & 0xF0) >> 4);
			sec = ((es_header[1] & 0x7) << 3) | ((es_header[2] & 0xE0) >> 5);
			pictures = ((es_header[2] & 0x1F) << 1) | ((es_header[3] & 0x80) >> 7);
			fprintf(stdout, "time code: %02d:%02d:%02d pictures:%02d, ", hour, min, sec, pictures);
	
			closed = ((es_header[3] & 0x40) > 0);
			fprintf(stdout, "closed: %s, ", closed ? "yes":"no");
		
			broken = ((es_header[3] & 0x20) > 0);
			fprintf(stdout, "broken: %s\n", broken ? "yes":"no");
		
			byte_read += fread(es_header, 1, ES_HEADER_SIZE, file_es);
		
		} else {
		
			es_header[0] = es_header[1];
			es_header[1] = es_header[2];
			es_header[2] = es_header[3];
			byte_read = fread(es_header + 3, 1, 1, file_es);
	
		}
	}

//	fprintf(stdout, "maximum bit rate was:%f\n", max_bps);
	fprintf(stdout, "maximum frame size was:%d bytes\n", max_frame_size);
//	fprintf(stdout, "average bit rate is: %f\n", average_bps);
	return 0;
}

