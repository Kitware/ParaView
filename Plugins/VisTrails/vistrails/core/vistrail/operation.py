
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

from db.domain import DBAdd, DBChange, DBDelete
from db.domain import DBAnnotation, DBAbstractionRef, DBConnection, DBGroup, \
    DBLocation, DBModule, DBFunction, DBPluginData, DBParameter, DBPort, \
    DBPortSpec, DBTag

from core.vistrail.annotation import Annotation
from core.vistrail.abstraction_module import AbstractionModule
from core.vistrail.connection import Connection
from core.vistrail.group import Group
from core.vistrail.location import Location
from core.vistrail.module import Module
from core.vistrail.module_function import ModuleFunction
from core.vistrail.module_param import ModuleParam
from core.vistrail.plugin_data import PluginData
from core.vistrail.port import Port
from core.vistrail.port_spec import PortSpec
from core.vistrail.tag import Tag

def convert_data(_data):
    map = {
        DBAnnotation.vtType: Annotation,
        DBAbstractionRef.vtType: AbstractionModule,
        DBConnection.vtType: Connection,
        DBLocation.vtType: Location,
        DBModule.vtType: Module,
        DBFunction.vtType: ModuleFunction,
        DBGroup.vtType: Group,
        DBParameter.vtType: ModuleParam,
        DBPluginData.vtType: PluginData,
        DBPort.vtType: Port,
        DBPortSpec.vtType: PortSpec,
        DBTag.vtType: Tag,
        }
    try:
        map[_data.vtType].convert(_data)
    except KeyError:
        raise Exception('cannot convert data of type %s' % _data.vtType)

class AddOp(DBAdd):

    ##########################################################################
    # Constructors and copy

    def __init__(self, *args, **kwargs):
        DBAdd.__init__(self, *args, **kwargs)
    
    def __copy__(self):
        return AddOp.do_copy(self)

    def do_copy(self, new_ids=False, id_scope=None, id_remap=None):
        cp = DBAdd.do_copy(self, new_ids, id_scope, id_remap)
        cp.__class__ = AddOp
        return cp

    @staticmethod
    def convert(_add_op):
        if _add_op.__class__ == AddOp:
            return
        _add_op.__class__ = AddOp
        if _add_op.data is not None:
            convert_data(_add_op.data)
    ##########################################################################
    # Properties

    id = DBAdd.db_id
    what = DBAdd.db_what
    objectId = DBAdd.db_objectId
    parentObjId = DBAdd.db_parentObjId
    parentObjType = DBAdd.db_parentObjType
    data = DBAdd.db_data

    ##########################################################################
    # Operators
    
    def __str__(self):
        """__str__() -> str - Returns a string representation of an Annotation
        object. 

        """
        
        rep = ("<add id=%s what=%s objectId=%s parentObjId=%s" + \
               " parentObjType=%s>") % (str(self.id), str(self.what), str(self.objectId),
                                        str(self.parentObjId), str(self.parentObjType))
        rep += str(self.data) + "</add>"
        return rep

    # FIXME expand this
    def __eq__(self, other):
        """ __eq__(other: AddOp) -> boolean
        Returns True if self and other have the same attributes. Used by == 
        operator. 
        
        """
        if type(other) != type(self):
            return False
        return True

    def __ne__(self, other):
        return not self.__eq__(other)

class ChangeOp(DBChange):

    ##########################################################################
    # Constructors and copy

    def __init__(self, *args, **kwargs):
        DBChange.__init__(self, *args, **kwargs)

    def __copy__(self):
        return ChangeOp.do_copy(self)

    def do_copy(self, new_ids=False, id_scope=None, id_remap=None):
        cp = DBChange.do_copy(self, new_ids, id_scope, id_remap)
        cp.__class__ = ChangeOp
        return cp
    
    @staticmethod
    def convert(_change_op):
        if _change_op.__class__ == ChangeOp:
            return
        _change_op.__class__ = ChangeOp
        if _change_op.data is not None:
            convert_data(_change_op.data)

    ##########################################################################
    # Properties
    
    id = DBChange.db_id
    what = DBChange.db_what
    oldObjId = DBChange.db_oldObjId
    old_obj_id = DBChange.db_oldObjId
    newObjId = DBChange.db_newObjId
    new_obj_id = DBChange.db_newObjId
    parentObjId = DBChange.db_parentObjId
    parentObjType = DBChange.db_parentObjType
    data = DBChange.db_data

    # def _get_id(self):
    #     return self.db_id
    # def _set_id(self, id):
    #     self.db_id = id
    # id = property(_get_id, _set_id)

    # def _get_what(self):
    #     return self.db_what
    # def _set_what(self, what):
    #     self.db_what = what
    # what = property(_get_what, _set_what)

    # def _get_oldObjId(self):
    #     return self.db_oldObjId
    # def _set_oldObjId(self, oldObjId):
    #     self.db_oldObjId = oldObjId
    # oldObjId = property(_get_oldObjId, _set_oldObjId)
    # old_obj_id = property(_get_oldObjId, _set_oldObjId)

    # def _get_newObjId(self):
    #     return self.db_newObjId
    # def _set_newObjId(self, newObjId):
    #     self.db_newObjId = newObjId
    # newObjId = property(_get_newObjId, _set_newObjId)
    # new_obj_id = property(_get_newObjId, _set_newObjId)

    # def _get_parentObjId(self):
    #     return self.db_parentObjId
    # def _set_parentObjId(self, parentObjId):
    #     self.db_parentObjId = parentObjId
    # parentObjId = property(_get_parentObjId, _set_parentObjId)

    # def _get_parentObjType(self):
    #     return self.db_parentObjType
    # def _set_parentObjType(self, parentObjType):
    #     self.db_parentObjType = parentObjType
    # parentObjType = property(_get_parentObjType, _set_parentObjType)

    # def _get_data(self):
    #     return self.db_data
    # def _set_data(self, data):
    #     self.db_data = data
    # data = property(_get_data, _set_data)

    ##########################################################################
    # Operators
    
    def __str__(self):
        """__str__() -> str - Returns a string representation of an Annotation
        object. 

        """
        rep = "<change id=%s what=%s oldId=%s newId=%s parentObjId=%s" + \
            " parentObjType=%s>" + str(self.data) + "</change>"
        return rep % (str(self.id), str(self.what), str(self.oldObjId),
                      str(self.newObjId), str(self.parentObjId), 
                      str(self.parentObjType))

    # FIXME expand this
    def __eq__(self, other):
        """ __eq__(other: ChangeOp) -> boolean
        Returns True if self and other have the same attributes. Used by == 
        operator. 
        
        """
        if type(other) != type(self):
            return False
        return True

    def __ne__(self, other):
        return not self.__eq__(other)

class DeleteOp(DBDelete):

    ##########################################################################
    # Constructors and copy

    def __init__(self, *args, **kwargs):
        DBDelete.__init__(self, *args, **kwargs)
    
    def __copy__(self):
        return DeleteOp.do_copy(self)

    def do_copy(self, new_ids=False, id_scope=None, id_remap=None):
        cp = DBDelete.do_copy(self, new_ids, id_scope, id_remap)
        cp.__class__ = DeleteOp
        return cp

    @staticmethod
    def convert(_delete_op):
        _delete_op.__class__ = DeleteOp

    ##########################################################################
    # Properties

    id = DBDelete.db_id
    what = DBDelete.db_what
    objectId = DBDelete.db_objectId
    old_obj_id = DBDelete.db_objectId
    new_obj_id = DBDelete.db_objectId
    parentObjId = DBDelete.db_parentObjId
    parentObjType = DBDelete.db_parentObjType

    # def _get_id(self):
    #     return self.db_id
    # def _set_id(self, id):
    #     self.db_id = id
    # id = property(_get_id, _set_id)

    # def _get_what(self):
    #     return self.db_what
    # def _set_what(self, what):
    #     self.db_what = what
    # what = property(_get_what, _set_what)

    # def _get_objectId(self):
    #     return self.db_objectId
    # def _set_objectId(self, objectId):
    #     self.db_objectId = objectId
    # objectId = property(_get_objectId, _set_objectId)
    # old_obj_id = property(_get_objectId, _set_objectId)
    # new_obj_id = property(_get_objectId, _set_objectId)

    # def _get_parentObjId(self):
    #     return self.db_parentObjId
    # def _set_parentObjId(self, parentObjId):
    #     self.db_parentObjId = parentObjId
    # parentObjId = property(_get_parentObjId, _set_parentObjId)
    
    # def _get_parentObjType(self):
    #     return self.db_parentObjType
    # def _set_parentObjType(self, parentObjType):
    #     self.db_parentObjType = parentObjType
    # parentObjType = property(_get_parentObjType, _set_parentObjType)

    ##########################################################################
    # Operators
    
    def __str__(self):
        """__str__() -> str - Returns a string representation of an Annotation
        object. 

        """
        rep = "<delete id=%s what=%s objectId=%s parentObjId=%s" + \
            " parentObjType=%s/>"
        return rep % (str(self.id), str(self.what), str(self.objectId),
                      str(self.parentObjId), str(self.parentObjType))

    # FIXME expand this
    def __eq__(self, other):
        """ __eq__(other: DeleteOp) -> boolean
        Returns True if self and other have the same attributes. Used by == 
        operator. 
        
        """
        if type(other) != type(self):
            return False
        return True

    def __ne__(self, other):
        return not self.__eq__(other)

################################################################################
# Unit tests

import unittest
import copy
from db.domain import IdScope

class TestOperation(unittest.TestCase):
    
    def create_ops(self, id_scope=IdScope()):
        from core.vistrail.module import Module
        from core.vistrail.module_function import ModuleFunction
        from core.vistrail.module_param import ModuleParam
        from core.vistrail.annotation import Annotation
        
        if id_scope is None:
            id_scope = IdScope(remap={AddOp.vtType: 'operation',
                                      ChangeOp.vtType: 'operation',
                                      DeleteOp.vtType: 'operation'})

        m = Module(id=id_scope.getNewId(Module.vtType),
                   name='Float',
                   package='edu.utah.sci.vistrails.basic')
        add_op = AddOp(id=id_scope.getNewId(AddOp.vtType),
                       what=Module.vtType,
                       objectId=m.id,
                       data=m)
        function = ModuleFunction(id=id_scope.getNewId(ModuleFunction.vtType),
                                  name='value')
        change_op = ChangeOp(id=id_scope.getNewId(ChangeOp.vtType),
                             what=ModuleFunction.vtType,
                             oldObjId=2,
                             newObjId=function.real_id,
                             parentObjId=m.id,
                             parentObjType=Module.vtType,
                             data=function)
        param = ModuleParam(id=id_scope.getNewId(ModuleParam.vtType),
                            type='Float',
                            val='1.0')
        
        delete_op = DeleteOp(id=id_scope.getNewId(DeleteOp.vtType),
                             what=ModuleParam.vtType,
                             objectId=param.real_id,
                             parentObjId=function.real_id,
                             parentObjType=ModuleFunction.vtType)

        annotation = Annotation(id=id_scope.getNewId(Annotation.vtType),
                                key='foo',
                                value='bar')
        add_annotation = AddOp(id=id_scope.getNewId(AddOp.vtType),
                               what=Annotation.vtType,
                               objectId=m.id,
                               data=annotation)
        
        return [add_op, change_op, delete_op, add_annotation]

    def test_copy(self):       
        id_scope = IdScope(remap={AddOp.vtType: 'operation',
                                  ChangeOp.vtType: 'operation',
                                  DeleteOp.vtType: 'operation'})
        for op1 in self.create_ops(id_scope):
            op2 = copy.copy(op1)
            self.assertEquals(op1, op2)
            self.assertEquals(op1.id, op2.id)
            op3 = op1.do_copy(True, id_scope, {})
            self.assertEquals(op1, op3)
            self.assertNotEquals(op1.id, op3.id)
            if hasattr(op1, 'data'):
                self.assertNotEquals(op1.data.db_id, op3.data.db_id)

    def test_serialization(self):
        import core.db.io
        for op1 in self.create_ops():
            xml_str = core.db.io.serialize(op1)
            op2 = core.db.io.unserialize(xml_str, op1.__class__)
            self.assertEquals(op1, op2)
            self.assertEquals(op1.id, op2.id)
