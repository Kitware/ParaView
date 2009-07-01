
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
""" This module contains class definitions for:

    * ModuleFunction
"""

from db.domain import DBFunction
from core.utils import enum, VistrailsInternalError, all, eprint
from core.vistrail.port import PortEndPoint
from core.vistrail.module_param import ModuleParam
from itertools import izip
import copy
import __builtin__


################################################################################

PipelineElementType = enum('PipelineElementType',
                           ['Module', 'Connection', 'Function', 'Parameter'])

################################################################################

class ModuleFunction(DBFunction):
    __fields__ = ['name', 'returnType', 'params']
    """ Stores a function from a vistrail module """

    ##########################################################################
    # Constructors and copy
    
    def __init__(self, *args, **kwargs):
  DBFunction.__init__(self, *args, **kwargs)
        if self.name is None:
            self.name = ""
        if self.real_id is None:
            self.real_id = -1
        if self.pos is None:
            self.pos = -1
        self.returnType = "void"

    def __copy__(self):
        """ __copy__() -> ModuleFunction - Returns a clone of itself """
        return ModuleFunction.do_copy(self)

    def do_copy(self, new_ids=False, id_scope=None, id_remap=None):
        cp = DBFunction.do_copy(self, new_ids, id_scope, id_remap)
        cp.__class__ = ModuleFunction
        cp.returnType = self.returnType
        return cp

    @staticmethod
    def convert(_function):
        if _function.__class__ == ModuleFunction:
            return
  _function.__class__ = ModuleFunction
  for _parameter in _function.db_get_parameters():
      ModuleParam.convert(_parameter)
        _function.returnType = "void"

    ##########################################################################
    # Properties

    id = DBFunction.db_pos
    pos = DBFunction.db_pos
    real_id = DBFunction.db_id
    name = DBFunction.db_name
   

    def _get_params(self):
        self.db_parameters.sort(key=lambda x: x.db_pos)
        return self.db_parameters
    def _set_params(self, params):
        self.db_parameters = params
    # If you're mutating the params property, watch out for the sort
    # gotcha: every time you use the params on reading position,
    # they get resorted
    params = property(_get_params, _set_params)
    parameters = property(_get_params, _set_params)

    def addParameter(self, param):
        self.db_add_parameter(param)

    ##########################################################################

    def getNumParams(self):
        """ getNumParams() -> int Returns the number of params. """
        return len(self.params)
    
    def serialize(self, doc, element):
  """serialize(doc, element) -> None - Writes itself in XML """
  child = doc.createElement('function')
  child.setAttribute('name',self.name)
  child.setAttribute('returnType',self.type)
  for p in self.params:
    p.serialize(doc,child)
  element.appendChild(child)


    def getSignature(self):
        """ getSignature() -> str - Returns the function signature..

        This is a deprecated call! """
        result = self.returnType + "("
        for p in self.params:
            result = result + p.type + ", "
        if result.rfind(",") != -1:
            result = result[0:result.rfind(",")]
        else:
            result = result + " "
        result = result + ")"
        return result

    ##########################################################################
    # Debugging

    def show_comparison(self, other):
        if type(self) != type(other):
            print "type mismatch"
            return
        if self.name != other.name:
            print "name mismatch"
            return
        if self.returnType != other.returnType:
            print "return type mismatch"
            return
        if len(self.params) != len(other.params):
            print "params length mismatch"
            return
        for p,q in izip(self.params, other.params):
            if p != q:
                print "params mismatch"
                p.show_comparison(q)
                return
        print "no difference found"
        assert self == other
        return

    ##########################################################################
    # Operators
    
    def __str__(self):
        """ __str__() -> str - Returns a string representation of itself """
        return ("<function id='%s' pos='%s' name='%s' params=%s)@%X" %
                (self.real_id,
                 self.pos,
                 self.name,
                 [str(p) for p in self.params],
                 id(self)))

    def __eq__(self, other):
        """ __eq__(other: ModuleFunction) -> boolean
        Returns True if self and other have the same attributes. Used by == 
        operator. 
        
        """
        if type(self) != type(other):
            return False
        if self.name != other.name:
            return False
        if self.returnType != other.returnType:
            return False
        if len(self.params) != len(other.params):
            return False
        for p,q in zip(self.params, other.params):
            if p != q:
                return False
        return True
            
    def __ne__(self, other):
        """ __ne__(other: ModuleFunction) -> boolean
        Returns True if self and other don't have the same attributes. 
        Used by !=  operator. 
        
        """
        return not self.__eq__(other)

################################################################################
# Testing

import unittest
import copy
from core.vistrail.module_param import ModuleParam
from db.domain import IdScope

#TODO add more meaningful tests

class TestModuleFunction(unittest.TestCase):

    def create_function(self, id_scope=IdScope()):
        param = ModuleParam(id=id_scope.getNewId(ModuleParam.vtType),
                            pos=2,
                            type='Int',
                            val='1')
        function = ModuleFunction(id=id_scope.getNewId(ModuleFunction.vtType),
                                  pos=0,
                                  name='value',
                                  parameters=[param])
        return function

    def test_copy(self):        
        id_scope = IdScope()
        f1 = self.create_function(id_scope)
        f2 = copy.copy(f1)
        self.assertEquals(f1, f2)
        self.assertEquals(f1.id, f2.id)
        f3 = f1.do_copy(True, id_scope, {})
        self.assertEquals(f1, f3)
        self.assertNotEquals(f1.real_id, f3.real_id)

    def test_serialization(self):
        import core.db.io
        f1 = self.create_function()
        xml_str = core.db.io.serialize(f1)
        f2 = core.db.io.unserialize(xml_str, ModuleFunction)
        self.assertEquals(f1, f2)
        self.assertEquals(f1.real_id, f2.real_id)
                            
    def testComparisonOperators(self):
        f = ModuleFunction()
        f.name = "value"
        param = ModuleParam()
        param.strValue = "1.2"
        param.type = "Float"
        param.alias = ""
        f.addParameter(param)
        g = ModuleFunction()
        g.name = "value"
        param = ModuleParam()
        param.strValue = "1.2"
        param.type = "Float"
        param.alias = ""
        g.addParameter(param)
        assert f == g
        param = ModuleParam()
        param.strValue = "1.2"
        param.type = "Float"
        param.alias = ""
        g.addParameter(param)
        assert f != g

    def test_str(self):
        f = ModuleFunction(name='value',
                           parameters=[ModuleParam(type='Float',
                                                   val='1.2')],
                           )
        str(f)

if __name__ == '__main__':
    unittest.main()
    
