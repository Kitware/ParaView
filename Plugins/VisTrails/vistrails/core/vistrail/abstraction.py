
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

from core.vistrail.action import Action
from core.vistrail.tag import Tag
from db.domain import DBAbstraction

class Abstraction(DBAbstraction):

    ##########################################################################
    # Constructors and copy

    def __init__(self, *args, **kwargs):
        DBAbstraction.__init__(self, *args, **kwargs)
        if self.id is None:
            self.id = -1
        
    def __copy__(self):
        return Abstraction.do_copy(self)

    def do_copy(self, new_ids=False, id_scope=None, id_remap=None):
        cp = DBAbstraction.do_copy(self, new_ids, id_scope, id_remap)
        cp.__class__ = Abstraction
        return cp

    @staticmethod
    def convert(_abstraction):
        if _abstraction.__class__ == Abstraction:
            return
        _abstraction.__class__ = Abstraction
        for _action in _abstraction.action_list:
            Action.convert(_action)
        for _tag in _abstraction.tag_list:
            Tag.convert(_tag)

    ##########################################################################
    # Properties
    
    id = DBAbstraction.db_id
    name = DBAbstraction.db_name

    def _get_actions(self):
        return self.db_actions_id_index
    actions = property(_get_actions)
    def _get_action_list(self):
        return self.db_actions
    action_list = property(_get_action_list)
    
    def _get_tags(self):
        return self.db_tags_id_index
    tags = property(_get_tags)
    def _get_tag_list(self):
        return self.db_tags
    tag_list = property(_get_tag_list)

    ##########################################################################

    def add_action(self, action, parent):
        Action.convert(action)
        if action.id < 0:
            action.id = self.idScope.getNewId(action.vtType)
        action.prevId = parent
        for op in action.operations:
            if op.id < 0:
                op.id = self.idScope.getNewId('operation')
        self.add_version(action)                

    def add_version(self, action):
        self.db_add_action(action)

    ##########################################################################
    # Operators
    
    def __str__(self):
        """__str__() -> str - Returns a string representation of an Abstraction
        object. 

        """
        rep = '<abstraction id="%s" name="%s">' + \
            '<actions>%s</actions>' + \
            '<tags>%s</tags>' + \
            '</abstraction>'
        return  rep % (str(self.id), str(self.name), 
                       [str(a) for a in self.action_list],
                       [str(t) for t in self.tag_list])

    def __eq__(self, other):
        """ __eq__(other: Abstraction) -> boolean
        Returns True if self and other have the same attributes. Used by == 
        operator. 
        
        """
        if type(self) != type(other):
            return False
        if len(self.tag_list) != len(other.tag_list):
            return False
        if len(self.action_list) != len(other.action_list):
            return False
        for f,g in zip(self.tag_list, other.tag_list):
            if f != g:
                return False
        for f,g in zip(self.action_list, other.action_list):
            if f != g:
                return False
        return True

    def __ne__(self, other):
        return not self.__eq__(other)

################################################################################
# Testing

import unittest

class TestAbstraction(unittest.TestCase):
    
    def create_abstraction(self, id_scope=None):
        from core.vistrail.action import Action
        from core.vistrail.module import Module
        from core.vistrail.module_function import ModuleFunction
        from core.vistrail.module_param import ModuleParam
        from core.vistrail.operation import AddOp
        from core.vistrail.tag import Tag
        from db.domain import IdScope
        from datetime import datetime
        
        if id_scope is None:
            id_scope = IdScope()
        function = ModuleFunction(id=id_scope.getNewId(ModuleFunction.vtType),
                                  name='value')
        m = Module(id=id_scope.getNewId(Module.vtType),
                   name='Float',
                   package='edu.utah.sci.vistrails.basic',
                   functions=[function])

        add_op = AddOp(id=id_scope.getNewId('operation'),
                       what='module',
                       objectId=m.id,
                       data=m)
        add_op2 = AddOp(id=id_scope.getNewId('operation'),
                       what='function',
                       objectId=function.id,
                       data=function)
        action = Action(id=id_scope.getNewId(Action.vtType),
                        prevId=0,
                        date=datetime(2007,11, 18),
                        operations=[add_op, add_op2])
        tag = Tag(id=id_scope.getNewId(Tag.vtType),
                  name='a tag')
        abstraction = Abstraction(id=id_scope.getNewId(Abstraction.vtType),
                                  name='blah',
                                  actions=[action],
                                  tags=[tag])
        return abstraction

    def test_copy(self):
        import copy
        from db.domain import IdScope

        id_scope = IdScope()

        a1 = self.create_abstraction(id_scope)
        a2 = copy.copy(a1)
        self.assertEquals(a1, a2)
        self.assertEquals(a1.id, a2.id)
        a3 = a1.do_copy(True, id_scope, {})
        self.assertEquals(a1, a3)
        self.assertNotEquals(a1.id, a3.id)

    def test_serialization(self):
        import core.db.io 
        a1 = self.create_abstraction()
        xml_str = core.db.io.serialize(a1)
        a2 = core.db.io.unserialize(xml_str, Abstraction)
        self.assertEquals(a1, a2)
        self.assertEquals(a1.id, a2.id)
