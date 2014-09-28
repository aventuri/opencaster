/*
 * assoc.c
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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "assoc.h"
#include "utils.h"

void
init_assoc(struct assoc *a)
{
	a->nassocs = 0;
	a->pids = NULL;
	a->sids = NULL;

	return;
}

void
add_assoc(struct assoc *a, uint16_t elementary_pid, uint16_t stream_id)
{
	a->nassocs ++;

	a->pids = safe_realloc(a->pids, a->nassocs * sizeof(uint16_t));
	a->sids = safe_realloc(a->sids, a->nassocs * sizeof(uint16_t));

	a->pids[a->nassocs - 1] = elementary_pid;
	a->sids[a->nassocs - 1] = stream_id;

	return;
}

uint16_t
stream2pid(struct assoc *a, uint16_t stream_id)
{
	unsigned int i;

	for(i=0; i<a->nassocs; i++)
	{
		if(a->sids[i] == stream_id)
			return a->pids[i];
	}

	error("Unknown PID for association tag 0x%x", stream_id);

	return 0;
}


void clean_assoc(struct assoc *a)
{
    if (a != 0) {
	if (a->pids != 0) {
	    safe_free(a->pids);
	}
	if (a->sids != 0) {
	    safe_free(a->sids);
	}
    }

}
