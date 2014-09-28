/*
 * fs.c
 *
 * file system interactions
 * files get stored under a directory named after their carousel ID
 * the filename includes the module ID and the object key
 * when we download a directory we create a directory in our carousel directory
 * and fill it with sym links to the actual files and dirs it contains
 * (the sym links maybe dangling if we have not downloaded all the files yet)
 *
 * we also create a symlink that points to the Service Gateway dir
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

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "fs.h"
#include "biop.h"
#include "utils.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static char _carousel_root[PATH_MAX];

char *
make_carousel_root(char * paramDir, uint16_t elementary_pid, uint32_t carousel_id)
{

	/* make the hierarchy */
	snprintf(_carousel_root, sizeof(_carousel_root), "%s/%s", paramDir, CAROUSELS_DIR);
	if(mkdir(_carousel_root, 0755) < 0 && errno != EEXIST)
		fatal("Unable to create carousel directory '%s': %s", _carousel_root, strerror(errno));

	snprintf(_carousel_root, sizeof(_carousel_root), "%s/%s/%u", paramDir, CAROUSELS_DIR, elementary_pid);
	if(mkdir(_carousel_root, 0755) < 0 && errno != EEXIST)
		fatal("Unable to create carousel directory '%s': %s", _carousel_root, strerror(errno));

	snprintf(_carousel_root, sizeof(_carousel_root), "%s/%s/%u/%u", paramDir, CAROUSELS_DIR, elementary_pid, carousel_id);
	if(mkdir(_carousel_root, 0755) < 0 && errno != EEXIST)
		fatal("Unable to create carousel directory '%s': %s", _carousel_root, strerror(errno));

	return _carousel_root;
}


void
save_file(char* paramDir, char *kind, uint16_t elementary_pid, uint32_t carousel_id, uint16_t module_id, char *key, uint32_t key_size, char *file, uint32_t file_size, uint8_t module_version)
{
	char *root;
	char *ascii_key;
	char filename[PATH_MAX];
	FILE *f;

	/* make sure the carousel directory exists */
	root = make_carousel_root(paramDir, elementary_pid, carousel_id);

	/* BBC use numbers as object keys, so convert to a text value we can use as a file name */
	ascii_key = convert_key(key, key_size);

	/* construct the file name */
	snprintf(filename, sizeof(filename), "%s/%s-%u-%s", root, kind, module_id, ascii_key);

	if((f = fopen(filename, "wb")) == NULL)
		fatal("Unable to create file '%s': %s", filename, strerror(errno));
	if(fwrite(file, 1, file_size, f) != file_size)
		fatal("Unable to write to file '%s'", filename);

	fclose(f);

	verbose("Created file '%s'\n", filename);

	snprintf(filename, sizeof(filename), "%s/%s-%u-%s.version", root, kind, module_id, ascii_key);
	if((f = fopen(filename, "wb")) == NULL) 
		fatal("Unable to create file '%s': %s", filename, strerror(errno));
	if(fwrite(&module_version, 1, 1, f) != 1)
		fatal("Unable to write to file '%s'", filename);
	fclose(f);


	return;
}

/*
 * creates the symlink services/<service_id>
 * it points to the given Service Gateway object
 */

void
make_service_root(char* paramDir, uint16_t service_id, char *kind, uint16_t elementary_pid, uint32_t carousel_id, uint16_t module_id, char *key, uint32_t key_size)
{
	char dirname[PATH_MAX];
	char *ascii_key;
	char realfile[PATH_MAX];
	char linkfile[PATH_MAX];

	/* make sure the services directory exists */
	snprintf(dirname, sizeof(dirname), "%s", paramDir);
	if(mkdir(dirname, 0755) < 0 && errno != EEXIST)
		fatal("Unable to create services directory '%s': %s", dirname, strerror(errno));
	snprintf(dirname, sizeof(dirname), "%s/%s", paramDir, SERVICES_DIR);
	if(mkdir(dirname, 0755) < 0 && errno != EEXIST)
		fatal("Unable to create services directory '%s': %s", dirname, strerror(errno));

	/* BBC use numbers as object keys, so convert to a text value we can use as a file name */
	ascii_key = convert_key(key, key_size);

	/* create a symlink to the Service Gateway dir */
	if (paramDir[0] == '/') {
		snprintf(realfile, sizeof(realfile), "../../%s/%s/%u/%u/%s-%u-%s", paramDir+1, CAROUSELS_DIR, elementary_pid, carousel_id, kind, module_id, ascii_key);
	} else {
		snprintf(realfile, sizeof(realfile), "../../%s/%s/%u/%u/%s-%u-%s", paramDir, CAROUSELS_DIR, elementary_pid, carousel_id, kind, module_id, ascii_key);
	}
	snprintf(linkfile, sizeof(linkfile), "%s/%u", dirname, service_id);

	/*
	 * linkfile may already exist if we get an update to an existing module
	 * if linkfile already exists, symlink will not update it
	 * so delete it first to make sure the link gets updated
	 */
	unlink(linkfile);
	if(symlink(realfile, linkfile) < 0 && errno != EEXIST)
		fatal("Unable to create link '%s' to '%s': %s", linkfile, realfile, strerror(errno));

	verbose("Added service root '%s' -> '%s'\n", linkfile, realfile);

	return;
}

/*
 * returns a ptr to the directory name
 * the name will be overwritten by the next call to this routine
 */

static char _dirname[PATH_MAX];

char *
make_dir(char* paramDir, char *kind, uint16_t elementary_pid, uint32_t carousel_id, uint16_t module_id, char *key, uint32_t key_size, uint8_t module_version)
{
	char *root;
	char *ascii_key;
	FILE *f;
	char filename[PATH_MAX];

	/* make sure the carousel directory exists */
	root = make_carousel_root(paramDir, elementary_pid, carousel_id);

	/* BBC use numbers as object keys, so convert to a text value we can use as a file name */
	ascii_key = convert_key(key, key_size);

	snprintf(_dirname, sizeof(_dirname), "%s/%s-%u-%s", root, kind, module_id, ascii_key);

	/* may already exist if we get an updated version of it */
	if(mkdir(_dirname, 0755) < 0 && errno != EEXIST)
		fatal("Unable to create directory '%s': %s", _dirname, strerror(errno));

	verbose("Created directory '%s'\n", _dirname);

	snprintf(filename, sizeof(filename), "%s/%s-%u-%s.version", root, kind, module_id, ascii_key);
	if((f = fopen(filename, "wb")) == NULL) 
		fatal("Unable to create file '%s': %s", filename, strerror(errno));
	if(fwrite(&module_version, 1, 1, f) != 1)
		fatal("Unable to write to file '%s'", filename);
	fclose(f);

	return _dirname;
}

/*
 * adds a sym link to the real file to the given directory
 * the real file may not exist if it has not been downloaded yet
 */

void
add_dir_entry(char* paramDir, char *dir, char *entry, uint32_t entry_size, char *kind, uint16_t elementary_pid, uint32_t carousel_id, uint16_t module_id, char *key, uint32_t key_size)
{
	char *ascii_key;
	char realfile[PATH_MAX];
	char linkfile[PATH_MAX];

	/* BBC use numbers as object keys, so convert to a text value we can use as a file name */
	ascii_key = convert_key(key, key_size);

	if (paramDir[0] == '/') {
		snprintf(realfile, sizeof(realfile), "../../../../../%s/%s/%u/%u/%s-%u-%s", paramDir + 1, CAROUSELS_DIR, elementary_pid, carousel_id, kind, module_id, ascii_key);
	} else {
		snprintf(realfile, sizeof(realfile), "../../../../../%s/%s/%u/%u/%s-%u-%s", paramDir, CAROUSELS_DIR, elementary_pid, carousel_id, kind, module_id, ascii_key);
	}	
	snprintf(linkfile, sizeof(linkfile), "%s/%.*s", dir, entry_size, entry);

	/*
	 * linkfile may already exist if we get an update to an existing module
	 * if linkfile already exists, symlink will not update it
	 * so delete it first to make sure the link gets updated
	 */
	unlink(linkfile);
	if(symlink(realfile, linkfile) < 0)
		fatal("Unable to create link '%s' to '%s': %s", linkfile, realfile, strerror(errno));

	verbose("Added directory entry '%s' -> '%s'\n", linkfile, realfile);

	return;
}

/*
 * BBC use numbers as object keys, ITV/C4 use ascii strings
 * we want an ascii string we can use as a filename
 * returns a static string that will be overwritten by the next call to this function
 */

static char _ascii_key[PATH_MAX];

char *
convert_key(char *key, uint32_t size)
{
	uint32_t i;

	/* assert */
	if(sizeof(_ascii_key) < (size * 2) + 1)
		fatal("objectKey too long");

	/* convert to a string of hex byte values */
	for(i=0; i<size; i++)
	{
		_ascii_key[i * 2] = hex_digit((key[i] >> 4) & 0x0f);
		_ascii_key[(i * 2) + 1] = hex_digit(key[i] & 0x0f);
	}
	_ascii_key[size * 2] = '\0';

	return _ascii_key;
}

