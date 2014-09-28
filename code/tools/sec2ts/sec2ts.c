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
 *
 * Stuff option added by julian.cable@bbc.co.uk  
 * and by Guislain Bonnard guislain.bonnard@quadrille.fr>
 *
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
#include <sys/stat.h>
#include <fcntl.h>

#define DEFAULT_PID 0x200
#define TS_HEADER_MAX_SIZE 5 
#define TS_PACKET_SIZE 188
#define SECTION_MAX_SIZE 4096
#define SECTION_HEADER_SIZE 3

u_char g_section[SECTION_MAX_SIZE+1];	/* +1 to handle a special case, see later in the code */
u_char g_section_head[SECTION_HEADER_SIZE];    
u_short stuff = 0;

u_char* get_section(u_short* section_size, int fd) 
{
	u_short	temp = 0;
	u_char* section = 0;

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
				/* now see if we should add stuffing */
				if (stuff) {
					u_short residue = *section_size % 184;
					/* first TS packet has one octet of overhead */
					if(residue != 183) {
						u_short num_stuff_octets = 183 - residue;
						u_char* stuff_ptr = &g_section[*section_size];
						memset(stuff_ptr, 0xff, num_stuff_octets);
						*section_size += num_stuff_octets;
					}
				}
			} else {
				fprintf(stderr, "Section too big skipped\n");
			}
		} else {
			fprintf(stderr, "No more sections\n");
		}
	}

	return section;
}

int main(int argc, char *argv[])
{
	int not_finished;
	int fd_in;
	FILE* fd_out;
	int ts_bytes;			/* TS packet output counter */
	int section_bytes;		/* Section output counter */
	int ts_needtopad;		/* Set if necessary to pad to end a packet */
	int ts_padded_did_not_output_next_header; /* Set if packet header wasn't written after padding */
	u_short pid;			/* Pid for the TS packets */
	u_short temp;			/* Temp */ 
	u_short section_size;		/* Section size */
	u_short section_size_next;	/* Section size next section */
	u_char *section_next;		/* Next section */
	u_char *section;		/* Current section */
	u_char ts_header[TS_HEADER_MAX_SIZE]; /* TS packet header */
	u_char section_memory[SECTION_MAX_SIZE + 1]; /* Current section body */
	u_char ts_header_size;		/* Current size of the ts header */
	u_char ts_continuity_counter;	/* Continuity counter */

	/* Set pid and fd */
	fd_in = STDIN_FILENO;
	fd_out = stdout;
	if (argc > 1) {
		if (argc > 2) {
			if(strcmp(argv[2], "-s") == 0) {
				stuff = 1;
			}
		}
		pid = atoi(argv[1]);
		if (pid < 8192) {
			pid = htons(pid);
	 	} else { 
			fprintf(stdout, "Usage: 'sec2ts pid [-s]', where pid is bounded from 1 to 8191 and as default is %d\n", DEFAULT_PID);
			fprintf(stdout, "-s tells sec2ts to start sections on TS packet boundaries\n");
			fprintf(stdout, "Sections come from stdin and ts packets will go out from stdout.\n");
			return 2;
		}
	} else {
		pid = htons(DEFAULT_PID); 
	}

	/* Set TS packet fixed header byte */
	ts_header[0] = 0x47; /* sync byte */ 	
	ts_continuity_counter = 0x0; 
	ts_bytes = 0;
	section = 0;
	section_next = 0;
	ts_needtopad = 0;
	ts_padded_did_not_output_next_header = 0;
	
	/* Write the first TS valid packet header */
	memcpy(ts_header + 1, &pid, 2); /* pid */
	ts_header[1] |= 0x40; /* payload unit start indicator */
	ts_header[3] = ts_continuity_counter | 0x10; /* continuity counter, no scrambling, only payload */
	ts_continuity_counter = (ts_continuity_counter + 1) % 0x10; /* inc. continuity counter */
	ts_header[4] = 0x0; /* point field */                
	ts_header_size = TS_HEADER_MAX_SIZE;
	fwrite(ts_header, 1, ts_header_size, fd_out);
	ts_bytes = ts_header_size; 
	
	/* Start to process sections */
	not_finished = 1;
	if(stuff){
		section_next = 1;
	} else {
		section_next = get_section(&section_size_next, fd_in);
	}
	while (not_finished) {

		if (section_next != 0 && !stuff) {
			memcpy(section_memory, section_next, SECTION_MAX_SIZE + 1);
			section = section_memory;
			section_size = section_size_next;
			section_next = get_section(&section_size_next, fd_in);
		} else if(section_next != 0 &&  stuff) {
			section = get_section(&section_size, fd_in);
		} else {
			section = 0;
			section_size = 0;
			section_next = 0;
			section_size_next = 0;
		}

		/* Pack the section */
		if (section) {

			if (ts_padded_did_not_output_next_header) {
			    fwrite(ts_header, 1, ts_header_size, fd_out);
			    ts_bytes = ts_header_size;
			    ts_padded_did_not_output_next_header = 0; 
			}

			/* Check if padding is necessary */
			if (TS_PACKET_SIZE - ts_bytes > section_size) {
				ts_needtopad = 1;
			}

			/* Output is section framed in TS packets */
			section_bytes = 0;
			while (section_bytes < section_size ) {
				if (section_bytes < section_size) {
					for (; ts_bytes < TS_PACKET_SIZE && section_bytes < section_size; ts_bytes++, section_bytes++)
						fwrite(section + section_bytes, 1, 1, fd_out);
				}
				
				if (ts_bytes == TS_PACKET_SIZE) {
					fflush(fd_out);
                                    	memcpy(ts_header + 1, &pid, 2); /* pid */
					if ((section_size - section_bytes) < (TS_PACKET_SIZE - TS_HEADER_MAX_SIZE)) { 
					    if (section_next != 0) { 
					        ts_header_size = TS_HEADER_MAX_SIZE; /* then next packet starts a new section */
					        ts_header[1] |= 0x40; /* set payload unit start indicator */
					        ts_header[4] = section_size - section_bytes; /* pointer field */ 
					    } else {
					        ts_header_size = TS_HEADER_MAX_SIZE - 1 ; /* then next packet is the last */
					        ts_header[1] &= 0xBF; /* clear set payload unit start indicator */
					    }
					} else {
					    ts_header_size = TS_HEADER_MAX_SIZE - 1; /* section will go over the next packet */
					    if ((section_size - section_bytes) == (TS_PACKET_SIZE - TS_HEADER_MAX_SIZE)) { /* and finish exactly at the end of it! */
						/* we need to introduce a 1 byte ! */
						*(section+section_size) = 0xff;
						section_size++;
					    }
					}
					ts_header[3] = ts_continuity_counter | 0x10; /* continuity counter, no scrambling, only payload */
					ts_continuity_counter = (ts_continuity_counter + 1) % 0x10; /* inc. continuity counter */
					fwrite(ts_header, 1, ts_header_size, fd_out);
					ts_bytes = ts_header_size;
				}
			}

		} else {
			ts_needtopad = 1;	
			not_finished = 0;
		} 
		
		/* Padding */ 
		if (ts_needtopad) {
			temp = 0xffff;
			for ( ; ts_bytes < TS_PACKET_SIZE; ts_bytes++)
				fwrite(&temp, 1, 1, fd_out);
			ts_needtopad = 0;
			fflush(fd_out);	
				
			memcpy(ts_header + 1, &pid, 2); /* pid */
			if (section_next != 0) {
				ts_header[1] |= 0x40; /* payload unit start indicator */
				ts_header[4] = 0x0; /* point field */
				ts_header_size = TS_HEADER_MAX_SIZE;
			} else {
				ts_header[1] &= 0xBF; /* clear payload unit start indicator */
				ts_header_size = TS_HEADER_MAX_SIZE - 1;
			}
			ts_header[3] = ts_continuity_counter | 0x10; /* continuity counter, no scrambling, only payload */
			ts_continuity_counter = (ts_continuity_counter + 1) % 0x10; /* inc. continuity counter */
			ts_padded_did_not_output_next_header = 1;

        	}
		
	}
	
	return 0;
}
