/*
 * table.c
 */

/*
 * Copyright (C) 2008, Lorenzo Pallara
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "dsmcc.h"
#include "carousel.h"
#include "biop.h"
#include "utils.h"
#include "table.h"

#define SECTION_HEADER_SIZE 3


int
read_dsmcc_tables(struct carousel *car, unsigned char* buffer)
{
	int result = 0;
	u_char section_head[SECTION_HEADER_SIZE];    
	u_short temp = 0;
	int section_size = 0;
	int found = 0;

	verbose("reading dsmcc tables\n");

	while (!found) {
	    if (read(STDIN_FILENO, section_head, SECTION_HEADER_SIZE)) {

		/* Parse datagram section size */
		memcpy(&temp, section_head + 1, 2);
		temp = ntohs(temp);
		temp &= 0x0FFF;
		section_size = temp;
		fprintf(stderr, "section size is %d\n", section_size);
		
		/* Read all the section */
		if (section_size <= (MAX_TABLE_LEN - SECTION_HEADER_SIZE)) {
			memcpy(buffer, section_head, SECTION_HEADER_SIZE);
			read(STDIN_FILENO, buffer + SECTION_HEADER_SIZE, section_size);
			result = section_size;
			if (buffer[0] & 0x3F) {
			    found = 1;
			}
		} else {
			fprintf(stderr, "found section too big, size is %d, skipped, section_size\n", section_size);
		}
	    } else {
		found = 1;
		result = -1;
	    }
	}

/*
	hexdump(buffer, result);
*/

	verbose("reading dsmcc table done\n");

	return result;

}



void
add_dsmcc_pid(struct carousel *car, uint16_t pid)
{
    ;
}

int 
read_dsi_dii_tables(struct carousel *car, unsigned char* buffer)
{
	int found = 0;
	int result = 0;
	
	while(!found) {
	    result = read_dsmcc_tables(car, buffer);
	    if (result > 0) {
		if (buffer[0] & 0x3B) {
		    found = 1;
		}
	    } else {
		found = 1;
	    } 
	}
	
	verbose("reading dsi dii table done\n");

	return result;

}

