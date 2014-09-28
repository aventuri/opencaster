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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SERVER_PORT 12345
#define TS_PACKET_SIZE 188
#define BUF_SIZE TS_PACKET_SIZE * 7

int main(int argc, char *argv[]) {

    int i;
    int j;
    int port;
    int rc;
    int listen_sd;
    int accept_sd;
    char buffer[BUF_SIZE];
    char ts_packet[TS_PACKET_SIZE];
    struct sockaddr_in addr;
     
    if(argc >= 2) {
	port = atoi(argv[1]);
    } else {
	fprintf(stderr, "Usage: %s port\n", argv[0]);
	return 0;
    }
    
    listen_sd = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_sd < 0) {
	fprintf(stderr, "socket() error\n");
	return 0;
   }

    int reuse = 1;
    if (setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
	perror("setsockopt() SO_REUSEADDR: error ");
    }

/*
    if (setsockopt(listen_sd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
	perror("setsockopt() SO_REUSEPORT: error ");
    }
*/

    fprintf(stderr, "Binding the socket...\n");
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    rc = bind(listen_sd, (struct sockaddr *)&addr, sizeof(addr));
    if(rc < 0) {
	fprintf(stderr, "bind() error\n");
	close(listen_sd);
	return 0;
    } 

    rc = listen(listen_sd, 1);
    if(rc < 0) {
        fprintf(stderr, "listen() error\n");
        close(listen_sd);
        return 0;
     } 


    accept_sd = accept(listen_sd, NULL, NULL);
    if(accept_sd < 0) {
	fprintf(stderr, "accept() error\n");
        close(listen_sd);
        return 0;
    }
      
    rc = 1;
    j = 0;
    i = 0;
    while (rc > 0) {
	rc = recv(accept_sd, buffer, BUF_SIZE, 0);	
	for (i = 0; i < rc; i++) {
	    ts_packet[j] = buffer[i];
	    j++;
	    if (j == TS_PACKET_SIZE) {
    	        write(STDOUT_FILENO, ts_packet, TS_PACKET_SIZE);
		j = 0;
	    }
	}
    }
    
    
    close(accept_sd);
    close(listen_sd);
    return 0;
    
}
