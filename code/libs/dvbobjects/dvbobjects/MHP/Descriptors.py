#! /usr/bin/env python

# This file is part of the dvbobjects library.
# 
# Copyright  2000-2001, GMD, Sankt Augustin
# -- German National Research Center for Information Technology 
#
# Copyright 2010, LIFIA - Facultad de Informatica - Univ. Nacional de La Plata
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

import string
from dvbobjects.utils import *
from dvbobjects.MPEG.Descriptor import Descriptor

######################################################################
# Common constants.

# application_type
DVB_J_application_type = 0x0001
DVB_HTML_application_type = 0x0002

# application_control_code
DVB_J_AUTOSTART = 0x01

# protocol_id
MHP_OC_protocol_id = 0x0001

######################################################################
class application_descriptor(Descriptor):
    
    descriptor_tag = 0x00
    application_profiles_length = 5

    def bytes(self):
        tp_labels = string.join(
            map(lambda label, self=self: pack("!B", label),
                self.transport_protocol_labels),
            "")

        self.flags = (
            0x00
            | (self.service_bound_flag << 7)
            | (self.visibility << 5)
            | 0x1F
            )

        fmt = "!BH5B%ds" % len(tp_labels)
        return pack(fmt,
                    self.application_profiles_length,
                    self.application_profile,
                    self.version_major,
                    self.version_minor,
                    self.version_micro,
                    self.flags,
                    self.application_priority,
                    tp_labels,
                    )

######################################################################
class application_name_descriptor(Descriptor):

    descriptor_tag = 0x01

    def bytes(self):
        fmt = "!3sB%ds" % len(self.application_name)
        return pack(fmt,
                    self.ISO_639_language_code,
                    len(self.application_name),
                    self.application_name,
                    )

######################################################################
class transport_protocol_descriptor(Descriptor):

    descriptor_tag = 0x02

    def bytes(self):
        fmt = "!HBBB"
        return pack(fmt,
                    self.protocol_id,
                    self.transport_protocol_label,
                    0x7F | (self.remote_connection << 7),
                    self.component_tag,
                    )

######################################################################
class transport_ic_protocol_descriptor(Descriptor):

    descriptor_tag = 0x02
    protocol_id = 0x03

    def bytes(self):

        URL_extension_count = 0
	
        for url in self.URL_extensions:
            URL_extension_count = URL_extension_count + 1

        fmt = "!HBB%dsB" % len(self.URL_base)
        result = pack(fmt,
                    self.protocol_id,
                    self.transport_protocol_label,
		    len(self.URL_base),
		    self.URL_base,
		    URL_extension_count
                    )
    
        for url in self.URL_extensions:
            result = result + pack(
                "!B%ds" % len(url),
                len(url),
                url,
                )
		
	return result    


######################################################################
class application_storage_descriptor(Descriptor):

   descriptor_tag = 0x10

   def bytes(self):
       fmt = "!BBLB"
       return pack(fmt,
                   self.storage_property,
                   0x1F | (self.not_launchable_from_broadcast << 7) |
                   (self.launchable_completely_from_cache << 6) |
                   (self.is_launchable_with_older_version << 5),
                   0x80000000 | (self.version),
                   self.priority,
                   )

######################################################################
class dvb_html_application_descriptor(Descriptor):

    descriptor_tag = 0x08

    def bytes(self):

#TODO add applications_ids	
    
        fmt = "!BH%ds" % len(self.parameter)
        return pack(fmt, 1 ,1 , self.parameter)


######################################################################
class dvb_html_application_location_descriptor(Descriptor):

    descriptor_tag = 0x09

    def bytes(self):
        fmt = "!B%ds%ds" % (
            len(self.physical_root),
            len(self.initial_path),
            )

        return pack(fmt,
                    len(self.physical_root),
                    self.physical_root,
                    self.initial_path,
                    )                    

    
######################################################################
class dvb_j_application_descriptor(Descriptor):

    descriptor_tag = 0x03

    def bytes(self):
        result = ""
        
        for param in self.parameters:
            result = result + pack(
                "!B%ds" % len(param),
                len(param),
                param,
                )

        return result

######################################################################
class dvb_simple_application_location_descriptor(Descriptor):

    descriptor_tag = 0x15

    def bytes(self):
        fmt = "!%ds" % ( len(self.path) )
        return pack(fmt,
                    self.path,
                    )


######################################################################
class dvb_j_application_location_descriptor(Descriptor):

    descriptor_tag = 0x04

    def bytes(self):
        fmt = "!B%dsB%ds%ds" % (
            len(self.base_directory),
            len(self.class_path_extension),
            len(self.initial_class),
            )

        return pack(fmt,
                    len(self.base_directory),
                    self.base_directory,
                    len(self.class_path_extension),
                    self.class_path_extension,
                    self.initial_class,
                    )                    

######################################################################
class ginga_ncl_application_descriptor(Descriptor):

    descriptor_tag = 0x06

    def bytes(self):
        result = ""
        
        for param in self.parameters:
            result = result + pack(
                "!B%ds" % len(param),
                len(param),
                param,
                )

        return result

######################################################################
class ginga_ncl_application_location_descriptor(Descriptor):

    descriptor_tag = 0x07

    def bytes(self):
        fmt = "!B%dsB%ds%ds" % (
            len(self.base_directory),
            len(self.class_path_extension),
            len(self.initial_class),
            )

        return pack(fmt,
                    len(self.base_directory),
                    self.base_directory,
                    len(self.class_path_extension),
                    self.class_path_extension,
                    self.initial_class,
                    )                    

######################################################################
class content_type_descriptor(Descriptor):

    descriptor_tag = 0x72

    def bytes(self):
        return self.content_type

    def sample(self):
        self.content_type = "image/png"

###
# The following two should be somewhere else
######################################################################
#  INCOMPLETE
class additional_ginga_j_info(DVBobject):
    def __init__(self, transmission_format, document_resolution, organization_id, application_id, carousel_id):
        self.transmission_format = transmission_format
        self.document_resolution = document_resolution
        self.organization_id     = organization_id
        self.application_id      = application_id
        self.carousel_id         = carousel_id

    def bytes(self):
        fmt = "!BLHLB"
        return pack(
            fmt,
            (self.transmission_format << 6) | 0x20 | (self.document_resolution << 1),
            self.organization_id,
            self.application_id,
            self.carousel_id,
            0x1F
        )

######################################################################
class ait_identifier_info(DVBobject):
    def __init__(self, application_type, ait_version):
        self.application_type = application_type
        self.ait_version      = ait_version

    def bytes(self):
        return pack ("!HB",
            self.application_type,
            0xE0 | self.ait_version
        )

