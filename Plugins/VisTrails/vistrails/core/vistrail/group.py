
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

from core.vistrail.annotation import Annotation
from core.vistrail.location import Location
from core.vistrail.module import Module
from core.vistrail.module_function import ModuleFunction
from core.vistrail.port import Port, PortEndPoint
from db.domain import DBGroup

from core.utils import NoSummon, VistrailsInternalError, report_stack
import core.modules.module_registry
from core.modules.module_registry import registry, ModuleRegistry

class Group(DBGroup, Module):

    ##########################################################################
    # Constructors and copy

    def __init__(self, *args, **kwargs):
        if 'pipeline' in kwargs:
            kwargs['workflow'] = kwargs['pipeline']
            del kwargs['pipeline']
        DBGroup.__init__(self, *args, **kwargs)
        if self.cache is None:
            self.cache = 1
        if self.id is None:
            self.id = -1
        if self.location is None:
            self.location = Location(x=-1.0, y=-1.0)
        if self.name is None:
            self.name = ''
        if self.package is None:
            self.package = ''
        if self.version is None:
            self.version = ''
        self.portVisible = Set()
        self._registry = None

    def __copy__(self):
        return Group.do_copy(self)

    def do_copy(self, new_ids=False, id_scope=None, id_remap=None):
        cp = DBGroup.do_copy(self, new_ids, id_scope, id_remap)
        cp.__class__ = Group
        cp._registry = None
#         for port_spec in cp.db_portSpecs:
#             cp.add_port_to_registry(port_spec)
        cp.portVisible = copy.copy(self.portVisible)
        return cp

    @staticmethod
    def convert(_group):
        if _group.__class__ == Group:
            return
        _group.__class__ = Group
        _group._registry = None
        _group.portVisible = Set()
        if _group.db_location:
            Location.convert(_group.db_location)
        if _group.db_workflow:
            from core.vistrail.pipeline import Pipeline
            Pipeline.convert(_group.db_workflow)
  for _function in _group.db_functions:
      ModuleFunction.convert(_function)
        for _annotation in _group.db_get_annotations():
            Annotation.convert(_annotation)


    ##########################################################################
    # Properties

    # We need to repeat these here because Module uses DBModule. ...
    id = DBGroup.db_id
    cache = DBGroup.db_cache
    annotations = DBGroup.db_annotations
    location = DBGroup.db_location
    center = DBGroup.db_location
    name = DBGroup.db_name
    label = DBGroup.db_name
    namespace = DBGroup.db_namespace
    package = DBGroup.db_package
    tag = DBGroup.db_tag
    version = DBGroup.db_version

    def summon(self):
        # define this so that pipeline is copied over...
        pass

    def is_group(self):
        return True

    pipeline = DBGroup.db_workflow
    
    def _get_registry(self):
        if not self._registry:
            # print 'making registry'
            self.make_registry()
        return self._registry
    registry = property(_get_registry)

    # override these from the Module class with defaults
    def _get_port_specs(self):
        return dict()
    port_specs = property(_get_port_specs)
    def has_portSpec_with_name(self, name):
        return False
    def get_portSpec_by_name(self, name):
        return None

    @staticmethod
    def make_port_from_module(module, port_type):
        for function in module.functions:
            if function.name == 'name':
                port_name = function.params[0].strValue
                print '  port_name:', port_name
            if function.name == 'spec':
                port_spec = function.params[0].strValue
                #print '  port_spec:',  port_spec
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
            print 'module:', module.name
            if module.name == 'OutputPort':
                port = self.make_port_from_module(module, 'source')
                self._registry.add_port(reg_module, PortEndPoint.Source, port)
            elif module.name == 'InputPort':
                port = self.make_port_from_module(module, 'destination')
                self._registry.add_port(reg_module, PortEndPoint.Destination, 
                                        port)

    def sourcePorts(self):
        return self.registry.module_source_ports(False, self.package,
                                                 self.name, self.namespace)

    def destinationPorts(self):
        return self.registry.module_destination_ports(False, self.package, 
                                                      self.name, self.namespace)

    ##########################################################################
    # Operators
    
    def __str__(self):
        """__str__() -> str - Returns a string representation of an 
        GroupModule object. 

        """
        rep = '<group id="%s" abstraction_id="%s" verion="%s">'
        rep += str(self.location)
        rep += str(self.functions)
        rep += str(self.annotations)
        rep += '</group>'
        return  rep % (str(self.id), str(self.abstraction_id), 
                       str(self.version))

    def __eq__(self, other):
        """ __eq__(other: GroupModule) -> boolean
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

class TestGroup(unittest.TestCase):

    def create_group(self, id_scope=IdScope()):
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
            Group(id=id_scope.getNewId(Group.vtType),
                  location=location,
                  functions=functions,
                  )
        return module

    def test_copy(self):
        """Check that copy works correctly"""
        
        id_scope = IdScope()
        m1 = self.create_group(id_scope)
        m2 = copy.copy(m1)
        self.assertEquals(m1, m2)
        self.assertEquals(m1.id, m2.id)
        m3 = m1.do_copy(True, id_scope, {})
        self.assertEquals(m1, m3)
        self.assertNotEquals(m1.id, m3.id)

    def test_serialization(self):
        """ Check that serialize and unserialize are working properly """
        import core.db.io

        m1 = self.create_group()
        xml_str = core.db.io.serialize(m1)
        m2 = core.db.io.unserialize(xml_str, Group)
        self.assertEquals(m1, m2)
        self.assertEquals(m1.id, m2.id)
