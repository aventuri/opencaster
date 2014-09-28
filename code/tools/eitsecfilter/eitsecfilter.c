/*  
 * Copyright (C) 2010-2013  Lorenzo Pallara, l.pallara@avalpa.com
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

#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h> 
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <inttypes.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

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
	fprintf(stderr, "Usage: 'eitsecfilter file.sec +onid tsid sid +onid tsid sid [...]\n");
}

typedef struct Locator {
	unsigned short onid;
	unsigned short tsid;
	unsigned short sid;
} Locator;

#define MAX_LOCATORS 1024

int main(int argc, char *argv[])
{
	unsigned short a_section_size;
	unsigned char *a_section;
	int fd_sec;
	int fd_output;
	int i = 0;
	unsigned int onid;
	unsigned int tsid;
	unsigned int sid;
	unsigned int locators_num;
	
	char path[PATH_MAX];
	char old_path[PATH_MAX];
	Locator locator_table[MAX_LOCATORS];
	
	memset(locator_table, 0, sizeof(Locator) * MAX_LOCATORS);
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
	i = 2;
	locators_num = 0;
	while (i < argc) {
		if (argv[i][0] == '+') {
			locator_table[locators_num].onid =  atoi(&(argv[i][1]));
			locator_table[locators_num].tsid =  atoi(argv[i+1]);
			locator_table[locators_num].sid =  atoi(argv[i+2]);
			locators_num++;
			i+=3;
		}
	}
	
	while(1) {
		
		a_section = get_section(&a_section_size, fd_sec);
		if (a_section_size <= 0 || a_section == 0) {
			return 0;
		}
		
		
		memcpy(&onid, a_section + 10, 2);
		onid = ntohs(onid);
		memcpy(&tsid, a_section + 8, 2);
		tsid = ntohs(tsid);
		memcpy(&sid, a_section + 3, 2);
		sid = ntohs(sid);
		
		for (i = 0; i < locators_num; i++) {
			if (locator_table[i].onid == onid && locator_table[i].tsid == tsid && locator_table[i].sid == sid) {
				if (a_section[0] == 0x4e) { /* p/f -> 0 is present, 1 is following */
					if (a_section[6] == 0) {
						snprintf(path, PATH_MAX, "p.%d.%d.%d.sec", onid, tsid, sid);
					} else {
						snprintf(path, PATH_MAX, "f.%d.%d.%d.sec", onid, tsid, sid);
					}
					fd_output = open(path, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
				} else { /* scheduled */
					snprintf(path, PATH_MAX, "sc.%d.%d.%d.%d.sec", onid, tsid, sid, a_section[0]);
					if (a_section[6] == 0) {
						snprintf(old_path, PATH_MAX, "old_sc.%d.%d.%d.%d.sec", onid, tsid, sid, a_section[0]);
						rename(path, old_path);
						fd_output = open(path, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
					} else {
						fd_output = open(path, O_CREAT|O_WRONLY|O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
					}
				}
				
				if (fd_output >= 0) {
					write(fd_output, a_section, a_section_size);
					close(fd_output);
				}
			}
		}
		
		
	}
	
	return 0;
}

