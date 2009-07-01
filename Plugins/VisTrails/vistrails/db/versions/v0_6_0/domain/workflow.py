
############################################################################
##
## This file is part of the Vistrails ParaView Plugin.
##
## This file may be used under the terms of the GNU General Public
## License version 2.0 as published by the Free Software Foundation
## and appearing in the file LICENSE.GPL included in the packaging of
## this file.  Please review the following to ensure GNU General Public
## Licensing requirements will be met:
## http://www.opensource.org/licenses/gpl-2.0.php
##
## This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
## WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
##
############################################################################

############################################################################
##
## Copyright (C) 2006, 2007, 2008 University of Utah. All rights reserved.
##
############################################################################

from auto_gen import DBWorkflow as _DBWorkflow
from id_scope import IdScope

import copy

class DBWorkflow(_DBWorkflow):

    def __init__(self, *args, **kwargs):
  _DBWorkflow.__init__(self, *args, **kwargs)
  self.objects = {}
        self.tmp_id = IdScope(1)

    def __copy__(self):
        cp = _DBWorkflow.__copy__(self)
        cp.__class__ = DBWorkflow
        # need to go through and reset the index to the copied objects
        cp.objects = {}
        for (child, _, _) in cp.db_children():
            cp.addToIndex(child)
        cp.tmp_id = copy.copy(self.tmp_id)
        return cp

    def addToIndex(self, object):
        self.objects[(object.vtType, object.getPrimaryKey())] = object

    def deleteFromIndex(self, object):
  del self.objects[(object.vtType, object.getPrimaryKey())]

    def capitalizeOne(self, str):
  return str[0].upper() + str[1:]

    def db_add_object(self, object, parentObjType=None, parentObjId=None):
  if parentObjType is None or parentObjId is None:
      parentObj = self
  else:
      try:
    parentObj = self.objects[(parentObjType, parentObjId)]
      except KeyError:
    msg = "Cannot find object of type '%s' with id '%s'" % \
        (parentObjType, parentObjId)
    raise Exception(msg)
  funname = 'db_add_%s' % object.vtType
  objCopy = copy.copy(object)
  getattr(parentObj, funname)(objCopy)
  self.addToIndex(objCopy)

    def db_change_object(self, object, parentObjType=None, parentObjId=None):
  if parentObjType is None or parentObjId is None:
      parentObj = self
  else:
      try:
    parentObj = self.objects[(parentObjType, parentObjId)]
      except KeyError:
    msg = "Cannot find object of type '%s' with id '%s'" % \
        (parentObjType, parentObjId)
    raise Exception(msg)
  funname = 'db_change_%s' % object.vtType
  objCopy = copy.copy(object)
  getattr(parentObj, funname)(objCopy)
  self.addToIndex(objCopy)

    def db_delete_object(self, objId, objType, 
                         parentObjType=None, parentObjId=None):
  if parentObjType is None or parentObjId is None:
      parentObj = self
  else:
      try:
    parentObj = self.objects[(parentObjType, parentObjId)]
      except KeyError:
    msg = "Cannot find object of type '%s' with id '%s'" % \
        (parentObjType, parentObjId)
    raise Exception(msg)
  funname = 'db_get_%s' % objType
        try:
            object = getattr(parentObj, funname)(objId)
        except AttributeError:
            attr_name = 'db_%s' % objType
            object = getattr(parentObj, attr_name)
  funname = 'db_delete_%s' % objType
  getattr(parentObj, funname)(object)
  self.deleteFromIndex(object)
