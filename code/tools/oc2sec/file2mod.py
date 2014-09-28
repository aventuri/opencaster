#! /usr/bin/env python

# 
# Copyright  2000-2001, GMD, Sankt Augustin
# -- German National Research Center for Information Technology 
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
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

import sys
import os
import re
import getopt
import pprint
import string

from stat import ST_SIZE
from dvbobjects.utils import *
from dvbobjects.utils.SpecFile import *
from dvbobjects.DSMCC import BIOP
from dvbobjects.DVB.DataCarousel import TransactionId

######################################################################

TIMEOUT			= 0xFFFFFFFF
TABLE_ID		= 0x3C
MAX_MODULE_SIZE		= 1024 * 64

DEBUG 			= 0
OPTIONS = "h"
LONG_OPTIONS = [ "help" ]

######################################################################
class ModuleBuilder:

	def __init__(self, OUTPUT_DIR, MODULE_ID):
		self.module_id = MODULE_ID
		filename = "%s/%04d.mod" % (OUTPUT_DIR, self.module_id)
		self.name = filename # needed for debug
		if DEBUG:
			print "NEW MODULE: %s" % filename
		self.__file = open(filename, "wb")
		self.size = 0

	def hasRoom(self, requestedSize):
		if self.size == 0:
			# Anything goes...
			return 1
		elif self.size + requestedSize <= MAX_MODULE_SIZE:
			# Module isn't empty, but still have space
			return 1
		else:
			# Start new module
			return 0

	def write(self, bytes):
		msgSize = len(bytes)
		if DEBUG:
			print "ADD (mod %d, size %d+%d=%d)" % ( self.module_id, self.size, msgSize, self.size+msgSize)
		self.__file.write(bytes)
		self.size = self.size + msgSize

######################################################################
class ObjectCarouselBuilder: # Build an object carousel with 1 DII, this limits the number of modules to something a bit more than 100

	def __init__(self, OUTPUT_DIR, CAROUSEL_ID, DOWNLOAD_ID, ASSOC_TAG, MODULE_VERSION, BLOCK_SIZE, UPDATE_FLAG, COMPRESS_MODE):
		
		self.MODULE_ID = 2
		
		# The builder generate modules files and specification files
		# specification files are for informations not included in the modules files themself
		
		self.DSI_TransactionId = TransactionId(
			version = MODULE_VERSION,
			identification = 0,
			updateFlag     = UPDATE_FLAG,
			)
		
		self.DII_TransactionId = TransactionId(
			version = MODULE_VERSION,
			identification = 1,
			updateFlag     = UPDATE_FLAG,
			)
		
		self.__SPECdii = open("%s/DII.spec" % OUTPUT_DIR, "w")
		
		self.__spec = SuperGroupSpec(
			transactionId = self.DSI_TransactionId,
			version       = MODULE_VERSION,
			srg_ior       = "%s/SRG_IOR" % OUTPUT_DIR,
		)

		self.__spec.addGroup(
			transactionId = self.DII_TransactionId,
			version       = MODULE_VERSION,
			downloadId    = DOWNLOAD_ID,
			assocTag      = ASSOC_TAG,
			blockSize     = BLOCK_SIZE,
			)
	
		# Output variables
		self.OUTPUT_DIR = OUTPUT_DIR
		self.DOWNLOAD_ID = DOWNLOAD_ID
		self.CAROUSEL_ID = CAROUSEL_ID
		self.ASSOC_TAG = ASSOC_TAG
		self.MODULE_VERSION = MODULE_VERSION
		self.COMPRESS_MODE = COMPRESS_MODE
		self.UPDATE_FLAG = UPDATE_FLAG
		self.BLOCK_SIZE = BLOCK_SIZE
		
		# currently open modules indexed by type
		self.__ModByType = {
			'sgw': 0,
			'dir': 0,
			'fil': 0,
			'ste': 0,
			'solo': 0,
		}
		self.__TypeInUse = {
			'sgw': 0,
			'dir': 0,
			'fil': 0,
			'ste': 0,
			'solo': 0,
		}
		
		self.__ModById = {}

		# Table of Contents, for debugging
		self.TOC = []

	def genSpec(self):
		self.__spec.write(self.OUTPUT_DIR)
	
	def addDirectory(self, node):
		self.__addNode(node, "dir")
	
	def addSGW(self, node):
		self.__addNode(node, "sgw")
		
	def addFile(self, node):
		self.__addNode(node, "fil")
		
	def addSoloFile(self, node):
		self.__addNode(node, "solo")
		
	def addStreamEvent(self, node):
		self.__addNode(node, "ste")
	
	def __addNode(self, node, type):
		msg = node.message()
		msgBytes = msg.pack()
		msgSize = len(msgBytes)
		
		if (type == 'solo'):
			self.__ModByType[type] = self.__nextModule(self.MODULE_ID)
			self.__ModByType[type].write(msgBytes)
			filename = "%s/%04d.mod.solo" % (OUTPUT_DIR, self.__ModByType[type].module_id)
			out = open(filename, "wb")
			out.close()
			modid = str(self.__ModByType[type].module_id)
		elif (type == 'sgw'):
			self.__ModByType[type] = self.__nextModule(1)
			self.__ModByType[type].write(msgBytes)
			modid = str(self.__ModByType[type].module_id)
		else:
			if not self.__TypeInUse[type]:
				self.__ModByType[type] = self.__nextModule(self.MODULE_ID)
				self.__TypeInUse[type] = 1
		
			if self.__ModByType[type].hasRoom(msgSize):
				self.__ModByType[type].write(msgBytes)
			else:
				self.__ModByType[type] = self.__nextModule(self.MODULE_ID)
				self.__ModByType[type].write(msgBytes)
			modid = str(self.__ModByType[type].module_id)
			
		node.bind(
			carouselId = self.CAROUSEL_ID,
			moduleId   = int(modid),
			assoc_tag = self.ASSOC_TAG,
			DII_TransactionId = self.DII_TransactionId,
		)
	
		self.TOC.append((
			int(modid),
			msg.objectKey,
			os.path.basename(msg.PATH),
			msg.objectKind,
			msgSize,
		))

	def __nextModule(self, modid):
		mod = ModuleBuilder(self.OUTPUT_DIR, int(modid))
		if (int(modid) > 1): # id 1 is reserved for sgw
			self.MODULE_ID = mod.module_id + 1
		
		self.__SPECdii.write("%s 0x%02X 0x%04X 0x%02X\n" % (
			mod.name,
			TABLE_ID,
			mod.module_id, 
			self.MODULE_VERSION))
		
		self.__spec.addModule(
			tableId       = TABLE_ID,
			moduleId      = mod.module_id,
			moduleVersion = self.MODULE_VERSION,
		)
		
		return mod

######################################################################
class FSNode(DVBobject): # superclass for FSDirectory, FSSteam and FSFile.

	def __init__(self, KEY_SERIAL_NUMBER):
		self.KEY = KEY_SERIAL_NUMBER
	def IOR(self, carouselId, moduleId, key, assoc_tag, DII_TransactionId):
		iop = BIOP.IOP.IOR(
			PATH = self.PATH, # for debugging
			type_id = self.MessageClass.objectKind,
			carouselId = carouselId,
			moduleId   = moduleId,
			objectKey  = key,
			assocTag      = assoc_tag,
			transactionId = DII_TransactionId,
			timeout       = TIMEOUT,
		)
		return iop

	def _checkBinding(self):
		try:
			raise "Already Bound", self._binding
		except AttributeError:
			pass
			

######################################################################
class FSFile(FSNode): # A File in a File System destined for an Object Carousel.

	MessageClass = BIOP.FileMessage
	BindingClass = BIOP.ObjectFileBinding
	
	def __init__(self, path, KEY_SERIAL_NUMBER):
		FSNode.__init__(self, KEY_SERIAL_NUMBER)
		assert(len(path) > 0)
		self.PATH = path
		self.contentSize = os.stat(path)[ST_SIZE]
	
	def bind(self, carouselId, moduleId, assoc_tag, DII_TransactionId):
		self._checkBinding()
		filename = os.path.basename(self.PATH)
		self._binding = self.BindingClass(
			nameId = filename + "\x00",
			IOR = self.IOR(carouselId, moduleId, self.KEY, assoc_tag, DII_TransactionId),
			contentSize = self.contentSize,
		)
	
	def binding(self):
		return self._binding
	
	def message(self):
		msg = self.MessageClass(
			PATH = self.PATH,
			objectKey   = self.KEY,
			contentSize = self.contentSize,
		)
		return msg
	
	def shipMessage(self, theObjectCarouselBuilder, isSolo):
		if (isSolo == 1):
			theObjectCarouselBuilder.addSoloFile(self)
		else:
			theObjectCarouselBuilder.addFile(self)
		
######################################################################
class FSStreamEvent(FSNode): # A Directory in a File System destined to genereate a StreamEvent Object for Object Carousel.

	MessageClass = BIOP.StreamEventMessage
	BindingClass = BIOP.ObjectStreamEventBinding
	
	def __init__(self, path, KEY_SERIAL_NUMBER):
		FSNode.__init__(self, KEY_SERIAL_NUMBER)
		assert(len(path) > 0)
		self.PATH = path
		
	def bind(self, carouselId, moduleId, assoc_tag, DII_TransactionId):
		self._checkBinding()
		filename = os.path.basename(self.PATH)
		self._binding = self.BindingClass(
			nameId = filename + "\x00",
			IOR = self.IOR(carouselId, moduleId, self.KEY, assoc_tag, DII_TransactionId),
		)
	
	def binding(self):
		return self._binding
	
	def message(self):
		msg = self.MessageClass(
			PATH = self.PATH,
			objectKey   = self.KEY,
		)
		return msg
	
	def shipMessage(self, theObjectCarouselBuilder):
		theObjectCarouselBuilder.addStreamEvent(self)

######################################################################
class FSDir(FSNode, ObjectCarouselBuilder):# A Directory in a File System destined for an Object Carousel.

	MessageClass = BIOP.DirectoryMessage
	BindingClass = BIOP.ContextBinding
	
	def __init__(self, path, KEY_SERIAL_NUMBER):
		FSNode.__init__(self, KEY_SERIAL_NUMBER)
		assert(len(path) > 0)
		self.PATH  = path
		self.bindings = []
		self.visitKEY = KEY_SERIAL_NUMBER
	
	def bind(self, carouselId, moduleId, assoc_tag, DII_TransactionId):
		self._checkBinding()
		filename = os.path.basename(self.PATH)
		self._binding = self.BindingClass(
			nameId = filename + "\x00",
			IOR = self.IOR(carouselId, moduleId, self.KEY, assoc_tag, DII_TransactionId),
		)
	
	def binding(self):
		return self._binding
		
	
	def message(self):
		msg = self.MessageClass(
			PATH = self.PATH,
			objectKey = self.KEY,
			bindings = self.bindings,
		)
		return msg 
	
	
	def visit(self, theObjectCarouselBuilder): #Depth first visit

		#REJECT_EXT = ['.pyc', '.o', '.so']
		#SINGLE_MODULE_EXT = ['.jpg', '.png', '.gif', '.solo']
		SINGLE_MODULE_EXT = ['.solo']
		#REJECT_FN = ['x', 'tmp']
		#REJECT_FN = []
		EVENT_EXT = ['.event']
	
		assert os.path.isdir(self.PATH), self.PATH
		try:
			ls = os.listdir(self.PATH)
		except:
			print self.PATH
			raise
	
		ls.sort()
	
		for filename in ls:
			path = os.path.join(self.PATH, filename)
#			if os.path.splitext(filename)[1] in REJECT_EXT:
#				continue
			if os.path.isfile(path):
				self.visitKEY = self.visitKEY + 1
				obj = FSFile(path, self.visitKEY)
				if (os.path.splitext(filename)[1] in SINGLE_MODULE_EXT) and (theObjectCarouselBuilder.COMPRESS_MODE == 2):
					obj.shipMessage(theObjectCarouselBuilder, 1)
				else:
					obj.shipMessage(theObjectCarouselBuilder, 0)
				if DEBUG:
					print obj.message()
					print
			elif os.path.isdir(path):
				if os.path.splitext(filename)[1] in EVENT_EXT:
					self.visitKEY = self.visitKEY + 1
					obj = FSStreamEvent(path, self.visitKEY)
					obj.shipMessage(theObjectCarouselBuilder)
					if DEBUG:
						print obj.message()
						print
				else:
					self.visitKEY = self.visitKEY + 1
					obj = FSDir(path, self.visitKEY)
					obj.visit(theObjectCarouselBuilder)
					self.visitKEY = obj.visitKEY
					if DEBUG:
						print obj.message()
						print
			else:
				continue
		
			self.bindings.append(obj.binding())
	
		# THIS directory (i.e. self) is complete, so...
		self.shipMessage(theObjectCarouselBuilder)
		if DEBUG:
			print self.message()
			print
		
	
	def shipMessage(self, theObjectCarouselBuilder):
		theObjectCarouselBuilder.addDirectory(self)

######################################################################
class FSRoot(FSDir): #A Directory in a File System destined as Service Gateway for an Object Carousel
	MessageClass = BIOP.ServiceGatewayMessage
	
	def shipMessage(self, theObjectCarouselBuilder):
		theObjectCarouselBuilder.addSGW(self)

######################################################################
if __name__ == '__main__':
	try:
		opts, args = getopt.getopt(sys.argv[1:], OPTIONS, LONG_OPTIONS)
	except getopt.error:
		print ("Usage: %s"
			" <InputDirectory>"
			" <OutputModulesDirectory>"
			" download_id"
			" carousel_id"
			" association_tag"
			" version" 
			" [block_size update_flag compress_mode]"
			) % (
		sys.argv[0])
		sys.exit(1)
	
	INPUT_DIR, OUTPUT_DIR, CAROUSEL_ID, DOWNLOAD_ID, ASSOC_TAG, MODULE_VERSION, BLOCK_SIZE, UPDATE_FLAG, COMPRESS_MODE = args
	
	root = FSRoot(INPUT_DIR, 0)
	theObjectCarouselBuilder = ObjectCarouselBuilder(OUTPUT_DIR, int(CAROUSEL_ID), int(DOWNLOAD_ID), int(ASSOC_TAG, 16), int(MODULE_VERSION, 16), int(BLOCK_SIZE), int(UPDATE_FLAG), int(COMPRESS_MODE))
	root.visit(theObjectCarouselBuilder)
	
	out = open("%s/SRG_IOR" % OUTPUT_DIR, "wb")
	out.write(root.binding().IOR.pack())
	out.close()
	
	theObjectCarouselBuilder.genSpec()
	
	if DEBUG:
		print root.binding().IOR
		pprint.pprint(theObjectCarouselBuilder.TOC)
