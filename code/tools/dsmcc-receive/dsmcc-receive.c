/*
 * Copyright (C) 2008-2013, Lorenzo Pallara l.pallara@avalpa.com
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
#include <unistd.h>
#include <stdint.h>
#include <errno.h>

#include "carousel.h"
#include "utils.h"
#include "table.h"

void usage(char *);

int
main(int argc, char *argv[])
{
    char* directory = 0;
    int cachesize = 0;
    int result = 0;
    int pid = 0;
    int component_tag = 0;
    
    if (argc != 5) {
	usage(argv[0]);
	return 0;
    } else {
	directory = argv[1];
	cachesize = atoi(argv[2]);
	pid = atoi(argv[3]);
	component_tag = atoi(argv[4]);
    }
    
    struct carousel a_carousel;
    
    a_carousel.timeout = 60; /* 1 minute, not used for dsmcc-receive */
    a_carousel.service_id = 1; /* not used for dsmcc-receive */
    a_carousel.carousel_id = 0; /* not used for dsmcc-receive */
    a_carousel.npids = 0;
    a_carousel.pids = NULL;
    a_carousel.got_dsi = 0;
    a_carousel.nmodules = 0;
    a_carousel.modules = NULL;
    a_carousel.stop = 0;
    a_carousel.completed = 0;
    a_carousel.current_pid = pid; /* not used for dsmcc-receive */
    add_dsmcc_pid(&a_carousel, pid); /* a carousel can be received from multiple PIDs, add the first we have found, this must carry DSI table or won't load */
    init_assoc(&(a_carousel.assoc)); 
    add_assoc(&(a_carousel.assoc), pid, component_tag); /* set the mapping between stream_id_descriptors and elementary_PIDs, not used for dsmcc-receive */
    
    set_verbose(1); /* 1, 2, 3 */    

    fprintf(stdout, "info: loading carousel on directory %s\n", directory);

    /*
     * the file structure will be:
     * directory/services/<service_id>
     * this is a symlink to the root of the carousel
     * the actual carousel files and directories are stored under:
     * directory/carousels/<PID>/<CID>/
     * where <PID> is the PID the carousel was downloaded from
     * and <CID> is the Carousel ID
     */
    
    result = load_carousel(directory, &(a_carousel), cachesize);

    clean_assoc(&(a_carousel.assoc)); 

    fprintf(stdout, "info: load carousel done\n");
    
    return result;
}

void
usage(char *prog_name)
{
	fatal("Usage: %s output_directory cache_size pid component_tag < file.sec", prog_name);
}

