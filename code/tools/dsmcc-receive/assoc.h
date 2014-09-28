/*
 * assoc.h
 *
 * maps stream_id_descriptors to elementary_PID numbers
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

#ifndef __ASSOC_H__
#define __ASSOC_H__

#include <stdint.h>

struct assoc
{
	unsigned int nassocs;
	uint16_t *pids;
	uint16_t *sids;
};

void init_assoc(struct assoc *);
void add_assoc(struct assoc *, uint16_t, uint16_t);
void clean_assoc(struct assoc *);

uint16_t stream2pid(struct assoc *, uint16_t);

#endif	/* __ASSOC_H__ */

