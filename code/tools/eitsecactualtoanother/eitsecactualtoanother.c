/*  
 * Copyright (C) 2011-2013  Lorenzo Pallara, l.pallara@avalpa.com
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
#include <sys/time.h>
#include <inttypes.h>


#include "../../libs/sectioncrc/sectioncrc.c"

#define SECTION_MAX_SIZE 4096
#define SECTION_HEADER_SIZE 3

unsigned char g_section[SECTION_MAX_SIZE+1];	/* +1 to handle a special case, see later in the code */
unsigned char g_section_head[SECTION_HEADER_SIZE];    

unsigned char* get_section(unsigned short* section_size, int fd) 
{
	unsigned short	temp = 0;
	unsigned char* section = 0;

	if (section_size) {

		/* Read first bytes */
		*section_size = 0;
		if (read(fd, g_section_head, SECTION_HEADER_SIZE)) {

			/* Parse datagram section size */
			memcpy(&temp, g_section_head + 1, 2);
			temp = ntohs(temp);
			temp &= 0x0FFF;		
			*section_size = temp;
    
			/* Read all the section */
			if (*section_size <= (SECTION_MAX_SIZE - SECTION_HEADER_SIZE)) {
				memcpy(g_section, g_section_head, SECTION_HEADER_SIZE);
				read(fd, g_section + SECTION_HEADER_SIZE, *section_size);
				section = g_section;
				*section_size += SECTION_HEADER_SIZE;
			} else {
				fprintf(stderr, "Section too big skipped\n");
			}
		}
	}

	return section;
}


void Usage (void) {
	fprintf(stderr, "Usage: 'eitsecactualtoanother inputeit.sec > outputeit.sec \n");
}



int main(int argc, char *argv[])
{
	unsigned short a_section_size;
	unsigned char *a_section;
	int fd_sec;
	unsigned int crc;
	
	if (argc >= 2) {
		fd_sec = open(argv[1], O_RDONLY);
		if (fd_sec < 0) {
			fprintf(stderr, "Can't find file %s\n", argv[1]);
			return 2;
		}
	} else {
		Usage();
		return 2;
	}
	
	while(1) {
		
		a_section = get_section(&a_section_size, fd_sec);
		if (a_section_size <= 0 || a_section == 0) {
			return 0;
		}
		
		if (a_section[0] == 0x4E) {
			a_section[0] = 0x4F;
			a_section[13] = 0x4F;
		} else if (a_section[0] >= 0x50 && a_section[0] <= 0x5F) {
			a_section[0] = 0x60 | (a_section[0] & 0xF) ;
			a_section[13] = 0x60 | (a_section[13] & 0xF) ;
		}
		
		crc = sectioncrc(a_section, a_section_size - 4);
		a_section[a_section_size - 4] = (crc >> 24) & 0xFF;
		a_section[a_section_size - 3] = (crc >> 16) & 0xFF;
		a_section[a_section_size - 2] = (crc >> 8) & 0xFF;
		a_section[a_section_size - 1] = crc & 0xFF;
		
		write(STDOUT_FILENO, a_section, a_section_size);
		
	}
	
	return 0;
}
