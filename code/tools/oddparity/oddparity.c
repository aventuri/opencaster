/*  
 * Copyright (C) 2009-2013  Lorenzo Pallara, l.pallara@avalpa.com
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


int main(int argc, char *argv[]) {
	unsigned char byte = 0;
	unsigned char paritybyte = 0;
	unsigned char parity = 0;
	fprintf(stdout, "{ ");
	for (byte = 0; byte < 127; byte++) {
		parity = ((byte & 0x01) + 
		((byte >> 1) & 0x01) + 
		((byte >> 2) & 0x01) + 
		((byte >> 3) & 0x01) +
		((byte >> 4) & 0x01) + 
		((byte >> 5) & 0x01) + 
		((byte >> 6) & 0x01) );
		if ((parity % 2) == 0)  {
		    paritybyte = 0x80 | byte;
		} else {
		    paritybyte = byte;
		}
		fprintf(stdout, "0x%02x, ", paritybyte);
	}
	fprintf(stdout, " }\n");
	return 0;
}

