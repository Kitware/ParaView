
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

import copy
from itertools import izip
from sets import Set

import core.modules.module_registry
import core.modules.vistrails_module
from core.modules.basic_modules import Variant
from core.modules.module_registry import registry, ModuleRegistry
from core.vistrail.annotation import Annotation
from core.vistrail.location import Location
from core.vistrail.module_function import ModuleFunction
from core.vistrail.port import Port, PortEndPoint
from db.domain import DBAbstractionRef


class AbstractionModule(DBAbstractionRef):

    ##########################################################################
    # Constructors and copy

    def __init__(self, *args, **kwargs):
        DBAbstractionRef.__init__(self, *args, **kwargs)
        if self.id is None:
            self.id = -1
        self.portVisible = Set()
        self._registry = None
        self.abstraction = None
        # FIXME should we have a registry for an abstraction module?

    def __copy__(self):
        return AbstractionModule.do_copy(self)

    def do_copy(self, new_ids=False, id_scope=None, id_remap=None):
        cp = DBAbstractionRef.do_copy(self, new_ids, id_scope, id_remap)
        cp.__class__ = AbstractionModule
        cp.portVisible = copy.copy(self.portVisible)
        cp._registry = self._registry
        cp.abstraction = self.abstraction
        return cp

    @staticmethod
    def convert(_abstraction_module):
        if _abstraction_module.__class__ == AbstractionModule:
            return
        _abstraction_module.__class__ = AbstractionModule
        if _abstraction_module.db_location:
            Location.convert(_abstraction_module.db_location)
  for _function in _abstraction_module.db_functions:
      ModuleFunction.convert(_function)
        for _annotation in _abstraction_module.db_get_annotations():
            Annotation.convert(_annotation)
        _abstraction_module.portVisible = Set()
        _abstraction_module._registry = None
        _abstraction_module.abstraction = None


    ##########################################################################
    # Properties

    id = DBAbstractionRef.db_id
    cache = DBAbstractionRef.db_cache
    abstraction_id = DBAbstractionRef.db_abstraction_id
    location = DBAbstractionRef.db_location
    center = DBAbstractionRef.db_location
    version = DBAbstractionRef.db_version
    tag = DBAbstractionRef.db_name
    label = DBAbstractionRef.db_name
    name = 'Abstraction'
    package = 'edu.utah.sci.vistrails.basic'
    namespace = None
    annotations = DBAbstractionRef.db_annotations
    
    def _get_functions(self):
        self.db_functions.sort(key=lambda x: x.db_pos)
        return self.db_functions
    def _set_functions(self, functions):
  # want to convert functions to hash...?
        self.db_functions = functions
    functions = property(_get_functions, _set_functions)

    def _get_pipeline(self):
        from core.vistrail.pipeline import Pipeline
        import db.services.vistrail
        workflow = db.services.vistrail.materializeWorkflow(self.abstraction, 
                                                            self.version)
        Pipeline.convert(workflow)
        return workflow
    pipeline = property(_get_pipeline)

    def _get_registry(self):
        if not self._registry:
            self.make_registry()
        return self._registry
    registry = property(_get_registry)

    def add_annotation(self, annotation):
        self.db_add_annotation(annotation)
    def delete_annotation(self, annotation):
        self.db_delete_annotation(annotation)
    def has_annotation_with_key(self, key):
        return self.db_has_annotation_with_key(key)
    def get_annotation_by_key(self, key):
        return self.db_get_annotation_by_key(key)        

    def getNumFunctions(self):
        """getNumFunctions() -> int - Returns the number of functions """
        return len(self.functions)

    def summon(self):
        # we shouldn't ever call this since we're expanding abstractions
        return None

    @staticmethod
    def make_port_from_module(module, port_type):
        for function in module.functions:
            if function.name == 'name':
                port_name = function.params[0].strValue
            if function.name == 'spec':
                port_spec = function.params[0].strValue
        port = Port(id=-1,
                    name=port_name,
                    type=port_type)
        portSpecs = port_spec[1:-1].split(',')
        signature = []
        for s in portSpecs:
            spec = s.split(':', 2)
            signature.append(registry.get_descriptor_by_name(*spec).module)
        port.spec = core.modules.module_registry.PortSpec(signature)
        return port

    def make_registry(self):
        reg_module = \
            registry.get_descriptor_by_name('edu.utah.sci.vistrails.basic', 
                                            self.name).module
        self._registry = ModuleRegistry()
        self._registry.add_hierarchy(registry, self)
        for module in self.pipeline.module_list:
            if module.name == 'OutputPort':
                port = self.make_port_from_module(module, 'source')
                self._registry.add_port(reg_module, PortEndPoint.Source, port)
            elif module.name == 'InputPort':
                port = self.make_port_from_module(module, 'destination')
                self._registry.add_port(reg_module, PortEndPoint.Destination, 
                                        port)

    def sourcePorts(self):
        ports = []
        for module in self.pipeline.module_list:
            if module.name == 'OutputPort':
                ports.append(self.make_port_from_module(module, 'source'))
        return ports

    def destinationPorts(self):
        ports = []
        for module in self.pipeline.module_list:
            if module.name == 'InputPort':
                ports.append(self.make_port_from_module(module, 'destination'))
        return ports

    ##########################################################################
    # Operators
    
    def __str__(self):
        """__str__() -> str - Returns a string representation of an 
        AbstractionModule object. 

        """
        rep = '<abstraction_module id="%s" abstraction_id="%s" verion="%s">'
        rep += str(self.location)
        rep += str(self.functions)
        rep += str(self.annotations)
        rep += '</abstraction_module>'
        return  rep % (str(self.id), str(self.abstraction_id), 
                       str(self.version))

    def __eq__(self, other):
        """ __eq__(other: AbstractionModule) -> boolean
        Returns True if self and other have the same attributes. Used by == 
        operator. 
        
        """
        if type(other) != type(self):
            return False
        if self.location != other.location:
            return False
        if len(self.functions) != len(other.functions):
            return False
        if len(self.annotations) != len(other.annotations):
            return False
        for f,g in izip(self.functions, other.functions):
            if f != g:
                return False
        for f,g in izip(self.annotations, other.annotations):
            if f != g:
                return False
        return True

    def __ne__(self, other):
        return not self.__eq__(other)


################################################################################
# Testing

import unittest
from db.domain import IdScope

class TestModule(unittest.TestCase):

    def create_abstraction_module(self, id_scope=IdScope()):
        from core.vistrail.location import Location
        from core.vistrail.module_function import ModuleFunction
        from core.vistrail.module_param import ModuleParam

        params = [ModuleParam(id=id_scope.getNewId(ModuleParam.vtType),
                                  type='Int',
                                  val='1')]
        functions = [ModuleFunction(id=id_scope.getNewId(ModuleFunction.vtType),
                                    name='value',
                                    parameters=params)]
        location = Location(id=id_scope.getNewId(Location.vtType),
                            x=12.342,
                            y=-19.432)
        module = \
            AbstractionModule(id=id_scope.getNewId(AbstractionModule.vtType),
                              abstraction_id=1,
                              version=12,
                              location=location,
                              functions=functions)
        return module

    def test_copy(self):
        """Check that copy works correctly"""
        
        id_scope = IdScope()
        m1 = self.create_abstraction_module(id_scope)
        m2 = copy.copy(m1)
        self.assertEquals(m1, m2)
        self.assertEquals(m1.id, m2.id)
        m3 = m1.do_copy(True, id_scope, {})
        self.assertEquals(m1, m3)
        self.assertNotEquals(m1.id, m3.id)

    def test_serialization(self):
        """ Check that serialize and unserialize are working properly """
        import core.db.io

        m1 = self.create_abstraction_module()
        xml_str = core.db.io.serialize(m1)
        m2 = core.db.io.unserialize(xml_str, AbstractionModule)
        self.assertEquals(m1, m2)
        self.assertEquals(m1.id, m2.id)
