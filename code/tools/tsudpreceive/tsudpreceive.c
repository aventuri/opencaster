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


#define MULTICAST

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#define UDP_MAXIMUM_SIZE 65535 /* theoretical maximum size */

int main(int argc, char* argv[]) {

    int sockfd;
    struct sockaddr_in addr;
    #ifdef ip_mreqn
     struct ip_mreqn mgroup;XXX
    #else
     /* according to
          http://lists.freebsd.org/pipermail/freebsd-current/2007-December/081080.html
        in bsd it is also possible to simply use ip_mreq instead of ip_mreqn
        (same as in Linux), so we are using this instead
     */
     struct ip_mreq mgroup;
    #endif
    int reuse;
    unsigned int addrlen;
    int len;
    unsigned char udp_packet[UDP_MAXIMUM_SIZE];


    if (argc != 3) {
	fprintf(stderr, "Usage: %s ip_addr port > output.ts\n", argv[0]);
	return 0;
    } else {
	memset((char *) &mgroup, 0, sizeof(mgroup));
	mgroup.imr_multiaddr.s_addr = inet_addr(argv[1]);
        #ifdef ip_mreqn
	 mgroup.imr_address.s_addr = INADDR_ANY;
        #else
         /* this is called 'interface' here */
	 mgroup.imr_interface.s_addr = INADDR_ANY;
        #endif
	memset((char *) &addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(argv[2]));
	addr.sin_addr.s_addr = inet_addr(argv[1]);
	addrlen = sizeof(addr);
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
	perror("socket(): error ");
	return 0;
    }
    
    reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) {
	perror("setsockopt() SO_REUSEADDR: error ");
    }

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
	perror("bind(): error");
	close(sockfd);
	return 0;
    }
    
    if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mgroup, sizeof(mgroup)) < 0) {
	perror("setsockopt() IPPROTO_IP: error ");
	close(sockfd);
	return 0;
    }

    while(1) {
	len = recvfrom(sockfd, udp_packet, UDP_MAXIMUM_SIZE, 0, (struct sockaddr *) &addr,&addrlen);
	if (len < 0) {
	    perror("recvfrom(): error ");
	} else {
	    write(STDOUT_FILENO, udp_packet, len);

	}
    }
    
}

