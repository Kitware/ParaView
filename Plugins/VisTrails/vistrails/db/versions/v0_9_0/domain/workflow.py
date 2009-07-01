
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
from auto_gen import DBAbstractionRef, DBModule
from id_scope import IdScope

import copy

class DBWorkflow(_DBWorkflow):

    def __init__(self, *args, **kwargs):
  _DBWorkflow.__init__(self, *args, **kwargs)
  self.objects = {}
        self.tmp_id = IdScope(1,
                              {DBAbstractionRef.vtType: DBModule.vtType})

    def __copy__(self):
        return DBWorkflow.do_copy(self)

    def do_copy(self, new_ids=False, id_scope=None, id_remap=None):
        cp = _DBWorkflow.do_copy(self, new_ids, id_scope, id_remap)
        cp.__class__ = DBWorkflow
        # need to go through and reset the index to the copied objects
        cp.objects = {}
        for (child, _, _) in cp.db_children():
            cp.add_to_index(child)
        cp.tmp_id = copy.copy(self.tmp_id)
        return cp        

    def add_to_index(self, object):
        if object.vtType == 'abstractionRef':
            obj_type = 'module'
        else:
            obj_type = object.vtType
        self.objects[(obj_type, object.getPrimaryKey())] = object

    def delete_from_index(self, object):
        if object.vtType == 'abstractionRef':
            obj_type = 'module'
        else:
            obj_type = object.vtType
  del self.objects[(obj_type, object.getPrimaryKey())]

    def capitalizeOne(self, str):
  return str[0].upper() + str[1:]

    def db_print_objects(self):
        for k,v in self.objects.iteritems():
            print '%s: %s' % (k, v)

    def db_has_object(self, type, id):
        return self.objects.has_key((type, id))

    def db_get_object(self, type, id):
        return self.objects[(type, id)]

    def db_add_object(self, object, parent_obj_type=None,
                      parent_obj_id=None, parent_obj=None):
        if parent_obj is None:
            if parent_obj_type is None or parent_obj_id is None:
                parent_obj = self
            else:
                if parent_obj_type == 'abstractionRef':
                    parent_obj_type = 'module'
                if self.objects.has_key((parent_obj_type, parent_obj_id)):
                    parent_obj = self.objects[(parent_obj_type, parent_obj_id)]
                else:
                    msg = "Cannot find object of type '%s' with id '%s'" % \
                        (parent_obj_type, parent_obj_id)
                    raise Exception(msg)
        if object.vtType == 'abstractionRef':
            obj_type = 'module'
        else:
            obj_type = object.vtType
  funname = 'db_add_%s' % obj_type
  obj_copy = copy.copy(object)
  getattr(parent_obj, funname)(obj_copy)
  self.add_to_index(obj_copy)

    def db_change_object(self, old_id, object, parent_obj_type=None, 
                         parent_obj_id=None, parent_obj=None):
  if parent_obj is None:
            if parent_obj_type is None or parent_obj_id is None:
                parent_obj = self
            else:
                if parent_obj_type == 'abstractionRef':
                    parent_obj_type = 'module'
                if self.objects.has_key((parent_obj_type, parent_obj_id)):
                    parent_obj = self.objects[(parent_obj_type, parent_obj_id)]
                else:
                    msg = "Cannot find object of type '%s' with id '%s'" % \
                        (parent_obj_type, parent_obj_id)
                    raise Exception(msg)

        self.db_delete_object(old_id, object.vtType, None, None, parent_obj)
        self.db_add_object(object, None, None, parent_obj)

    def db_delete_object(self, obj_id, obj_type, parent_obj_type=None, 
                         parent_obj_id=None, parent_obj=None):
        if parent_obj is None:
            if parent_obj_type is None or parent_obj_id is None:
                parent_obj = self
            else:
                if parent_obj_type == 'abstractionRef':
                    parent_obj_type = 'module'
                if self.objects.has_key((parent_obj_type, parent_obj_id)):
                    parent_obj = self.objects[(parent_obj_type, parent_obj_id)]
                else:
                    msg = "Cannot find object of type '%s' with id '%s'" % \
                        (parent_obj_type, parent_obj_id)
                    raise Exception(msg)
        if obj_type == 'abstractionRef':
            obj_type = 'module'
  funname = 'db_get_%s' % obj_type
        if hasattr(parent_obj, funname):
            object = getattr(parent_obj, funname)(obj_id)
        else:
            attr_name = 'db_%s' % obj_type
            object = getattr(parent_obj, attr_name)
  funname = 'db_delete_%s' % obj_type
  getattr(parent_obj, funname)(object)
  self.delete_from_index(object)
