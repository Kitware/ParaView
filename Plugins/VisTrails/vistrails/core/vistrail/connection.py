
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
""" This python module defines Connection class.
"""

import copy
from db.domain import DBConnection
import core.modules.module_registry
from core.modules.vistrails_module import ModuleConnector
from core.utils import VistrailsInternalError
from core.vistrail.port import PortEndPoint, Port

registry = core.modules.module_registry.registry

################################################################################

def moduleConnection(conn):
    """moduleConnection(conn)-> function 
    Returns a function to build a module connection

    """
    def theFunction(src, dst):
        iport = conn.destination.name
        oport = conn.source.name
        src.enableOutputPort(oport)
        dst.set_input_port(iport, ModuleConnector(src, oport))
    return theFunction

################################################################################

class Connection(DBConnection):
    """ A Connection is a connection between two modules.
    Right now there's only Module connections.

    """

    ##########################################################################
    # Constructors and copy
    
    @staticmethod
    def fromPorts(source, dest):
        """fromPorts(source: Port, dest: Port) -> Connection
        Static method that creates a Connection given source and 
        destination ports.

        """
        conn = Connection()
        conn.source = copy.copy(source)
        conn.destination = copy.copy(dest)
        return conn
        
    @staticmethod
    def fromID(id):
        """fromTypeID(id: int) -> Connection
        Static method that creates a Connection given an id.

        """
        conn = Connection()
        conn.id = id
        conn.source.endPoint = PortEndPoint.Source
        conn.destination.endPoint = PortEndPoint.Destination
        return conn
    
    def __init__(self, *args, **kwargs):
        """__init__() -> Connection 
        Initializes source and destination ports.
        
        """
  DBConnection.__init__(self, *args, **kwargs)
        if self.id is None:
            self.db_id = -1
        if not len(self.ports) > 0:
            self.source = Port(type='source')
            self.destination = Port(type='destination')
#             self.source.endPoint = PortEndPoint.Source
#             self.destination.endPoint = PortEndPoint.Destination
        self.makeConnection = moduleConnection(self)

    def __copy__(self):
        """__copy__() -> Connection -  Returns a clone of self.
        
        """
        return Connection.do_copy(self)

    def do_copy(self, new_ids=False, id_scope=None, id_remap=None):
        cp = DBConnection.do_copy(self, new_ids, id_scope, id_remap)
        cp.__class__ = Connection
        cp.makeConnection = moduleConnection(cp)
        for port in cp.ports:
            Port.convert(port)
        return cp

    ##########################################################################

    @staticmethod
    def convert(_connection):
#  print "ports: %s" % _Connection._get_ports(_connection)
        if _connection.__class__ == Connection:
            return
        _connection.__class__ = Connection

        for port in _connection.ports:
            Port.convert(port)

#         _connection.sourceInfo = \
#       (_connection.source.moduleName, _connection.source.sig)
#         _connection.destinationInfo = \
#       (_connection.destination.moduleName, _connection.destination.sig)
# #        print _connection.sourceInfo
# #        print _connection.destinationInfo
#         portFromRepresentation = registry.portFromRepresentation
#         newSource = \
#       portFromRepresentation(_connection.source.moduleName, 
#            _connection.source.sig,
#            PortEndPoint.Source, None, True)
#   newDestination = \
#       portFromRepresentation(_connection.destination.moduleName,
#            _connection.destination.sig,
#            PortEndPoint.Destination, None, True)
#   newSource.moduleId = _connection.source.moduleId
#   newDestination.moduleId = _connection.destination.moduleId
#   _connection.source = newSource
#   _connection.destination = newDestination
        _connection.makeConnection = moduleConnection(_connection)


    ##########################################################################
    # Debugging

    def show_comparison(self, other):
        if type(other) != type(self):
            print "Type mismatch"
            return
        if self.__source != other.__source:
            print "Source mismatch"
            self.__source.show_comparison(other.__source)
            return
        if self.__dest != other.__dest:
            print "Dest mismatch"
            self.__dest.show_comparison(other.__dest)
            return
        print "no difference found"
        assert self == other

    ##########################################################################
    # Properties

    id = DBConnection.db_id
    ports = DBConnection.db_ports
    
    def add_port(self, port):
        self.db_add_port(port)

    def _get_sourceId(self):
        """ _get_sourceId() -> int
        Returns the module id of source port. Do not use this function, 
        use sourceId property: c.sourceId 

        """
        return self.source.moduleId

    def _set_sourceId(self, id):
        """ _set_sourceId(id : int) -> None 
        Sets this connection source id. It updates both self.__source.moduleId
        and self.__source.id. Do not use this function, use sourceId 
        property: c.sourceId = id

        """
        self.source.moduleId = id
        self.source.id = id
    sourceId = property(_get_sourceId, _set_sourceId)

    def _get_destinationId(self):
        """ _get_destinationId() -> int
        Returns the module id of dest port. Do not use this function, 
        use sourceId property: c.destinationId 

        """
        return self.destination.moduleId

    def _set_destinationId(self, id):
        """ _set_destinationId(id : int) -> None 
        Sets this connection destination id. It updates self.__dest.moduleId. 
        Do not use this function, use destinationId property: 
        c.destinationId = id

        """
        self.destination.moduleId = id
    destinationId = property(_get_destinationId, _set_destinationId)

    def _get_type(self):
        """_get_type() -> VistrailModuleType - Returns this connection type.
        Do not use this function, use type property: c.type = t 

        """
        return self.source.type

    def _set_type(self, t):
        """ _set_type(t: VistrailModuleType) -> None 
        Sets this connection type and updates self.__source.type and 
        self.__dest.type. It also updates the correct makeConnection function.
        Do not use this function, use type property: c.type = t

        """
        self.source.type = t
        self.destination.type = t
        self.updateMakeConnection()
    type = property(_get_type, _set_type)

    def _get_source(self):
        """_get_source() -> Port
        Returns source port. Do not use this function, use source property: 
        c.source 

        """
        try:
            return self.db_get_port_by_type('source')
        except KeyError:
            pass
        return None

    def _set_source(self, source):
        """_set_source(source: Port) -> None 
        Sets this connection source port. It also updates this connection 
        makeConnection function. Do not use this function, use source 
        property instead: c.source = source

        """
        try:
            port = self.db_get_port_by_type('source')
            self.db_delete_port(port)
        except KeyError:
            pass
        if source is not None:
            self.db_add_port(source)
    source = property(_get_source, _set_source)

    def _get_destination(self):
        """_get_destination() -> Port
        Returns destination port. Do not use this function, use destination
        property: c.destination 

        """
#  return self.db_ports['destination']
        try:
            return self.db_get_port_by_type('destination')
        except KeyError:
            pass
        return None

    def _set_destination(self, dest):
        """_set_destination(dest: Port) -> None 
        Sets this connection destination port. It also updates this connection 
        makeConnection function. Do not use this function, use destination 
        property instead: c.destination = dest

        """
        try:
            port = self.db_get_port_by_type('destination')
            self.db_delete_port(port)
        except KeyError:
            pass
        if dest is not None:
            self.db_add_port(dest)
    destination = property(_get_destination, _set_destination)
    dest = property(_get_destination, _set_destination)

    ##########################################################################
    # Operators

    def __str__(self):
        """__str__() -> str - Returns a string representation of a Connection
        object. 

        """
        rep = "<connection id='%s'>%s%s</connection>"
        return  rep % (str(self.id), str(self.source), str(self.destination))

    def __ne__(self, other):
        return not self.__eq__(other)

    def __eq__(self, other):
        if type(other) != type(self):
            return False
        return (self.source == other.source and
                self.dest == other.dest)

    def equals_no_id(self, other):
        """Checks equality up to ids (connection and ports)."""
        if type(self) != type(other):
            return False
        return (self.source.equals_no_id(other.source) and
                self.dest.equals_no_id(other.dest))
    
################################################################################
# Testing

import unittest
from db.domain import IdScope

class TestConnection(unittest.TestCase):

    def create_connection(self, id_scope=IdScope()):
        from core.vistrail.port import Port

        source = Port(id=id_scope.getNewId(Port.vtType),
                      type='source', 
                      moduleId=21L, 
                      moduleName='String', 
                      name='self',
                      spec='edu.sci.utah.vistrails.basic:String')
        destination = Port(id=id_scope.getNewId(Port.vtType),
                           type='destination',
                           moduleId=20L,
                           moduleName='Float',
                           name='self',
                           spec='edu.sci.utah.vistrails.basic:Float')
        connection = Connection(id=id_scope.getNewId(Connection.vtType),
                                ports=[source, destination])
        return connection

    def test_copy(self):
        id_scope = IdScope()
        
        c1 = self.create_connection(id_scope)
        c2 = copy.copy(c1)
        self.assertEquals(c1, c2)
        self.assertEquals(c1.id, c2.id)
        c3 = c1.do_copy(True, id_scope, {})
        self.assertEquals(c1, c3)
        self.assertNotEquals(c1.id, c3.id)

    def test_serialization(self):
        import core.db.io
        c1 = self.create_connection()
        xml_str = core.db.io.serialize(c1)
        c2 = core.db.io.unserialize(xml_str, Connection)
        self.assertEquals(c1, c2)
        self.assertEquals(c1.id, c2.id)

    def testModuleConnection(self):
        a = Connection.fromID(0)
        c = moduleConnection(a)
        def bogus(asd):
            return 3
        assert type(c) == type(bogus)

    def testEmptyConnection(self):
        """Tests sane initialization of empty connection"""
        c = Connection()
        self.assertEquals(c.source.endPoint, PortEndPoint.Source)
        self.assertEquals(c.destination.endPoint, PortEndPoint.Destination)
        
if __name__ == '__main__':
    unittest.main()
