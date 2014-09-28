/*
 * carousel.c
 */

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

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>

#include "carousel.h"
#include "table.h"
#include "dsmcc.h"
#include "biop.h"
#include "utils.h"


int
load_carousel(char* paramDir, struct carousel *car, int cache_size)
{
	int result = 3;

	int processed;
	int index;
	int completed_modules;
	int i = 0;
	
	unsigned char table[MAX_TABLE_LEN];

	/* no modules yet */
	car->nmodules = 0;
	car->modules = NULL;
	
	/* tables cached */
	unsigned char* used_slot = safe_malloc(sizeof(unsigned char) * cache_size);
	unsigned char* cached = safe_malloc(MAX_TABLE_LEN * cache_size);
	memset(used_slot, 0, cache_size);

	/* see what the next DSMCC table is */
	do
	{
		struct dsmccMessageHeader *dsmcc;
		
		if (!car->completed) {
			if(read_dsmcc_tables(car, table) < 0) {
				fatal("Unable to read PID");
				result = 1; // timeout
				car->stop = 1;
			}
		} else { // dsmcc can update itself, keep on reading for dii/dsi changes until we get a complete file system of the same version
			if(read_dsi_dii_tables(car, table) < 0) {
				fatal("Unable to read PID");
				result = 1; // timeout
				car->stop = 1;				
			}
		}	
		if (!car->stop) {
			dsmcc = (struct dsmccMessageHeader *) &table[8];
			if(dsmcc->protocolDiscriminator == DSMCC_PROTOCOL && dsmcc->dsmccType == DSMCC_TYPE_DOWNLOAD)
			{
				if(ntohs(dsmcc->messageId) == DSMCC_MSGID_DII) {
				
					processed = process_dii(car, (struct DownloadInfoIndication *) dsmccMessage(dsmcc), ntohl(dsmcc->transactionId));
					if (processed) {
						verbose("Checking cached tables");
						unsigned char* cached_table;
						for (index = 0; index < cache_size; index++) {
							if (used_slot[index] == 1) {
								cached_table = cached + (index * MAX_TABLE_LEN);
								dsmcc = (struct dsmccMessageHeader *) &cached_table[8];
								processed = process_ddb(paramDir, car, (struct DownloadDataBlock *) dsmccMessage(dsmcc), ntohl(dsmcc->transactionId), DDB_blockDataLength(dsmcc));
								if (processed) {
									verbose("DDB processed from cache index: %d", index);
									used_slot[index] = 0;
								}
							}
						}
						car->completed = 0;
					} else if (car->nmodules > 0) {
						verbose("Check if all modules are completed, modules number is %d", car->nmodules);
						completed_modules = 0;
						for (i = 0; i < car->nmodules; i++) {
							if ((car->modules[i]).blocks_left == 0) {
								completed_modules++;
							}
							verbose("module id %d, blocks_left %d", (car->modules[i]).module_id, (car->modules[i]).blocks_left);
						}
						verbose("completed are %d", completed_modules);
						/* remove this to make it run endless */
						if (completed_modules == car->nmodules) {
							car->completed= 1;
							verbose("carousels completed, check dii update");
						}
						
					}
					
				} else if(ntohs(dsmcc->messageId) == DSMCC_MSGID_DSI) {
				
					process_dsi(paramDir, car, (struct DownloadServerInitiate *) dsmccMessage(dsmcc));
					
				} else if(ntohs(dsmcc->messageId) == DSMCC_MSGID_DDB) {
				
					processed = process_ddb(paramDir, car, (struct DownloadDataBlock *) dsmccMessage(dsmcc), ntohl(dsmcc->transactionId), DDB_blockDataLength(dsmcc));
					if (!processed) {
						for (index = 0; index < cache_size; index++) {
							if (used_slot[index] == 0) {
								verbose("DDB not processed, adding to cache, index is: %d", index);
								memcpy(cached + (index * MAX_TABLE_LEN), table, MAX_TABLE_LEN);
								used_slot[index] = 1;
								index = cache_size;
							}
						}
					}
					
				} else {
				
					error("Unknown DSMCC messageId: 0x%x", ntohs(dsmcc->messageId));
					
				}
			}
		}
	}
	while(!car->stop);
	
	verbose("Carousel stopped, doing cleanings ");

	for (i = 0; i < car->npids; i++) {
		if (car->pids[i].filterid >= 0) {
		    closePidFiltering(car->pids[i].filterid);
		}
	}

	verbose("descriptor closed ");
	
	for (i = 0; i < car->nmodules; i++) {
	    safe_free(car->modules[i].got_block);
	    safe_free(car->modules[i].data);
	}

	verbose("modules block free ");
	
	safe_free(car->modules);

	verbose("modules free ");
	
	safe_free(car->pids);

	verbose("pid free ");
	
	safe_free(used_slot);
	safe_free(cached);

	verbose("cache free ");

	return result;
}

int
process_dii(struct carousel *car, struct DownloadInfoIndication *dii, uint32_t transactionId)
{
	unsigned int nmodules;
	unsigned int i;
	int processed = 0;

	verbose("DownloadInfoIndication ");
	vverbose("transactionId: %u, ", transactionId);
	vverbose("downloadId: %u, ", ntohl(dii->downloadId));

	nmodules = DII_numberOfModules(dii);
	vverbose("numberOfModules: %u, ", nmodules);

	for(i=0; i<nmodules; i++)
	{
		struct DIIModule *mod;
		mod = DII_module(dii, i);
		vverbose("Module %u", i);
		vverbose(" moduleId: %u", ntohs(mod->moduleId));
		vverbose(" moduleVersion: %u", mod->moduleVersion);
		vverbose(" moduleSize: %u", ntohl(mod->moduleSize));
		if(find_module(car, ntohs(mod->moduleId), mod->moduleVersion, ntohl(dii->downloadId)) == NULL) {
			vverbose("Adding module %u", i);
			add_module(car, dii, mod);
			processed = 1;
		}
	}
	

	return processed;
}

void
process_dsi(char* paramDir, struct carousel *car, struct DownloadServerInitiate *dsi)
{
	uint16_t elementary_pid;

	verbose("DownloadServerInitiate");
	
	/* only download DSI from boot pid */
	/*
	if((car->current_pid != car->boot_pid) || car->got_dsi)
		return;
	*/

	if(car->got_dsi)
		return;
		
	/* actually should check for an updated version  */
	car->got_dsi = 1;

	elementary_pid = process_biop_service_gateway_info(paramDir, car->service_id, &car->assoc, DSI_privateDataByte(dsi), ntohs(dsi->privateDataLength));

	// make sure we are downloading data from the PID the DSI refers to 
	// add_dsmcc_pid(car, elementary_pid);

	return;
}

int 
process_ddb(char* paramDir, struct carousel *car, struct DownloadDataBlock *ddb, uint32_t downloadId, uint32_t blockLength)
{
	unsigned char *block;
	struct module *mod;
	int processed = 0;

	verbose("DownloadDataBlock ");
	vverbose("downloadId: %u", downloadId);
	verbose("moduleId: %u", ntohs(ddb->moduleId));
	vverbose("moduleVersion: %u", ddb->moduleVersion);
	verbose("blockNumber: %u", ntohs(ddb->blockNumber));
	vverbose("blockLength: %u", blockLength);

	block = DDB_blockDataByte(ddb);
	vhexdump(block, blockLength);

	if((mod = find_module(car, ntohs(ddb->moduleId), ddb->moduleVersion, downloadId)) != NULL) {
		download_block(paramDir, car, mod, ntohs(ddb->blockNumber), block, blockLength);
		processed = 1;
	} 
	
	return processed;
}

