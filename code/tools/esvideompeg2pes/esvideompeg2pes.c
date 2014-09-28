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

#define PES_HEADER_SIZE 19
#define SYSTEM_CLOCK 90000.0
#define ES_HEADER_SIZE 4
#define ES_HEADER_MAX_SIZE ES_HEADER_SIZE * 2
#define HEADER_BUFFER_MAX_SIZE 1024
#define PTS_MAX 8589934592LL

const float frame_rate[16] = {
				-1.0, 
				23.9, 
				24.0, 
				25.0,
				29.9,
				30.0,
				50.0, 
				59.9,
				60.0,
				-1.0, 
				-1.0, 
				-1.0,
				-1.0,
				-1.0,
				-1.0,
				-1.0,
};

const int pts_steps[16] = {
				-1, 
				-1, // not supported
				3750, 
				3600,
				3003,
				3000,
				1800, 
				-1, // not supported
				1500,
				-1, 
				-1, 
				-1,
				-1,
				-1,
				-1,
				-1,
};


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

int main(int argc, char *argv[])
{
	int bframe;
	int read_fps;
	int skip_next_header;
	int bitrate;
	int firstframe;
	int byte_read;
	FILE* file_es;
	
	unsigned int time_reference = 0;
	unsigned long long int dts;
	unsigned long long int dts_base;
	unsigned long long int pts;
	unsigned long long int pts_limit = 0ll;
	unsigned long long int pts_first_gop = 0ll;
	unsigned long long int pts_a_gop = 0ll;
	unsigned long long int frame_counter = 0;
	unsigned char stream_id = 224;
	unsigned char pes_header[PES_HEADER_SIZE];
	unsigned char es_header[ES_HEADER_SIZE  + 8];
	unsigned int pts_step = 0;
	unsigned char header_buffer[HEADER_BUFFER_MAX_SIZE];
	unsigned int header_buffer_size = 0;
	
	/* Open pes file */
	if (argc > 1) {
		file_es = fopen(argv[1], "rb");
	}

	if (argc > 2) {
		pts_limit = atol(argv[2]);
	} 
		
	if (argc > 3) {
		stream_id = atoi(argv[3]);
	} 

	if (argc < 2){
		fprintf(stderr, "Usage: 'esvideompeg2pes video.es [pts_limit] [stream_id]] '\n");
		fprintf(stderr, "Video stream id default is 224\n");
		return 2;
	}
	
	if (file_es == 0) {
		fprintf(stderr, "Can't find file %s\n", argv[1]);
		return 2;
	}

	/* Init. default pack header */
	pes_header[0] = 0x00;
	pes_header[1] = 0x00;
	pes_header[2] = 0x01;
	pes_header[3] = stream_id;
	pes_header[4] = 0x00;
	pes_header[5] = 0x00;
	pes_header[6] = 0x80; /* no scrambling, no priority, no alignment defined, no copyright, no copy */
	pes_header[7] = 0xC0; /* pts and dts, no escr, no es rate, no dsm trick, no extension flag, no additional copy info, no crc flag */
	pes_header[8] = 0x0A; /* pts and dts size*/ 

	/* Start the process */

	bframe = 1;
	firstframe = 1;
	skip_next_header = 0;
	read_fps = 0;
	
	/* get stream info, output first pes header */
	byte_read = fread(es_header, 1, ES_HEADER_SIZE, file_es);
	if (es_header[0] == 0x00 && es_header[1] == 0x00 && es_header[2] == 0x01 && es_header[3] == 0xB3) { 
					
		byte_read = fread(es_header + ES_HEADER_SIZE, 1, 8, file_es);
		
		if ((es_header[ES_HEADER_SIZE + 3] & 0x0F) < 16) {
			if (frame_rate[es_header[ES_HEADER_SIZE + 3] & 0x0F] < 0.0) {
				fprintf(stderr, "invalid fps into sequence video header\n");
				exit(2);
			} else {
				pts_step = pts_steps[es_header[ES_HEADER_SIZE + 3] & 0x0F];
				if (pts_step < 0) {
					fprintf(stderr, "%f fps are not supported\n", frame_rate[es_header[ES_HEADER_SIZE + 3] & 0x0F]);
				} else {
					fprintf(stderr, "pts step is %d\n", pts_step);
				}
			}
		} else {
			fprintf(stderr, "invalid fps into sequence video header\n");
			exit(2);
		}
		bitrate = ((es_header[ES_HEADER_SIZE + 4] << 10) | (es_header[ES_HEADER_SIZE + 5] << 2) | ((es_header[ES_HEADER_SIZE + 6] & 0xC0) >> 6)) * 400;
		fprintf(stderr, "bit per second are: %d\n", bitrate);
		
		dts = 0;
		dts_base = 0;
		pts = pts_step;

		pes_header[7] = 0xC0; /* pts and dts, no escr, no es rate, no dsm trick, no extension flag, no additional copy info, no crc flag */
		pes_header[8] = 0x0A; /* pts and dts size*/ 		
		stamp_ts(pts % PTS_MAX, pes_header + 9);
		pes_header[9] &= 0x0F;
		pes_header[9] |= 0x30;
		stamp_ts(dts % PTS_MAX, pes_header + 14);
		pes_header[14] &= 0x0F;
		pes_header[14] |= 0x10;

		fwrite(pes_header, 1, PES_HEADER_SIZE, stdout);
		frame_counter++;

		fwrite(es_header, 1, ES_HEADER_SIZE + 8, stdout);
		byte_read = fread(es_header, 1, ES_HEADER_SIZE, file_es);
		
	} else  {
		fprintf(stderr, "elementary stream video should start with a sequence video header\n");
		exit(2);
	}	
	
	/* skip nex start header */
	while(byte_read) {
		
		if (es_header[0] == 0x00 && es_header[1] == 0x00 && es_header[2] == 0x01 && es_header[3] == 0x00) { 
		
			/* pes header already written, just count dts */
			fwrite(es_header, 1, ES_HEADER_SIZE, stdout);
			dts += pts_step;
			byte_read = 0;
		
		} else {
		
			fwrite(es_header, 1, 1, stdout);
			es_header[0] = es_header[1];
			es_header[1] = es_header[2];
			es_header[2] = es_header[3];
			byte_read = fread(es_header + 3, 1, 1, file_es);
			
		}
		
	}

	byte_read = fread(es_header, 1, ES_HEADER_SIZE, file_es);
	while(byte_read) {
		
		if (es_header[0] == 0x00 && es_header[1] == 0x00 && es_header[2] == 0x01 && es_header[3] == 0x00) { 
		    
			fread(es_header + ES_HEADER_SIZE, 1, ES_HEADER_SIZE, file_es);
			
			time_reference = (es_header[4] << 2) | ((es_header[5] & 0xC0) >> 6 );
			bframe = (es_header[5] & 0x38) >> 3;
			bframe = (bframe == 0x03);
			
			pts = dts_base + (time_reference * pts_step) +  pts_step; 
			if (!bframe) {
				pes_header[7] = 0xC0; /* pts and dts, no escr, no es rate, no dsm trick, no extension flag, no additional copy info, no crc flag */
				pes_header[8] = 0x0A; /* pts and dts size*/ 
				stamp_ts(pts % PTS_MAX, pes_header + 9);
				pes_header[9] &= 0x0F;
				pes_header[9] |= 0x30;
				stamp_ts(dts % PTS_MAX, pes_header + 14);
				pes_header[14] &= 0x0F;
				pes_header[14] |= 0x10;
				fwrite(pes_header, 1, PES_HEADER_SIZE, stdout);
				frame_counter++;

			} else {
				pes_header[7] = 0x80; /* pts only, no escr, no es rate, no dsm trick, no extension flag, no additional copy info, no crc flag */
				pes_header[8] = 0x05; /* pts size*/ 
				stamp_ts(pts % PTS_MAX, pes_header + 9);
				pes_header[9] &= 0x0F;
				pes_header[9] |= 0x30;
				fwrite(pes_header, 1, PES_HEADER_SIZE - 5, stdout);
				frame_counter++;
				
			}
			
			fwrite(es_header, 1, ES_HEADER_MAX_SIZE, stdout);
			dts += pts_step;
			byte_read = fread(es_header, 1, ES_HEADER_SIZE, file_es);
			
		} else if (es_header[0] == 0x00 && es_header[1] == 0x00 && es_header[2] == 0x01 && es_header[3] == 0xB3) {
			
			/* need to know the time reference, look for the next start header*/
			memcpy(header_buffer, es_header, ES_HEADER_SIZE);
			header_buffer_size = ES_HEADER_SIZE;
			byte_read = fread(header_buffer + header_buffer_size, 1, ES_HEADER_SIZE, file_es);
			header_buffer_size += ES_HEADER_SIZE;
			

			while(byte_read && header_buffer_size < HEADER_BUFFER_MAX_SIZE - 4) {
			
				if (header_buffer[header_buffer_size - 4] == 0x00 && header_buffer[header_buffer_size - 3] == 0x00 && header_buffer[header_buffer_size - 2] == 0x01 && header_buffer[header_buffer_size - 1] == 0x00) { 

				    byte_read = fread(header_buffer + header_buffer_size, 1, 2, file_es);
				    header_buffer_size += 2;
				    
				    time_reference = (header_buffer[header_buffer_size - 2] << 2) | ((header_buffer[header_buffer_size - 1] & 0xC0) >> 6 );
				    /* fprintf(stderr, "temporal reference is %d\n", time_reference); */
				    byte_read = 0;
				    
				} else {
		
				    byte_read = fread(header_buffer + header_buffer_size, 1, 1, file_es);
				    header_buffer_size += 1;
				    
				}
			}
			
			if (header_buffer_size >= HEADER_BUFFER_MAX_SIZE - 4) {
				fprintf(stderr, "elementary stream header too verbose or corrupted, out of lookahead parser buffer\n");
				exit(2);
			} 

			dts_base = dts;
			pts = dts_base + (time_reference * pts_step) +  pts_step; 

			byte_read = 1;
			if (pts_limit > 0ll && pts_first_gop > 0ll && pts_a_gop > 0ll && pts > pts_first_gop) {
			    if (pts - pts_first_gop + pts_a_gop >= pts_limit - pts_first_gop) {
				byte_read = 0;
			    } 
			}

			if (byte_read) {
			    pes_header[7] = 0xC0; /* pts and dts, no escr, no es rate, no dsm trick, no extension flag, no additional copy info, no crc flag */
			    pes_header[8] = 0x0A; /* pts and dts size*/ 
			    stamp_ts(pts % PTS_MAX, pes_header + 9);
			    pes_header[9] &= 0x0F;
			    pes_header[9] |= 0x30;
			    stamp_ts(dts % PTS_MAX, pes_header + 14);
			    pes_header[14] &= 0x0F;
			    pes_header[14] |= 0x10;
			    fwrite(pes_header, 1, PES_HEADER_SIZE, stdout);
			    frame_counter++;

			    if (pts_first_gop == 0ll && pts > pts_step) {
				pts_first_gop = dts;
			        /* fprintf(stderr, "first gop pts length is %llu\n", pts_first_gop); */
			    } else if (pts_first_gop > 0 && pts_a_gop == 0ll) {
				pts_a_gop = dts - pts_first_gop;
			        /* fprintf(stderr, "a gop pts length is %llu\n", pts_a_gop); */ 
			    }

		    		
			    fwrite(header_buffer, 1, header_buffer_size, stdout);
			    header_buffer_size = 0;
			    dts += pts_step;
			    byte_read = fread(es_header, 1, ES_HEADER_SIZE, file_es);
			}
	
		} else {
		
			fwrite(es_header, 1, 1, stdout);
			es_header[0] = es_header[1];
			es_header[1] = es_header[2];
			es_header[2] = es_header[3];
			byte_read = fread(es_header + 3, 1, 1, file_es);
			
		}
		
		
	}
	
	return 0;
}

