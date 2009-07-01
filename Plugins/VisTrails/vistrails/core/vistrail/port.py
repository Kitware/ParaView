
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
    * Port
    * PortEndPoint

 """
from db.domain import DBPort
from core.utils import enum
from core.utils import VistrailsInternalError, all
import core.modules.vistrails_module
import __builtin__
import copy

################################################################################

PortEndPoint = enum('PortEndPoint',
                    ['Invalid', 'Source', 'Destination'])

################################################################################

class Port(DBPort):
    """ A port denotes one endpoint of a Connection.

    self.spec: list of list of (module, str) 
    
    """

    ##########################################################################
    # Constructor and copy
    
    def __init__(self, *args, **kwargs):
        if 'optional' in kwargs:
            self.optional = kwargs['optional']
            del kwargs['optional']
        else:
            self.optional = False
  DBPort.__init__(self, *args, **kwargs)
        if self.db_id is None:
            self.db_id = -1
        if self.db_moduleId is None:
            self.db_moduleId = 0
        if self.db_moduleName is None:
            self.db_moduleName = ""
        if self.db_name is None:
            self.db_name = ""
        if self.db_spec is None:
            self.db_spec = ""
        
        self.sort_key = -1
        self._spec = None

    def __copy__(self):
        return Port.do_copy(self)

    def do_copy(self, new_ids=False, id_scope=None, id_remap=None):
        cp = DBPort.do_copy(self, new_ids, id_scope, id_remap)
        cp.__class__ = Port
        cp.optional = self.optional
        cp.sort_key = self.sort_key
        cp.spec = copy.copy(self.spec)
        return cp

    @staticmethod
    def convert(_port):
        if _port.__class__ == Port:
            return
        _port.__class__ = Port

        _port.optional = False
        _port.sort_key = -1
        _port._spec = None

    ##########################################################################
    # Properties
    
    id = DBPort.db_id
    moduleId = DBPort.db_moduleId
    moduleName = DBPort.db_moduleName
    name = DBPort.db_name
    type = DBPort.db_type
    
    def _get_endPoint(self):
  map = {'source': PortEndPoint.Source,
         'destination': PortEndPoint.Destination}
  endPoint = self.db_type
        try:
            return map[endPoint]
        except KeyError:
            return PortEndPoint.Invalid
    def _set_endPoint(self, endPoint):
  map = {PortEndPoint.Source: 'source',
         PortEndPoint.Destination: 'destination'}
        try:
            self.db_type = map[endPoint]
        except KeyError:
            self.db_type = ''
    endPoint = property(_get_endPoint, _set_endPoint)

    def _get_specStr(self):
        if self.db_spec is None:
            self.db_spec = self._spec.create_sigstring()
        return self.db_spec
    def _set_specStr(self, specStr):
        self.db_spec = specStr
    specStr = property(_get_specStr, _set_specStr)

    def _get_spec(self):
        return self._spec
    
    _port_to_string_map = {PortEndPoint.Invalid: "Invalid",
          PortEndPoint.Source: "Output",
          PortEndPoint.Destination: "Input"}
    def _set_spec(self, spec):
        self._spec = spec
        if self._spec is not None:
            self.db_spec = self._spec._long_sigstring
            self.__tooltip = (self._port_to_string_map[self.endPoint] +
                              " port " +
                              self.name +
                              "\n" +
                              self._spec._short_sigstring)
        else:
            self.__tooltip = ""

    spec = property(_get_spec, _set_spec)

    def _get_sig(self):
        return self.name + self.specStr
    sig = property(_get_sig)

    def toolTip(self):
        """ toolTip() -> str
        Generates an appropriate tooltip for the port. 
        
        """
        return self.__tooltip

    ##########################################################################
    # Debugging

    def show_comparison(self, other):
        if type(self) != type(other):
            print "Type mismatch"
        elif self.endPoint != other.endPoint:
            print "endpoint mismatch"
        elif self.moduleName != other.moduleName:
            print "moduleName mismatch"
        elif self.name != other.name:
            print "name mismatch"
        elif self.spec != other.spec:
            print "spec mismatch"
        elif self.optional != self.optional:
            print "optional mismatch"
        elif self.sort_key != self.sort_key:
            print "sort_key mismatch"
        else:
            print "no difference found"
            assert self == other

    ##########################################################################
    # Operators

    def __ne__(self, other):
        return not self.__eq__(other)

    def __eq__(self, other):
        if type(self) != type(other):
            return False
        return (self.endPoint == other.endPoint and
# FIXME module id can change...
#                self.moduleId == other.moduleId and
                self.moduleName == other.moduleName and
                self.name == other.name and
                self.specStr == other.specStr and
                self.optional == other.optional and
                self.sort_key == other.sort_key)
    
    def __str__(self):
        """ __str__() -> str 
        Returns a string representation of a Port object.  """
        return '<Port endPoint="%s" moduleId=%s name=%s ' \
            'type=Module %s/>' % (self.endPoint,
                                  self.moduleId,
                                  self.name,
                                  self.spec)

    def equals_no_id(self, other):
        if type(self) != type(other):
            return False
        return (self.endPoint == other.endPoint and
                self.moduleName == other.moduleName and
                self.name == other.name and
                self.specStr == other.specStr and
                self.optional == other.optional and
                self.sort_key == other.sort_key)

    def key_no_id(self):
        """key_no_id(): tuple. returns a tuple that identifies
        the port without caring about ids. Used for sorting
        port lists."""
        return (self.endPoint,
                self.moduleName,
                self.name,
                self.specStr,
                self.optional,
                self.sort_key)

###############################################################################

import unittest
from db.domain import IdScope

if __name__ == '__main__':
    import core.modules.basic_modules
    import core.modules.module_registry
    
class TestPort(unittest.TestCase):
    def setUp(self):
        self.registry = core.modules.module_registry.registry

    def create_port(self, id_scope=IdScope()):
        port = Port(id=id_scope.getNewId(Port.vtType),
                    type='source',
                    moduleId=12L, 
                    moduleName='String', 
                    name='self',
                    spec='edu.utah.sci.vistrails.basic:String')
        return port

    def test_copy(self):
        id_scope = IdScope()
        
        p1 = self.create_port(id_scope)
        p2 = copy.copy(p1)
        self.assertEquals(p1, p2)
        self.assertEquals(p1.id, p2.id)
        p3 = p1.do_copy(True, id_scope, {})
        self.assertEquals(p1, p3)
        self.assertNotEquals(p1.id, p3.id)

    def test_serialization(self):
        import core.db.io
        p1 = self.create_port()
        xml_str = core.db.io.serialize(p1)
        p2 = core.db.io.unserialize(xml_str, Port)
        self.assertEquals(p1, p2)
        self.assertEquals(p1.id, p2.id)

    def testPort(self):
        x = Port()
        a = str(x)

    def test_registry_port_subtype(self):
        """Test registry isPortSubType"""
        descriptor = self.registry.get_descriptor_by_name('edu.utah.sci.vistrails.basic',
                                                          'String')
        ports = self.registry.source_ports_from_descriptor(descriptor)
        assert self.registry.is_port_sub_type(ports[0], ports[0])

    def test_registry_ports_can_connect(self):
        """Test registry isPortSubType"""
        descriptor = self.registry.get_descriptor_by_name('edu.utah.sci.vistrails.basic',
                                                          'String')
        oport = self.registry.source_ports_from_descriptor(descriptor)[0]
        iport = self.registry.destination_ports_from_descriptor(descriptor)[0]
        assert self.registry.ports_can_connect(oport, iport)


if __name__ == '__main__':
    unittest.main()

