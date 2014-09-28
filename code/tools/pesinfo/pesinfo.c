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

#define PACK_HEADER_SIZE 4 
#define TIME_STAMP_SIZE 5

const long long unsigned system_frequency = 27000000;

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

int main(int argc, char *argv[])
{
	int byte_read;
	FILE* file_pes;
	unsigned short pes_size;
	long long unsigned time;
	unsigned char pes_header[PACK_HEADER_SIZE];
	unsigned char timestamp[TIME_STAMP_SIZE];
	unsigned char byte;
	
	/* Open pes file */
	if (argc > 1) {
		file_pes = fopen(argv[1], "rb");
	} else {
		fprintf(stderr, "Usage: 'pesinfo filename.pes'\n");
		return 2;
	}
	if (file_pes == 0) {
		fprintf(stderr, "Can't find file %s\n", argv[1]);
		return 2;
	}
		
	/* Start to process the file */
	byte_read = fread(pes_header, 1, 4, file_pes);
	while(byte_read) {
	
		/* Check headers */
		byte_read = 0;
		if ((pes_header[0] == 0x00) && (pes_header[1] == 0x00) && (pes_header[2] == 0x01)) { 
			if ((pes_header[3] >> 4) == 0x0E) {
				fprintf(stdout, "pes header: %02x %02x %02x %02x, ", pes_header[0], pes_header[1], pes_header[2], pes_header[3]);
				fprintf(stdout, "video stream number %d, ", pes_header[3] & 0x0F);	
				fread(&pes_size, 1, 2, file_pes); /* get pack size */
				pes_size = ntohs(pes_size);
				fprintf(stdout, "pes size %d, ", pes_size);
				fseek(file_pes, 2, SEEK_CUR); /* get header size */
				fread(&byte, 1, 1, file_pes);
				fprintf(stdout, "header length: %d, ", byte);
				fread(timestamp, 1, TIME_STAMP_SIZE, file_pes);
				time = parse_timestamp(timestamp);
				fprintf(stdout, "Presentation Time Stamp is: %llu, %llu.%04llu sec.", time,  time / (system_frequency / 300), (time % (system_frequency / 300)) / ((system_frequency / 300) / 10000)); 
				if (byte > 5) {
					byte_read = fread(timestamp, 1, TIME_STAMP_SIZE, file_pes);
                                	time = parse_timestamp(timestamp);
					fprintf(stdout, ", Decode Time Stamp is: %llu, %llu.%04llu sec.\n", time,  time / (system_frequency / 300), (time % (system_frequency / 300)) / ((system_frequency / 300) / 10000)); 
				} else {
					fprintf(stdout, "\n");
				}
				byte_read = fread(pes_header, 1, 4, file_pes);
			} else if ((pes_header[3] >> 5) == 0x06) {
				fprintf(stdout, "pes header: %02x %02x %02x %02x, ", pes_header[0], pes_header[1], pes_header[2], pes_header[3]);
				fprintf(stdout, "audio stream number %d, ", pes_header[3] & 0x1F);	
				fread(&pes_size, 1, 2, file_pes); /* get pack size */
				pes_size = ntohs(pes_size);
				fprintf(stdout, "pes size %d, ", pes_size);
				fseek(file_pes, 2, SEEK_CUR); /* get header size */
				fread(&byte, 1, 1, file_pes);
				fprintf(stdout, "header length: %d, ", byte);
				fread(timestamp, 1, TIME_STAMP_SIZE, file_pes);
				time = parse_timestamp(timestamp);
				fprintf(stdout, "Presentation Time Stamp is: %llu, %llu.%04llu sec.\n", time,  time / (system_frequency / 300), (time % (system_frequency / 300)) / ((system_frequency / 300) / 10000)); 
				byte_read = fread(pes_header, 1, 4, file_pes);
			} else if (pes_header[3] == 0xBD) {
				fprintf(stdout, "pes header: %02x %02x %02x %02x, ", pes_header[0], pes_header[1], pes_header[2], pes_header[3]);
				fread(&pes_size, 1, 2, file_pes); /* get pack size */
				pes_size = ntohs(pes_size);
				fprintf(stdout, "pes size %d, ", pes_size);
				fseek(file_pes, 2, SEEK_CUR); /* get header size */
				fread(&byte, 1, 1, file_pes);
				fprintf(stdout, "header length: %d, ", byte);
				fread(timestamp, 1, TIME_STAMP_SIZE, file_pes);
				time = parse_timestamp(timestamp);
				fprintf(stdout, "Presentation Time Stamp is: %llu, %llu.%04llu sec.\n", time,  time / (system_frequency / 300), (time % (system_frequency / 300)) / ((system_frequency / 300) / 10000)); 
				byte_read = fread(pes_header, 1, 4, file_pes);
			}
		} 
		if (byte_read == 0) {
			pes_header[0] = pes_header[1];
			pes_header[1] = pes_header[2];
			pes_header[2] = pes_header[3];
			byte_read = fread(pes_header+3, 1, 1, file_pes);
		}
	}
	
	return 0;
}

