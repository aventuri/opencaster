/*
 * module.c
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

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <zlib.h>
#include <netinet/in.h>

#include "dsmcc.h"
#include "carousel.h"
#include "biop.h"
#include "utils.h"

/*
 * returns NULL if the module does not exist
 * if this is an update to a module we already have, the old one is deleted
 */

struct module *
find_module(struct carousel *car, uint16_t module_id, uint8_t version, uint32_t download_id)
{
	unsigned int i;

	for(i=0; i<car->nmodules; i++)
	{
		if(car->modules[i].module_id == module_id
		&& car->modules[i].download_id == download_id)
		{
			/* spot on */
			if(car->modules[i].version == version)
			{
				return &car->modules[i];
			}
			/* is it an update to one we already have */
			else if(car->modules[i].version != version)
			{
				delete_module(car, i);
				return NULL;
			}
		}
	}

	return NULL;
}

struct module *
add_module(struct carousel *car, struct DownloadInfoIndication *dii, struct DIIModule *diimod)
{
	struct module *mod;

	car->nmodules ++;
	car->modules = safe_realloc(car->modules, car->nmodules * sizeof(struct module));
	mod = &car->modules[car->nmodules - 1];

	mod->module_id = ntohs(diimod->moduleId);
	mod->download_id = ntohl(dii->downloadId);
	mod->version = diimod->moduleVersion;
	mod->block_size = ntohs(dii->blockSize);
	mod->nblocks = (ntohl(diimod->moduleSize) + mod->block_size - 1) / mod->block_size;
	mod->blocks_left = mod->nblocks;
	mod->got_block = safe_malloc(mod->nblocks * sizeof(bool));
	bzero(mod->got_block, mod->nblocks * sizeof(bool));
	mod->size = ntohl(diimod->moduleSize);
	mod->data = safe_malloc(mod->size);

	verbose("add_module: nmodules=%u module=%u size=%u", car->nmodules, mod->module_id, mod->size);

	return mod;
}

void
delete_module(struct carousel *car, uint32_t num)
{
	uint32_t size;

	free_module(&car->modules[num]);

	size = ((car->nmodules - 1) - num) * sizeof(struct module);
	if(size > 0)
		memmove(&car->modules[num], &car->modules[num + 1], size);

	car->nmodules --;

	return;
}

void
free_module(struct module *mod)
{
	safe_free(mod->data);
	safe_free(mod->got_block);

	return;
}

void
download_block(char* paramDir, struct carousel *car, struct module *mod, uint16_t block, unsigned char *data, uint32_t length)
{
	/* assert */
	if(block >= mod->nblocks)
	{
		error("download_block: moduleId=%u block=%u nblocks=%u", mod->module_id, block, mod->nblocks);
		return;
	}

	/* have we already got it */
	if(mod->got_block[block])
		return;

	mod->got_block[block] = true;
	memcpy(mod->data + (block * mod->block_size), data, length);

	mod->blocks_left --;

	verbose("download_block: module=%u block=%u left=%u", mod->module_id, block, mod->blocks_left);

	/* have we got it all yet */
	if(mod->blocks_left == 0)
	{
		verbose("got module %u (size=%u)", mod->module_id, mod->size);
		/* if it doesn't start with 'BIOP' assume it is compressed */
		if(strncmp(mod->data, BIOP_MAGIC_STR, BIOP_MAGIC_LEN) != 0)
		{
			vhexdump(mod->data, mod->size);
			uncompress_module(mod);
			verbose("uncompressed size=%u", mod->size);
		}
		if(process_biop(paramDir, car, mod, (struct BIOPMessageHeader *) mod->data, mod->size))
		{
			/* we can free the data now, keep got_block so we don't download it again */
			safe_free(mod->data);
			/* delete_module may safe_free it again */
			mod->data = NULL;
		}
		else
		{
			/* failed to process it, try downloading it again */
			mod->blocks_left = mod->nblocks;
			/* assumes false == 0 */
			bzero(mod->got_block, mod->nblocks * sizeof(bool));
		}
	}

	return;
}

/*
 * uncompress in blocks of this size
 * this is also the minimum size of an uncompressed file
 */
#define CHUNK_SIZE	(4 * 1024)

int
uncompress_module(struct module *mod)
{
	int ret;
	z_stream strm;
	unsigned char *out = NULL;
	unsigned int out_size = 0;

	/* init zlib state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ret = inflateInit(&strm);
	if(ret != Z_OK)
		return ret;

	strm.avail_in = mod->size;
	strm.next_in = mod->data;
	/* decompress until ouput buffer not full */
	do
	{
		out = safe_realloc(out, out_size + CHUNK_SIZE);
		strm.avail_out = CHUNK_SIZE;
		strm.next_out = out + out_size;
		ret = inflate(&strm, Z_NO_FLUSH);
		if(ret != Z_OK && ret != Z_STREAM_END)
		{
			inflateEnd(&strm);
			safe_free(out);
			return ret;
		}
		out_size += CHUNK_SIZE - strm.avail_out;
	}
	while(strm.avail_out == 0);

	/* clean up */
	inflateEnd(&strm);

	/* swap compressed with uncompressed data */
	safe_free(mod->data);
	mod->data = out;
	mod->size = out_size;

	return Z_OK;
}

