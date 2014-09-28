/*
 * dsmcc.c
 *
 * helper functions for handling data types for ISO/IEC 13818-6 DSM-CC specs
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

#include <netinet/in.h>

#include "dsmcc.h"
#include "utils.h"

/*
 * return a ptr to the message following the dsmccMessageHeader
 */

unsigned char *
dsmccMessage(struct dsmccMessageHeader *hdr)
{
	unsigned char *byte = (unsigned char *) hdr;
	unsigned int offset;

	offset = sizeof(struct dsmccMessageHeader);
	offset += hdr->adaptionLength;

	return &byte[offset];
}

/*
 * return the numberOfModules field in a DownloadInfoIndication
 */

uint16_t
DII_numberOfModules(struct DownloadInfoIndication *dii)
{
	unsigned char *byte = (unsigned char *) dii;
	unsigned int offset;
       
	offset = sizeof(struct DownloadInfoIndication);
	offset += ntohs(dii->compatibilityDescriptor.compatibilityDescriptorLength);

	/* uint16_t numberOfModules */
	return (byte[offset] << 8) + byte[offset + 1];
}

/*
 * return info for module n in a DownloadInfoIndication
 * numbering starts at 0
 */

struct DIIModule *
DII_module(struct DownloadInfoIndication *dii, uint16_t n)
{
	unsigned char *byte = (unsigned char *) dii;
	unsigned int offset;
	struct DIIModule *mod;

	/* assert */
	if(n >= DII_numberOfModules(dii))
		fatal("DII_module: index too large");

	/* offset to first module */
	offset = sizeof(struct DownloadInfoIndication);
	offset += ntohs(dii->compatibilityDescriptor.compatibilityDescriptorLength);
	offset += sizeof(uint16_t);	/* numberOfModules */

	/* find the module we want */
	while(n != 0)
	{
		n --;
		mod = (struct DIIModule *) &byte[offset];
		offset += sizeof(struct DIIModule);
		offset += mod->moduleInfoLength;
	}

	return (struct DIIModule *) &byte[offset];
}

/*
 * returns the number of bytes of data in the DDB block
 */

uint32_t
DDB_blockDataLength(struct dsmccMessageHeader *hdr)
{
	uint32_t length;

	length = ntohs(hdr->messageLength);
	length -= hdr->adaptionLength;
	length -= sizeof(struct DownloadDataBlock);

	return length;
}

/*
 * returns a ptr to the data bytes in the DDB block
 */

unsigned char *
DDB_blockDataByte(struct DownloadDataBlock *ddb)
{
	return ((unsigned char *) ddb) + sizeof(struct DownloadDataBlock);
}

/*
 * returns a ptr to the private data bytes in the DSI block
 */

unsigned char *
DSI_privateDataByte(struct DownloadServerInitiate *dsi)
{
	/* assert */
	if(dsi->compatibilityDescriptor.compatibilityDescriptorLength != 0)
		fatal("Unexpected DSI format");

	return ((unsigned char *) dsi) + sizeof(struct DownloadServerInitiate);
}

