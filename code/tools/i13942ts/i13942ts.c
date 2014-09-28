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

#define I1394_HEADER_SIZE 4
#define CIP_HEADER_SIZE 8
#define SP_HEADER_SIZE 4
#define TS_PACKET_SIZE 188

int main(int argc, char *argv[])
{
	int byte_read = 0;
	FILE* file_i1394 = 0;
	int data_length = 0;
	unsigned char fmt = 0;
	unsigned short dbs_fn_qpc_sph = 0;
	unsigned char CIP_header[CIP_HEADER_SIZE];
	unsigned char SP_header[SP_HEADER_SIZE];
	unsigned char ts_packet[TS_PACKET_SIZE];
	unsigned char i1394_header[I1394_HEADER_SIZE];

	file_i1394 = 0;
	if (argc > 1) {
		file_i1394 = fopen(argv[1], "rb");
	} 
	
	if (file_i1394 == 0) { 
		fprintf(stderr, "Usage: 'i13942ts file.iso', where .iso is a ieee1394-6 dump\n");
		return 2;
	}
	
	/* Process the i1394 file */
	byte_read = fread(i1394_header, 1, I1394_HEADER_SIZE, file_i1394);
	while (byte_read) {
		byte_read = fread(CIP_header, 1, CIP_HEADER_SIZE, file_i1394);
		data_length = (htonl(*(unsigned long*)i1394_header) >> 16);
		dbs_fn_qpc_sph = (htonl(*(unsigned long*)CIP_header) >> 10) & 0x3fff;
		fmt = (htonl(*(unsigned long*)(CIP_header + 4)) >> 24) & 0x3f;
		if (fmt == 0x20 && dbs_fn_qpc_sph == 0x01b1) {
			data_length -= CIP_HEADER_SIZE;
			for ( ;  data_length > TS_PACKET_SIZE;  data_length -= TS_PACKET_SIZE + SP_HEADER_SIZE) {
				fread(SP_header, 1, SP_HEADER_SIZE, file_i1394);
				fread(ts_packet, 1, TS_PACKET_SIZE, file_i1394);
				fwrite(ts_packet, 1, TS_PACKET_SIZE, stdout);
			}
		}
		byte_read = fread(i1394_header, 1, I1394_HEADER_SIZE, file_i1394);
	}

	return 0;
}

