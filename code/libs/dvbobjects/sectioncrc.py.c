/*
  sectioncrc.py.c -- MPEG-2 checksum calculation, Python Module

  Copyright (C) 2001 Oleg Tchekoulaev, GMD

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <Python.h>

#include "../sectioncrc/sectioncrc.c"

unsigned int sectioncrc( unsigned char*, unsigned int );
static PyObject* do_sectioncrc( PyObject*, PyObject* );
static PyMethodDef methods [] = {
    { "sectioncrc", do_sectioncrc, METH_VARARGS },
    {NULL, NULL}
};


void init_crc32(void) {
    Py_InitModule( "_crc32", methods );
}


static PyObject* do_sectioncrc( PyObject *self, PyObject *args ) {
    
    unsigned int c;
    char *buf;
    int len;

    if( !PyArg_ParseTuple( args, "s#", &buf, &len ) ) 
	return NULL;

    c = sectioncrc( buf, len );

    return Py_BuildValue( "I", c );
}
