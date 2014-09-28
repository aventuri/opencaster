/*
 * biop.h
 *
 * BIOP data types from ISO/IEC 13818-6
 */

/*
 * Copyright (C) 2005, Simon Kilvington
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

#ifndef __BIOP_H__
#define __BIOP_H__

#include <stdint.h>

#include "carousel.h"
#include "assoc.h"

struct BIOPVersion
{
	uint8_t major;
	uint8_t minor;
} __attribute__((__packed__));

struct BIOPMessageHeader
{
	char magic[4];				/* 'BIOP' */
	struct BIOPVersion biop_version;	/* 1.0 */
	uint8_t byte_order;			/* 0 = big endian */
	uint8_t message_type;			/* should be 0 */
	uint32_t message_size;			/* not including this header */
} __attribute__((__packed__));

/* magic header value */
#define BIOP_MAGIC_STR	"BIOP"
#define BIOP_MAGIC_LEN	4

/* version we are expecting */
#define BIOP_VSN_MAJOR	1
#define BIOP_VSN_MINOR	0

/* byte order */
#define BIOP_BIGENDIAN	0

/* message type we are expecting */
#define BIOP_MSG_TYPE	0

/* objectKind types */
#define BIOP_DIR		"dir"
#define BIOP_FILE		"fil"
#define BIOP_STREAM		"str"
#define BIOP_SERVICEGATEWAY	"srg"
#define BIOP_STREAMEVENT	"ste"

/* IOP::IOR profileId_tag values */
#define TAG_BIOP		0x49534f06
#define TAG_LITE_OPTIONS	0x49534f05

/* BIOP::ObjectLocation tag */
#define TAG_ObjectLocation	0x49534f50

/* DSM::ConnBinder tag */
#define TAG_ConnBinder		0x49534f40

/* use types */
#define BIOP_DELIVERY_PARA_USE	0x0016

/* selector_type we are expecting */
#define SELECTOR_TYPE_MESSAGE_LEN	0x0a
#define SELECTOR_TYPE_MESSAGE		0x0001

/* data type to hold a sequence */
struct biop_sequence
{
	uint32_t size;
	unsigned char *data;
};

/* data type to hold an IOP::IOR object reference */
struct biop_iop_ior
{
	uint16_t association_tag;	/* maps to an elementary_PID */
	uint32_t carousel_id;
	uint16_t module_id;
	struct biop_sequence key;
};

/* functions */
bool process_biop(char* paramDir, struct carousel *, struct module *, struct BIOPMessageHeader *, uint32_t);
void process_biop_dir(char* paramDir, uint8_t, char *, struct carousel *, unsigned char *, uint32_t);
uint32_t process_iop_ior(uint8_t, unsigned char *, struct biop_iop_ior *);
uint16_t process_biop_service_gateway_info(char* paramDir, uint16_t, struct assoc *, unsigned char *, uint16_t);

uint16_t biop_uint16(uint8_t, uint16_t);
uint32_t biop_uint32(uint8_t, uint32_t);

uint32_t biop_sequence255(unsigned char *, struct biop_sequence *);
uint32_t biop_sequence65535(uint8_t, unsigned char *, struct biop_sequence *);
uint32_t biop_sequence(uint8_t, unsigned char *, struct biop_sequence *);

uint32_t biop_align32(uint32_t);

#endif

