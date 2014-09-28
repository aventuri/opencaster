/*
 * dsmcc.h
 *
 * data types for ISO/IEC 13818-6 DSM-CC specs
 *
 * all values are big endian
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

#ifndef __DSMCC_H__
#define __DSMCC_H__

#include <stdint.h>

/*
 * header for all DSM-CC messages
 */

struct dsmccMessageHeader
{
	uint8_t protocolDiscriminator;		/* 0x11 */
	uint8_t dsmccType;
	uint16_t messageId;
	uint32_t transactionId;			/* downloadId for dsmccDownloadDataHeader */
	uint8_t reserved;			/* 0xff */
	uint8_t adaptionLength;
	uint16_t messageLength;			/* includes adaptionLength */
	/* dsmccAdaptionHeader, if adaptionLength > 0 */
	/* (messageLength - adaptionLength) bytes of message data */
} __attribute__((__packed__));

/* protocolDiscriminator */
#define DSMCC_PROTOCOL	0x11

/* dsmccType values we are interested in */
#define DSMCC_TYPE_DOWNLOAD	0x03

/* messageId values for DSMCC_TYPE_DOWNLOAD we are interested in */
/* control messages - have a dsmccMessageHeader */
#define DSMCC_MSGID_DII		0x1002	/* DownloadInfoIndication */
#define DSMCC_MSGID_DSI		0x1006	/* DownloadServerInitiate */
/* data messages - have a dsmccDownloadDataHeader */
#define DSMCC_MSGID_DDB		0x1003	/* DownloadDataBlock */

/* helper function */
unsigned char *dsmccMessage(struct dsmccMessageHeader *);

/*
 * CompatibilityDescriptor
 * used in various messages
 */

struct CompatibilityDescriptor
{
	uint16_t compatibilityDescriptorLength;
	/* uint8_t data[compatibilityDescriptorLength] */
} __attribute__((__packed__));

/*
 * DownloadInfoIndication message
 * preceeded by a dsmccMessageHeader
 * the transactionId is used as a version number,
 * if it changes the data has changed and must be downloaded again
 */

struct DownloadInfoIndication
{
	uint32_t downloadId;	/* to match DDB messages */
	uint16_t blockSize;
	uint8_t windowSize;
	uint8_t ackPeriod;
	uint32_t tCDownloadWindow;
	uint32_t tCDownloadScenario;
	struct CompatibilityDescriptor compatibilityDescriptor;
	/* uint16_t numberOfModules */
	/* struct DIIModule module[numberOfModules] */
	/* unit16_t privateDataLength */
	/* uint8_t privateDataByte[privateDataLength] */
} __attribute__((__packed__));

struct DIIModule
{
	uint16_t moduleId;
	uint32_t moduleSize;
	uint8_t moduleVersion;
	uint8_t moduleInfoLength;
	/* uint8_t moduleInfoByte[moduleInfoLength] */
} __attribute__((__packed__));

/* helper functions */
uint16_t DII_numberOfModules(struct DownloadInfoIndication *);
struct DIIModule *DII_module(struct DownloadInfoIndication *, uint16_t);

/*
 * DownloadDataBlock message
 * preceeded by a dsmccDownloadDataHeader
 * (same as a dsmccMessageHeader except the transactionId is called downloadId)
 */

struct DownloadDataBlock
{
	uint16_t moduleId;
	uint8_t moduleVersion;
	uint8_t reserved;		/* 0xff */
	uint16_t blockNumber;		/* starts at 0 */
	/* uint8_t blockDataByte[N] size determined by header.messageLength */
} __attribute__((__packed__));

/* helper functions */
uint32_t DDB_blockDataLength(struct dsmccMessageHeader *);
unsigned char *DDB_blockDataByte(struct DownloadDataBlock *);

/*
 * DownloadServerInitiate message
 * preceeded by a dsmccMessageHeader
 */

struct DownloadServerInitiate
{
	/* ETSI say serverId should be 20 0xff bytes */
	uint8_t serverId[20];
	/* ETSI say the compatibilityDescriptorLength should be 0 */
	struct CompatibilityDescriptor compatibilityDescriptor;
	/* ETSI say the privateData is a BIOP::ServiceGatewayInfo */
	uint16_t privateDataLength;
	/* uint8_t privateDataByte[privateDataLength] */
} __attribute__((__packed__));

/* helper */
unsigned char *DSI_privateDataByte(struct DownloadServerInitiate *);

#endif	/* __DSMCC_H__ */

