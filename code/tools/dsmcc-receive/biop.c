/*
 * biop.c
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
#include <limits.h>
#include <arpa/inet.h>

#include "biop.h"
#include "fs.h"
#include "assoc.h"
#include "table.h"
#include "utils.h"

/*
 * split the module into separate BIOP messages
 * returns false if the format is invalid
 */

bool
process_biop(char* paramDir, struct carousel *car, struct module *mod, struct BIOPMessageHeader *data, uint32_t size)
{
	uint32_t bytes_left;
	unsigned char *subhdr;
	struct biop_sequence key;
	struct biop_sequence kind;
	struct biop_sequence info;
	struct biop_sequence service_context;
	struct biop_sequence body;
	struct biop_sequence file;
	char *dirname;

	vverbose("Whole BIOP, size=%u", size);
	vhexdump((unsigned char *) data, size);

	/*
	 * we may get 0, 1 or more BIOP messages in a single block
	 * (Channel 4 sends us modules that uncompress to 0 bytes)
	 */
	bytes_left = size;
	while(bytes_left != 0)
	{
		/* assert */
		if(bytes_left < sizeof(struct BIOPMessageHeader)
		|| strncmp(data->magic, BIOP_MAGIC_STR, BIOP_MAGIC_LEN) != 0
		|| data->biop_version.major != BIOP_VSN_MAJOR
		|| data->biop_version.minor != BIOP_VSN_MINOR
		|| data->message_type != BIOP_MSG_TYPE)
		{
			error("Invalid BIOP header");
			return false;
		}
		size = biop_uint32(data->byte_order, data->message_size);
		vverbose("BIOP message_size=%u", size);
		if(bytes_left < sizeof(struct BIOPMessageHeader) + size)
		{
			error("Not enough BIOP data");
			return false;
		}
		/* process MessageSubHeader */
		subhdr = ((unsigned char *) data) + sizeof(struct BIOPMessageHeader);
		vhexdump(subhdr, size);
		subhdr += biop_sequence255(subhdr, &key);
		vverbose("objectKey:");
		vhexdump(key.data, key.size);
		subhdr += biop_sequence(data->byte_order, subhdr, &kind);
		vverbose("objectKind: '%.*s'", kind.size, kind.data);
		subhdr += biop_sequence65535(data->byte_order, subhdr, &info);
		vverbose("objectInfo:");
		vhexdump(info.data, info.size);
		subhdr += biop_sequence255(subhdr, &service_context);
		vverbose("serviceContextList:");
		vhexdump(service_context.data, service_context.size);
		subhdr += biop_sequence(data->byte_order, subhdr, &body);
		vverbose("messageBody: %u bytes", body.size);
		/* decode the message body, based on the objectKind field */
		if(strcmp(kind.data, BIOP_DIR) == 0)
		{
			/* a directory */
			verbose("DSM::Directory");
			dirname = make_dir(paramDir, kind.data, car->current_pid, mod->download_id, mod->module_id, key.data, key.size, mod->version);
			process_biop_dir(paramDir, data->byte_order, dirname, car, body.data, body.size);
		}
		else if(strcmp(kind.data, BIOP_SERVICEGATEWAY) == 0)
		{
			/* the service gateway is the root directory */
			verbose("DSM::ServiceGateway");
			dirname = make_dir(paramDir, kind.data, car->current_pid, mod->download_id, mod->module_id, key.data, key.size, mod->version);
			process_biop_dir(paramDir, data->byte_order, dirname, car, body.data, body.size);
		}
		else if(strcmp(kind.data, BIOP_FILE) == 0)
		{
			/* a file */
			verbose("DSM::File");
			(void) biop_sequence(data->byte_order, body.data, &file);
			vhexdump(file.data, file.size);
			save_file(paramDir, kind.data, car->current_pid, mod->download_id, mod->module_id, key.data, key.size, file.data, file.size, mod->version);
		}
		else if(strcmp(kind.data, BIOP_STREAM) == 0)
		{
			/* a stream */
			verbose("DSM::Stream");
			vhexdump(body.data, body.size);
			/*
			 * just save it for now
			 * could parse the Taps to make it easier for the browser
			 */
			save_file(paramDir, kind.data, car->current_pid, mod->download_id, mod->module_id, key.data, key.size, body.data, body.size, mod->version);
		}
		else if(strcmp(kind.data, BIOP_STREAMEVENT) == 0)
		{
			/* a stream event */
			verbose("BIOP::StreamEvent");
			vhexdump(body.data, body.size);
			/*
			 * just save it for now
			 * could parse it to make it easier for the browser
			 */
			save_file(paramDir, kind.data, car->current_pid, mod->download_id, mod->module_id, key.data, key.size, body.data, body.size, mod->version);
			
		}
		else
		{
			error("Ignoring unknown BIOP object: '%.*s'", kind.size, kind.data);
		}
		
		
		/* move onto the next */
		data = (struct BIOPMessageHeader *) (((unsigned char *) data) + sizeof(struct BIOPMessageHeader) + size);
		bytes_left -= sizeof(struct BIOPMessageHeader) + size;
	}

	return true;
}

/*
 * process the DSM::Directory message body
 */

void
process_biop_dir(char* paramDir, uint8_t byte_order, char *dirname, struct carousel *car, unsigned char *data, uint32_t size)
{
	uint16_t nbindings;
	uint16_t i;
	uint8_t nnames;
	uint8_t j;
	struct biop_sequence name;
	struct biop_sequence kind;
	uint8_t type;
	struct biop_iop_ior ior;
	struct biop_sequence info;
	uint16_t pid;

	nbindings = biop_uint16(byte_order, *((uint16_t *) data));
	data += 2;
	vverbose("binding_count: %u", nbindings);

	for(i=0; i<nbindings; i++)
	{
		vverbose(" binding %u", i);
		nnames = *data;
		data += 1;
		vverbose(" nameComponents: %u", nnames);
		/* only expecting 1 name, so just use the last one */
		for(j=0; j<nnames; j++)
		{
			data += biop_sequence255(data, &name);
			vverbose("  name %u: '%.*s'", j, name.size, name.data);
			data += biop_sequence255(data, &kind);
			vverbose("  kind %u: '%.*s'", j, kind.size, kind.data);
		}
		/* bindingType */
		type = *data;
		data += 1;
		vverbose(" bindingType: %u", type);
		/* objectRef */
		vverbose(" objectRef:");
		data += process_iop_ior(byte_order, data, &ior);
		/* make sure we are downloading the PID with this file on */
		pid = stream2pid(&car->assoc, ior.association_tag);
		/*
		 * is the PID on the MUX we are currently tuned to
		 * some BBC apps have links to files on different MUXes
		 * eg 'games' on BBC1
		 */
		if(pid != 0)
			add_dsmcc_pid(car, pid);
		add_dir_entry(paramDir, dirname, name.data, name.size, kind.data, pid, ior.carousel_id, ior.module_id, ior.key.data, ior.key.size);
		/* objectInfo */
		data += biop_sequence65535(byte_order, data, &info);
		vverbose(" objectInfo:");
		vhexdump(info.data, info.size);
	}

	return;
}

/*
 * returns the elementary_pid that maps to the association_tag in the IOP::IOR
 */

uint16_t
process_biop_service_gateway_info(char* paramDir, uint16_t service_id, struct assoc *assoc, unsigned char *data, uint16_t size)
{
	struct biop_iop_ior ior;
	uint16_t elementary_pid;

	verbose("BIOP::ServiceGatewayInfo");
	vhexdump(data, size);

	data += process_iop_ior(BIOP_BIGENDIAN, data, &ior);

	elementary_pid = stream2pid(assoc, ior.association_tag);

	make_service_root(paramDir, service_id, BIOP_SERVICEGATEWAY, elementary_pid, ior.carousel_id, ior.module_id, ior.key.data, ior.key.size);

	return elementary_pid;
}

/*
 * process an IOP::IOR data structure
 * stores the results in ior
 * returns the size in bytes
 */

uint32_t
process_iop_ior(uint8_t byte_order, unsigned char *data, struct biop_iop_ior *ior)
{
	unsigned char *start = data;
	struct biop_sequence type;
	uint32_t nprofiles;
	uint32_t i;
	uint32_t tag;
	struct biop_sequence profile;
	uint8_t profile_bo;
	uint8_t taps_count;
	uint32_t transaction_id;

	vverbose("IOP::IOR");
	/* typeId - "dir\0", "fil\0", etc */
	data += biop_sequence(byte_order, data, &type);
	vverbose("  typeId: '%.*s'", type.size, type.data);

	nprofiles = biop_uint32(byte_order, *((uint32_t *) data));
	data += 4;
	vverbose("  taggedProfiles_count: %u", nprofiles);
	for(i=0; i<nprofiles; i++)
	{
		vverbose("   IOP::taggedProfile %u", i);
		tag = biop_uint32(byte_order, *((uint32_t *) data));
		data += 4;
		data += biop_sequence(byte_order, data, &profile);
		if(tag == TAG_BIOP)
		{
			vverbose("   BIOPProfileBody:");
			/* profile_data_byte_order */
			profile_bo = *(profile.data);
			profile.data += 1;
			/* ncomponents = *(profile.data); */
			profile.data += 1;
			/* BIOP::ObjectLocation */
			if(biop_uint32(profile_bo, *((uint32_t *) profile.data)) != TAG_ObjectLocation)
				fatal("Expecting BIOP::ObjectLocation");
			profile.data += 4;
			/* component_data_length = *(profile.data); */
			profile.data += 1;
			/* carouselId */
			ior->carousel_id = biop_uint32(profile_bo, *((uint32_t *) profile.data));
			profile.data += 4;
			vverbose("    carouselId: %u", ior->carousel_id);
			/* moduleId */
			ior->module_id = biop_uint16(profile_bo, *((uint16_t *) profile.data));
			profile.data += 2;
			vverbose("    moduleId: %u", ior->module_id);
			/* BIOP version */
			if(profile.data[0] != BIOP_VSN_MAJOR
			|| profile.data[1] != BIOP_VSN_MINOR)
				fatal("Expecting BIOP version 1.0");
			profile.data += 2;
			/* objectKey */
			profile.data += biop_sequence255(profile.data, &ior->key);
			vverbose("    objectKey: '%.*s'", ior->key.size, ior->key.data);
			vhexdump(ior->key.data, ior->key.size);
			/* DSM::ConnBinder */
			if(biop_uint32(profile_bo, *((uint32_t *) profile.data)) != TAG_ConnBinder)
				fatal("Expecting DSM::ConnBinder");
			profile.data += 4;
			vverbose("    DSM::ConnBinder");
			/* component_data_length = *profile.data */
			profile.data += 1;
			taps_count = *profile.data;
			profile.data += 1;
			vverbose("    taps_count: %u", taps_count);
			if(taps_count > 0)
			{
				vverbose("    BIOP::Tap");
				/* id = biop_uint16(profile_bo, *((uint16_t *) profile.data)) */
				profile.data += 2;
				if(biop_uint16(profile_bo, *((uint16_t *) profile.data)) != BIOP_DELIVERY_PARA_USE)
					fatal("Expecting BIOP_DELIVERY_PARA_USE");
				profile.data += 2;
				vverbose("    use: BIOP_DELIVERY_PARA_USE");
				ior->association_tag = biop_uint16(profile_bo, *((uint16_t *) profile.data));
				profile.data += 2;
				vverbose("    association_tag: %u", ior->association_tag);
				if(*profile.data != SELECTOR_TYPE_MESSAGE_LEN)
					fatal("Expecting selector_length %u", SELECTOR_TYPE_MESSAGE_LEN);
				profile.data += 1;
				if(biop_uint16(profile_bo, *((uint16_t *) profile.data)) != SELECTOR_TYPE_MESSAGE)
					fatal("Expecting selector_type MESSAGE");
				profile.data += 2;
				transaction_id = biop_uint32(profile_bo, *((uint32_t *) profile.data));
				profile.data += 4;
				vverbose("    transaction_id: %u", transaction_id);
			}
		}
		else if(tag == TAG_LITE_OPTIONS)
		{
			fatal("TAG_LITE_OPTIONS not implemented");
		}
		else
		{
			fatal("Unknown IOP::IOR profileId_tag (0x%x)", tag);
		}
	}

	return (data - start);
}

/*
 * convert to the correct byte order
 */

uint16_t
biop_uint16(uint8_t byte_order, uint16_t raw)
{
	uint8_t *p = (uint8_t *) &raw;
	uint16_t val;

	if(byte_order == BIOP_BIGENDIAN)
		val = (p[0] << 8) + p[1];
	else
		val = (p[1] << 8) + p[0];

	return val;
}

/*
 * convert to the correct byte order
 */

uint32_t
biop_uint32(uint8_t byte_order, uint32_t raw)
{
	uint8_t *p = (uint8_t *) &raw;
	uint32_t val;

	if(byte_order == BIOP_BIGENDIAN)
		val = (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
	else
		val = (p[3] << 24) + (p[2] << 16) + (p[1] << 8) + p[0];

	return val;
}

/*
 * returns the number of bytes including the size field
 */

uint32_t
biop_sequence255(unsigned char *raw, struct biop_sequence *out)
{
	out->size = raw[0];
	out->data = &raw[1];

	return 1 + out->size;
}

/*
 * returns the number of bytes including the size field
 */

uint32_t
biop_sequence65535(uint8_t byte_order, unsigned char *raw, struct biop_sequence *out)
{
	out->size = biop_uint16(byte_order, *((uint16_t *) raw));
	out->data = &raw[2];

	return 2 + out->size;
}

/*
 * returns the number of bytes including the size field
 */

uint32_t
biop_sequence(uint8_t byte_order, unsigned char *raw, struct biop_sequence *out)
{
	out->size = biop_uint32(byte_order, *((uint32_t *) raw));
	out->data = &raw[4];

	return 4 + out->size;
}

/*
 * returns the number of bytes needed to round size upto the next 4 byte boundary
 */

uint32_t
biop_align32(uint32_t size)
{
	return (size % 4) != 0 ? (4 - (size % 4)) : 0;
}

