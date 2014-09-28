#! /usr/bin/env python

# This file is part of the dvbobjects library and contains the main HbbTV descriptors (see the list below)
# 
# Copyright  2000-2001, GMD, Sankt Augustin
# -- German National Research Center for Information Technology 
#
# Copyright  2010, mediatvcom, Claude Vanderm
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

#Below are listed the HbbTV available, and missing descriptor of this release.
#Based on HbbTV specification (ETSI-TS 102 796) version 1.1.1
#
#available hbbtv descriptors:                     | optional missing descriptors:
#application_descriptor()                         | application_icons_descriptor()
#application_name_descriptor()                              
#transport_protocol_descriptor()          
#simple_application_location_descriptor()                                       
#application_usage_descriptor()
#simple_application_boundary_descriptor()
#external_application_authorisation_descriptor()

import string
from dvbobjects.utils import *
from dvbobjects.MPEG.Descriptor import Descriptor

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
        if self.protocol_id == 0x0001:
            fmt = "!HBBB"
            return pack(fmt,
                        self.protocol_id,
                        self.transport_protocol_label,
                        0x7F | (self.remote_connection << 7),
                        self.component_tag,
                        )
                        
        if self.protocol_id == 0x0003:
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
class simple_application_location_descriptor(Descriptor):

    descriptor_tag = 0x15

    def bytes(self):
        fmt = "!%ds" % len(self.initial_path_bytes)
        return pack(fmt,
                    self.initial_path_bytes
                    )
######################################################################
class application_usage_descriptor(Descriptor):

    descriptor_tag = 0x16
    
    def bytes(self):
        fmt = "!B" 
        return pack(fmt,
                    self.usage_type
                    )
                    
######################################################################
class simple_application_boundary_descriptor(Descriptor):
    
    descriptor_tag = 0x17
    
    def bytes(self):
        boundary_extension_count = 0
        for boundary_extension in self.boundary_extensions:
            boundary_extension_count = boundary_extension_count + 1
        fmt = "!B"
        result = pack(fmt,
                      boundary_extension_count
                      )
        for boundary_extension in self.boundary_extensions:
            result = result + pack("!B%ds" % len(boundary_extension),
                                    len(boundary_extension),
                                    boundary_extension,
                                    )
        return result
######################################################################
class external_application_authorisation_descriptor(Descriptor):
    
    descriptor_tag = 0x05

    def bytes(self):
        result = ""
        for y in range(len(self.application_identifiers)):
            i=0
            fmt = "!LHB"
            result = result + pack(fmt,
                                    self.application_identifiers[y][i], #organisation_id,
                                    self.application_identifiers[y][i+1], # application_id,
                                    self.application_priority[y]
                                    )
        return result
