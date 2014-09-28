/*
 * fs.h
 *
 * file system interactions
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

#ifndef __FS_H__
#define __FS_H__

/* directories */
#define SERVICES_DIR	"services"
#define CAROUSELS_DIR	"carousels"

char *make_carousel_root(char *, uint16_t, uint32_t);

void save_file(char*, char *, uint16_t, uint32_t, uint16_t, char *, uint32_t, char *, uint32_t, uint8_t);

void make_service_root(char *, uint16_t, char *, uint16_t, uint32_t, uint16_t, char *, uint32_t);

char *make_dir(char *,char *, uint16_t, uint32_t, uint16_t, char *, uint32_t, uint8_t);
void add_dir_entry(char *, char *, char *, uint32_t, char *, uint16_t, uint32_t, uint16_t, char *, uint32_t);

char *convert_key(char *, uint32_t);

#endif	/* __FS_H__ */

