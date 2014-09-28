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

#define PES_HEADER_SIZE 4 

int main(int argc, char *argv[])
{
	int byte_read;
	int first_found;
	unsigned char stream_id;
	FILE* file_pes;
	unsigned char pes_header[PES_HEADER_SIZE];
	unsigned char byte;
	unsigned char length;
	
	/* Open pes file */
	if (argc > 2) {
		file_pes = fopen(argv[1], "rb");
		stream_id = atoi(argv[2]);
	} else {
		fprintf(stderr, "Usage: 'pes2es filename.pes stream_id_header_to_skip'\n");
		fprintf(stderr, "Example for audio: 'pes2es filename.pes 192', 192 is 0xc0 \n");
		fprintf(stderr, "Example for video: 'pes2es filename.pes 224', 224 is 0xe0\n");
		return 2;
	}
	if (file_pes == 0) {
		fprintf(stderr, "Can't find file %s\n", argv[1]);
		return 2;
	}
		
	/* Start to process the file */
	first_found = 0;
	byte_read = fread(pes_header, 1, 4, file_pes);
	while(byte_read) {
	
		/* Skip headers */
		if ((pes_header[0] == 0x00) && (pes_header[1] == 0x00) && (pes_header[2] == 0x01) && (pes_header[3] == stream_id)) { 
			first_found = 1;
			// fprintf(stderr, "header: %02x %02x %02x %02x, ", pes_header[0], pes_header[1], pes_header[2], pes_header[3]);
			// fseek(file_pes, 4, SEEK_CUR); /* get header size */
			int i = 0;
			for (i = 0; i < 4; i++) {
				fread(&byte, 1, 1, file_pes);
			}
			fread(&length, 1, 1, file_pes);
			// fprintf(stderr, "length: %d\n", byte);
			// fseek(file_pes, byte, SEEK_CUR); /* skip header */
			for (i = 0; i < length; i++) {
				fread(&byte, 1, 1, file_pes);
			}
			byte_read = fread(pes_header, 1, PES_HEADER_SIZE, file_pes);
		} else {
			if (first_found) {
			    fwrite(pes_header, 1, 1, stdout);
			}
			pes_header[0] = pes_header[1];
			pes_header[1] = pes_header[2];
			pes_header[2] = pes_header[3];
			byte_read = fread(pes_header+3, 1, 1, file_pes);
		}
	}
	fwrite(pes_header, 1, 3, stdout);
	
	return 0;
}

