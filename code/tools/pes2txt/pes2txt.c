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

#define PES_HEADER_SIZE 4 

int main(int argc, char *argv[])
{
	int byte_read;
	int first_found;
	FILE* file_pes;
	unsigned char pes_header[PES_HEADER_SIZE];
	unsigned char byte;
	
	/* Open pes file */
	if (argc > 1) {
		file_pes = fopen(argv[1], "rb");
	} else {
		fprintf(stderr, "Usage: 'pes2txt filename.pes > txt_units'\n");
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
		if ((pes_header[0] == 0x00) && (pes_header[1] == 0x00) && (pes_header[2] == 0x01) && (pes_header[3] == 0xBD)) { 
			first_found = 1;
			// fprintf(stderr, "header: %02x %02x %02x %02x, ", pes_header[0], pes_header[1], pes_header[2], pes_header[3]);
			fseek(file_pes, 4, SEEK_CUR); /* get header size */
			fread(&byte, 1, 1, file_pes);
			// fprintf(stderr, "length: %d\n", byte);
			fseek(file_pes, byte + 1, SEEK_CUR); /* skip header and ebu byte*/
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

