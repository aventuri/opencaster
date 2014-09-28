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
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define TS_PACKET_SIZE 188

long long int usecDiff(struct timespec* time_stop, struct timespec* time_start)
{
	long long int temp = 0;
	long long int utemp = 0;
		   
	if (time_stop && time_start) {
		if (time_stop->tv_nsec >= time_start->tv_nsec) {
			utemp = time_stop->tv_nsec - time_start->tv_nsec;    
			temp = time_stop->tv_sec - time_start->tv_sec;
		} else {
			utemp = time_stop->tv_nsec + 1000000000 - time_start->tv_nsec;       
			temp = time_stop->tv_sec - 1 - time_start->tv_sec;
		}
		if (temp >= 0 && utemp >= 0) {
			temp = (temp * 1000000000) + utemp;
        	} else {
			fprintf(stderr, "start time %ld.%ld is after stop time %ld.%ld\n", time_start->tv_sec, time_start->tv_nsec, time_stop->tv_sec, time_stop->tv_nsec);
			temp = -1;
		}
	} else {
		fprintf(stderr, "memory is garbaged?\n");
		temp = -1;
	}
        return temp / 1000;
}


int main (int argc, char *argv[]) {
    
    int sockfd;
    int len;
    int sent;
    int ret;
    int is_multicast;
    int transport_fd;
    unsigned char option_ttl;
    char start_addr[4];
    struct sockaddr_in addr;
    unsigned long int packet_size;   
    char* tsfile;
    unsigned char* send_buf;
    unsigned int bitrate;
    unsigned long long int packet_time;
    unsigned long long int real_time;    
    struct timespec time_start;
    struct timespec time_stop;
    struct timespec nano_sleep_packet;
    
    memset(&addr, 0, sizeof(addr));
    memset(&time_start, 0, sizeof(time_start));
    memset(&time_stop, 0, sizeof(time_stop));
    memset(&nano_sleep_packet, 0, sizeof(nano_sleep_packet));

    if(argc < 5 ) {
	fprintf(stderr, "Usage: %s file.ts ipaddr port bitrate [ts_packet_per_ip_packet] [udp_packet_ttl]\n", argv[0]);
	fprintf(stderr, "ts_packet_per_ip_packet default is 7\n");
	fprintf(stderr, "bit rate refers to transport stream bit rate\n");
	fprintf(stderr, "zero bitrate is 100.000.000 bps\n");
	return 0;
    } else {
	tsfile = argv[1];
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(argv[2]);
	addr.sin_port = htons(atoi(argv[3]));
	bitrate = atoi(argv[4]);
	if (bitrate <= 0) {
	    bitrate = 100000000;
	}
	if (argc >= 6) {
	    packet_size = strtoul(argv[5], 0, 0) * TS_PACKET_SIZE;
	} else {
	    packet_size = 7 * TS_PACKET_SIZE;
	}
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0) {
	perror("socket(): error ");
	return 0;
    } 
    
    if (argc >= 7)  {
	option_ttl = atoi(argv[6]);
	is_multicast = 0;
	memcpy(start_addr, argv[2], 3);
	start_addr[3] = 0;
	is_multicast = atoi(start_addr);
	is_multicast = (is_multicast >= 224) || (is_multicast <= 239);
	if (is_multicast) {
	    ret = setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, &option_ttl, sizeof(option_ttl));
	} else {
	    ret = setsockopt(sockfd, IPPROTO_IP, IP_TTL, &option_ttl, sizeof(option_ttl));
	}
	
	if(ret < 0) {
	    perror("ttl configuration fail");
	}
	
    }
    
    transport_fd = open(tsfile, O_RDONLY);
    if(transport_fd < 0) {
	fprintf(stderr, "can't open file %s\n", tsfile);
	close(sockfd);
	return 0;
    } 
    
    int completed = 0;
    send_buf = malloc(packet_size);
    packet_time = 0;
    real_time = 0;
    
    nano_sleep_packet.tv_nsec = 665778; /* 1 packet at 100mbps*/

    clock_gettime(CLOCK_MONOTONIC, &time_start);
    
    while (!completed) {
    
	    clock_gettime(CLOCK_MONOTONIC, &time_stop);
	    real_time = usecDiff(&time_stop, &time_start);
	    while (real_time * bitrate > packet_time * 1000000 && !completed) { /* theorical bits against sent bits */
		len = read(transport_fd, send_buf, packet_size);
		if(len < 0) {
		    fprintf(stderr, "ts file read error \n");
		    completed = 1;
		} else if (len == 0) {
		    fprintf(stderr, "ts sent done\n");
	    	    completed = 1;
		} else {
		    sent = sendto(sockfd, send_buf, len, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
		    if(sent <= 0) {
			perror("send(): error ");
			completed = 1;
		    } else {
			packet_time += packet_size * 8;
		    }
		}
	    }
	    nanosleep(&nano_sleep_packet, 0);
    }

    close(transport_fd);
    close(sockfd);
    free(send_buf);
    return 0;    
}

