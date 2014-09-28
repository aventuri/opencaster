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

#define PES_HEADER_SIZE 14
#define PES_HEADER_SIZE_WITH_AD 31
#define PACK_AUDIO_FRAME_INCREASE 8 
#define PACK_AUDIO_FRAME_INCREASE_WITH_AD 25
#define ES_HEADER_SIZE 4 
#define PTS_MAX 8589934592LL 

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
	int byte_read;
	FILE* file_es;
	int ismpeg2;
	int iseac3;
	int isdts;
	int AD_fade_value = -1;
	unsigned long long int pts = 0LL;
	unsigned long long int pts_limit = 0ll;
	unsigned long long int frame_number = 0ll;
	unsigned char stream_id = 192; /* usual value */
	unsigned char pes_header[PES_HEADER_SIZE_WITH_AD];
	unsigned char* es_frame;
	unsigned short es_frame_size;
	unsigned short pes_frame_size;
	unsigned int sample_rate;
	unsigned int samples_per_frame;
	unsigned long long int pts_offset = 0ll;
	
	/* Open pes file */
	if (argc > 4) {
		file_es = fopen(argv[1], "rb");
		samples_per_frame = atoi(argv[2]);
		sample_rate = atoi(argv[3]);
		es_frame_size = atoi(argv[4]);
		AD_fade_value = atoi(argv[5]);
		if (es_frame_size <= 0) {
			fprintf(stderr, "audio_frame_size suggested is not valid\n");
			return 2;
		}
		pes_frame_size = es_frame_size + PACK_AUDIO_FRAME_INCREASE;
		if (AD_fade_value >= 0) {
			pes_frame_size = es_frame_size + PACK_AUDIO_FRAME_INCREASE_WITH_AD;
			unsigned char ad = (unsigned char) AD_fade_value;
			fprintf(stderr, "AD fade value is %u\n", ad);
		}
		es_frame = malloc(es_frame_size);
		if (!es_frame) {
			fprintf(stderr, "Out of memory\n");
			return 2;
		}
	} else {
		fprintf(stderr, "Usage: 'esaudio2pes audio.es samples_per_frame sample_rate frame_size_without_padding AD_fade_value [pts_offset] [pts_limit] [stream_id]'\n");
		fprintf(stderr, "pts_offset can be used to set a pts different from zero to synch audio to the first video pts\n");
		fprintf(stderr, "pts_limit can be used to limit time length to force audio to end before the last video pts\n");
		fprintf(stderr, "Example for mpeg layer 2 audio with frame size 768 and sample rate 48000: esaudio2pes audio.es 1152 48000 768 -1\n");
		fprintf(stderr, "Example for ac3 audio with frame size 384 and sample rate 48000: esaudio2pes audio.es 1536 48000 384 -1\n");
		fprintf(stderr, "Example for dts audio with nblck 15 e short 31, samples are 16*32: esaudio2pes audio.es 512 48000 2012 -1\n");
		fprintf(stderr, "valid id for audio are 110xxxxx, default is 192\n");
		return 2;
	}
	
	if (argc > 6) {
		pts_offset = atol(argv[6]);
	}
	
	if (argc > 7) {
		pts_limit = atol(argv[7]);
	}
		
	if (argc > 8) {
		stream_id = atoi(argv[8]);
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
	pes_frame_size = htons(pes_frame_size);  
	memcpy(pes_header + 4, &pes_frame_size, 2);
	pes_frame_size = ntohs(pes_frame_size);
	pes_header[6] = 0x81; /* no scrambling, no priority, no alignment defined, no copyright, copy */
	pes_header[7] = 0x80; /* pts, no escr, no es rate, no dsm trick, no extension flag, no additional copy info, no crc flag */
	pes_header[8] = 0x05; /* pts */ 
	if (AD_fade_value >= 0) {
		pes_header[7] = 0x81; /* pts, no escr, no es rate, no dsm trick, yes extension flag, no additional copy info, no crc flag */
		pes_header[8] = 0x16; /* pts and AD */
		pes_header[14] = 0x8E; /* pes private data, reserved */
		pes_header[15] = 0xF8; /* AD */
		pes_header[16] = 0x44; /* AD */
		pes_header[17] = 0x54; /* AD */
		pes_header[18] = 0x47; /* AD */
		pes_header[19] = 0x41; /* AD */
		pes_header[20] = 0x44; /* AD */
		pes_header[21] = 0x31; /* AD */
		pes_header[22] = (unsigned char) AD_fade_value; /* AD fade value*/
		pes_header[23] = 0x00; /* AD pan value*/
		pes_header[24] = 0xFF; /* AD */
		pes_header[25] = 0xFF; /* AD */
		pes_header[26] = 0xFF; /* AD */
		pes_header[27] = 0xFF; /* AD */
		pes_header[28] = 0xFF; /* AD */
		pes_header[29] = 0xFF; /* AD */
		pes_header[30] = 0xFF; /* AD */
	}
	
	byte_read = fread(es_frame, 1, ES_HEADER_SIZE, file_es);
	bframe = 1;
	int padding = 0;
	pts = pts_offset;
	frame_number = 1;
	isdts = (es_frame[0] == 0x7F) && (es_frame[1] == 0xFE) && (es_frame[2] == 0x80) && (es_frame[3] == 0x01);
	if (isdts) {
		ismpeg2 = 0;
	} else {
		ismpeg2 = (es_frame[0] == 0xFF) && ((es_frame[1] >> 5)== 0x07);
	} 
	iseac3 = -1;
	while(byte_read) {
		
		if (
		(ismpeg2 && es_frame[0] == 0xFF && (es_frame[1] >> 5) == 0x07) ||
		(!ismpeg2 && (es_frame[0] == 0x0B) && (es_frame[1] == 0x77)) ||
		(isdts && (es_frame[0] == 0x7F) && (es_frame[1] == 0xFE) && (es_frame[2] == 0x80) && (es_frame[3] == 0x01))
		) { 
			byte_read = fread(es_frame + ES_HEADER_SIZE, 1, es_frame_size - ES_HEADER_SIZE, file_es);
			stamp_ts (pts % PTS_MAX, pes_header + 9);
			pes_header[9] &= 0x0F; 
			pes_header[9] |= 0x20; 
			/* check for padding */
			if (ismpeg2 && (((es_frame[2] & 0x3) >> 1) > 0)) {
				/* check layer */
				int layer = es_frame[1] & 0x6 >> 1;
				if (layer == 2) {
					padding = 4;
				} else {
					padding = 1;
					}
			} else {
				padding = 0;
			}
			pts = pts_offset + ((frame_number * samples_per_frame * 90000) / sample_rate);
			if (padding) {
				pes_frame_size = htons(pes_frame_size + padding);  
				memcpy(pes_header + 4, &pes_frame_size, 2);
				pes_frame_size = ntohs(pes_frame_size) - padding;
			} else {
				pes_frame_size = htons(pes_frame_size);  
				memcpy(pes_header + 4, &pes_frame_size, 2);
				pes_frame_size = ntohs(pes_frame_size);
			}
			if (AD_fade_value >= 0) {
				fwrite(pes_header, 1, PES_HEADER_SIZE_WITH_AD, stdout);
			} else {
				fwrite(pes_header, 1, PES_HEADER_SIZE, stdout);
			}
			frame_number++;
			fwrite(es_frame, 1, es_frame_size, stdout);
			if (padding) {
				fread(es_frame,1, padding, file_es);
				fwrite(es_frame,1, padding, stdout);
			}
			byte_read = fread(es_frame, 1, ES_HEADER_SIZE, file_es);
			
		} else {
			fprintf(stderr, "Critical: sync byte missing, corrupted ES or frame size changed\n");
			byte_read = 0;
		}
		if (pts_limit > 0ll && pts >= pts_limit) {
			byte_read = 0;
		}
	}
	
	return 0;
}
