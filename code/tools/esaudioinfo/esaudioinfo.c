/*  
 * Copyright (C) 2008-2013  Lorenzo Pallara, l.pallara@avalpa.com 
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
#define HEADER_MAX_SIZE 8

/* MPEG AUDIO */
const char* audio_version[4] = {	
				"MPEG Version 2.5",
				"Reserved",
				"MPEG Version 2 (ISO/IEC 13818-3)",
				"MPEG Version 1 (ISO/IEC 11172-3)"
				};
const char* audio_layer[4] = {
				"Reserved",
				"Layer III",
				"Layer II",
				"Layer I",
				};

const char* protection[2] = {
				"Protected by CRC (16bit crc follows header)",
				"Not protected",
				};

const char* bit_rate_audio[4][4][16] = {
				{ /* version 2.5 */
					{"wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", }, 
					{"free", "32", "16", "24", "32", "40", "48", "56", "64", "80", "96", "112", "128", "144", "160", "bad", }, /* layer 3 */
					{"free", "32", "16", "24", "32", "40", "48", "56", "64", "80", "96", "112", "128", "144", "160", "bad", }, /* layer 2 */
					{"free", "32", "48", "56", "64", "80", "96", "112", "128", "144", "160", "176", "192", "224", "256", "bad", }, /* layer 1 */
				},
				{ /* wrong */
					{"wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", }, 
					{"wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", }, 
					{"wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", }, 
					{"wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", }, 
				},
				{ /* version 2*/
					{"wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", }, 
					{"free", "32", "16", "24", "32", "40", "48", "56", "64", "80", "96", "112", "128", "144", "160", "bad", }, /* layer 3 */
					{"free", "32", "16", "24", "32", "40", "48", "56", "64", "80", "96", "112", "128", "144", "160", "bad", }, /* layer 2 */
					{"free", "32", "48", "56", "64", "80", "96", "112", "128", "144", "160", "176", "192", "224", "256", "bad", }, /* layer 1 */
				},
				{ /* version 1 */
					{"wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", "wrong", }, 
					{"free", "32", "40", "48", "56", "64", "80", "96", "112", "128", "160", "192", "224", "256", "320", "bad",} , /* layer 3 */
					{"free", "32", "48", "56", "64", "80", "96", "112", "128", "160", "192", "224", "256", "320", "384", "bad",} , /* layer 2 */
					{"free", "32", "64", "96", "128", "160", "192", "224", "256", "288", "320", "352", "384", "416", "448", "bad", }, /* layer 1 */
				},
};

const char* sampling[4][4] = {
				/* mpeg 2.5 */
				{"11025", "12000", "8000", "reserved",},
				/* wrong */
				{"wrong", "wrong", "wrong", "wrong", },
				/* mpeg 2 */
				{"22050", "24000", "16000", "reserved",},
				/* mpeg 1 */
				{"44100", "48000", "32000", "reserved",},
};

const char* padded[2] = {
				"Frame is not padded",
				"Frame is padded with one extra slot",
			};

const char* channel_mode[4] = {
				"Stereo",
				"Joint stereo (Stereo)",
				"Dual channel (Stereo)",
				"Single channel (Mono)",
			};

const char* copyright[2] = {
				"Audio is not copyrighted",
				"Audio is copyrighted",
			};

const char* original[2] = {
				"Copy of original media",
				"Original media", 
			};
const char* emphasis[4] = {
				"None",
				"50/15 ms",
				"Reserved",
				"CCIT J.17",
			};

/* AC3 audio */

const char* ac3sampling[4] = {"48000", "32000", "44100", "reserved"};
const char* eac3sampling[4] = {"24000", "22400", "16000", "reserved"};

unsigned int frame_size_code[38][3] = { /* 32000, 44100, 48000 */
	{96, 69, 64},
	{96, 70, 64},
	{120, 87, 80},
	{120, 88, 80},
	{144, 104, 96},
	{144, 105, 96},
	{168, 121, 112},
	{168, 122, 112},
	{192, 139, 128},
	{192, 140, 128},
	{240, 174, 160},
	{240, 175, 160},
	{288, 208, 192},
	{288, 209, 192},
	{336, 243, 224},
	{336, 244, 224},
	{384, 278, 256},
	{384, 279, 256},
	{480, 348, 320},
	{480, 349, 320},
	{576, 417, 384},
	{576, 418, 384},
	{672, 487, 448},
	{672, 488, 448},
	{768, 557, 512},
	{768, 558, 512},
	{960, 696, 640},
	{960, 697, 640},
	{1152, 835, 768},
	{1152, 836, 768},
	{1344, 975, 896},
	{1344, 976, 896},
	{1536, 1114, 1024},
	{1536, 1115, 1024},
	{1728, 1253, 1152},
	{1728, 1254, 1152},
	{1920, 1393, 1280},
	{1920, 1394, 1280}
};

/* DTS audio */

const char* amode[16] = { "A",
			"A + B (dual mono)",
			"L + R (stereo)",
			"(L+R) + (L-R) (sum and difference)",
			"LT + RT (left and right total)",
			"C + L + R",
			"L + R + S",
			"C + L + R + S",
			"L + R + SL + SR",
			"C + L + R + SL + SR",
			"CL + CR + L + R + SL + SR",
			"C + L + R + LR + RR + OV",
			"CF + CR + LF + RF + LR + RR",
			"CL + C + CR + L + R + SL + SR",
			"CL + CR + L + R + SL1 + SL2 + SR1 + SR2",
			"CL + C + CR + L + R + SL + S + SR"
			};

const char* sfreq[16] = {
			"invalid",
			"8 KHz",
			"16 KHz",
			"32 KHz",
			"invalid",
			"invalid",
			"11025 Hz",
			"22050 Hz",
			"44100 Hz",
			"invalid",
			"invalid",
			"12 KHz",
			"24 KHz",
			"48 KHz",
			"96 KHz",
			"192 KHz",
		};
		
const char* rate[32] = {
			"invalid",
			"invalid",
			"invalid",
			"invalid",
			"invalid",
			"128 kbps",
			"192 kbps",
			"224 kbps",
			"256 kbps",
			"320 kbps",
			"384 kbps",
			"448 kbps",
			"512 kbps",
			"576 kbps",
			"640 kbps",
			"768 kbps",
			"960 kbps",
			"1024 kbps",
			"1152 kbps",
			"1280 kbps",
			"1344 kbps",
			"1408 kbps",
			"1411.2 kbps",
			"1472 kbps",
			"1536 kbps",
			"1920 kbps",
			"2048 kbps",
			"3072 kbps",
			"3840 kbps",
			"open",
			"variable",
			"lossless",
		};
		
int main(int argc, char *argv[])
{
	int byte_read;
	int byte_count;
	int version;
	int layer;
	int bitrated;
	int sampled;
	int pad;
	int framelength;
	int ismpeg2;
	int iseac3;
	int isdts;
	int packet_counter;
	unsigned long long int total_byte_count;
	
	FILE* file_es;
	unsigned char es_header[HEADER_MAX_SIZE];
	
	/* Open es file */
	if (argc > 1) {
		file_es = fopen(argv[1], "rb");
	} else {
		fprintf(stderr, "Usage: 'esaudioinfo audio.es'\n");
		fprintf(stderr, "Prints info about sampling and frame size\n");
		fprintf(stderr, "Supports: mpeg2 audio layer 1,2,3\n");
		fprintf(stderr, "Supports: ac3 and enhanced-ac3\n");
		fprintf(stderr, "Supports: dts\n");
		return 2;
	}
	if (file_es == 0) {
		fprintf(stderr, "Can't find file %s\n", argv[1]);
		return 2;
	}
		
	/* Start to process the file */
	byte_count = 0;
	total_byte_count = 0LL;
	packet_counter = 0;
	byte_read = fread(es_header, 1, ES_HEADER_SIZE, file_es);
	
	isdts = (es_header[0] == 0x7F) && (es_header[1] == 0xFE) && (es_header[2] == 0x80) && (es_header[3] == 0x01);
	if (isdts) {
		ismpeg2 = 0;
	} else {
		ismpeg2 = (es_header[0] == 0xFF) && ((es_header[1] >> 5)== 0x07);
	} 
	
	iseac3 = -1;
	
	while(byte_read) {

		byte_count += byte_read;
		total_byte_count += byte_read;
		
		/* Search headers */
		if ( ismpeg2 && (es_header[0] == 0xFF) && ((es_header[1] >> 5)== 0x07)) { /* Audio header tentative */

			packet_counter++;
			
			if (byte_count > ES_HEADER_SIZE) {
				fprintf(stdout, "audio frame size from stream measured: %d bytes, %d bits\n\n", byte_count, byte_count * 8);
				byte_count = 0;
			}
			
			fprintf(stdout, "audio header packet %d, position:%llu\n", packet_counter, total_byte_count);
			
			version = (es_header[1] & 0x18) >> 3;
			fprintf(stdout, "audio version: %s\n", audio_version[version]);
			
			layer = (es_header[1] & 0x6) >> 1;
			fprintf(stdout, "audio layer: %s\n", audio_layer[layer]);
			
			fprintf(stdout, "protection bit: %s\n", protection[es_header[1] & 0x1]);
			
			bitrated = (es_header[2] & 0xF0) >> 4;
			fprintf(stdout, "bit rate index: %skbps\n", bit_rate_audio[version][layer][bitrated]);
			
			sampled = (es_header[2] & 0x0C) >> 2;
			fprintf(stdout, "sampling rate: %sHz\n", sampling[version][sampled]);
			
			pad = (es_header[2] & 0x3) >> 1;
			fprintf(stdout, "padding: %s\n", padded[pad]);
		
			if ((sampling[version][sampled][0] != 'w') && (sampling[version][sampled][0] != 'r') && (bit_rate_audio[version][layer][bitrated][0] != 'w') &&  (bit_rate_audio[version][layer][bitrated][0] != 'r')) { 
				if (layer == 0x3) {
					framelength = 12 * atoi(bit_rate_audio[version][layer][bitrated]) * 1000;
					framelength /= atoi(sampling[version][sampled]);
					framelength += pad;
					framelength *= 4;
				} else {
					framelength = 144 * atoi(bit_rate_audio[version][layer][bitrated]) * 1000;
					framelength /= atoi(sampling[version][sampled]);
					framelength += pad;
				}
			} else {
				framelength = 0;
			}
			fprintf(stdout, "audio frame from headers (formula can have rounds): %d bytes, %d bits\n", framelength, framelength * 8);
		
			fprintf(stdout, "channel mode: %s\n", channel_mode[(es_header[3] & 0xC0) >> 6]);
			
			fprintf(stdout, "copyrights: %s\n", copyright[(es_header[3] & 0x08) >> 3]);
			
			fprintf(stdout, "original: %s\n", original[(es_header[3] & 0x04) >> 2]);
			
			fprintf(stdout, "emphasis: %s\n", emphasis[es_header[3] & 0x03]);		
			
			byte_read = fread(es_header, 1, ES_HEADER_SIZE, file_es);
		
		} else if (!ismpeg2 && !isdts && (es_header[0] == 0x0B) && (es_header[1] == 0x77)) {
			
			packet_counter++;
			
			if (byte_count > ES_HEADER_SIZE) {
				fprintf(stdout, "audio frame size from stream measured: %d bytes, %d bits\n\n", byte_count, byte_count * 8);
				byte_count = 0;
			}
			
			/* is it possibile to decide if it is ac3 or ac3-enhanced better then this ? how? doing crc?  */
			if (iseac3 < 0) {
			    if  (((es_header[2] >> 3) & 0x07) == 0) {
				iseac3 = 1;
			    } else {
				iseac3 = 0;
			    }
			}
			
			if (iseac3) {
			
			    unsigned int frame_size = 0;
			    unsigned char fs_cod = 0;
			    frame_size = ((es_header[2] & 0x07) << 8) + es_header[3];
			    fprintf(stdout, "eac3 frame size is %d byte\n", frame_size * 2);

			    byte_read = fread(es_header, 1, 1, file_es);
			    byte_count++;
			    total_byte_count++;
			    
			    fs_cod = es_header[0] >> 6;
			    if (fs_cod != 0x03) {
				fprintf(stdout, "eac3 sampling rate is %s\n", ac3sampling[fs_cod]);
			    } else {
				fs_cod = (es_header[0] >> 4) & 0x03;
				fprintf(stdout, "eac3 sampling rate is %s\n", eac3sampling[fs_cod]);
			    }
			
			
			} else {

			    unsigned char frame_code = 0;
			    unsigned char sampling = 0;

			    byte_read = fread(es_header, 1, 1, file_es);
			    byte_count++;
			    total_byte_count++;
			
			    sampling = es_header[0] >> 6;
			    fprintf(stdout, "ac3 sampling rate is %s\n", ac3sampling[sampling]);
			
			    frame_code = es_header[0] & 0x3F;
			    fprintf(stdout, "ac3 frame size is %d byte\n", frame_size_code[frame_code][2 - sampling] * 2);

			}
				
			byte_read = fread(es_header, 1, ES_HEADER_SIZE, file_es);
		
		} else if (isdts && (es_header[0] == 0x7F) && (es_header[1] == 0xFE) && (es_header[2] == 0x80) && (es_header[3] == 0x01)) {
		
			packet_counter++;
			
			if (byte_count > ES_HEADER_SIZE) {
				fprintf(stdout, "audio frame size from stream measured: %d bytes, %d bits\n\n", byte_count, byte_count * 8);
				byte_count = 0;
			}
			
			byte_read = fread(es_header, 1, 6, file_es);
			
			byte_count += 6;
			total_byte_count += 6;

			fprintf(stdout, "ftype: %d\n", (es_header[0] >> 7) & 0x1);
			fprintf(stdout, "frame short: %d\n", (es_header[0] >> 2) & 0x1F);
			fprintf(stdout, "cpf crc: %d\n", (es_header[0] >> 1) & 0x1);
			fprintf(stdout, "nblks: %d\n", ((es_header[0] & 0x1) << 6) | (es_header[1] >> 2)) ;
			fprintf(stdout, "fsize: %d\n", ((es_header[1] & 0x3) << 12) | (es_header[2] << 4) | (es_header[3] >> 4));
			unsigned int temp = ((es_header[3] & 0xF) << 2) | ((es_header[4] >> 6) & 0x3);
			fprintf(stdout, "amode: %s\n", amode[temp]);
			temp = (es_header[4] >> 2) & 0xF;
			fprintf(stdout, "sfreq: %s\n", sfreq[temp]);
			temp = ((es_header[4] & 0x3) << 3)| ((es_header[5] >> 5) & 0x1F);
			fprintf(stdout, "rate: %s\n", rate[temp]);
			
			byte_read = fread(es_header, 1, ES_HEADER_SIZE, file_es);
			
		} else {
		
			es_header[0] = es_header[1];
			es_header[1] = es_header[2];
			es_header[2] = es_header[3];
			byte_read = fread(es_header + 3, 1, 1, file_es);
	
		}
	}
	
	return 0;
}

