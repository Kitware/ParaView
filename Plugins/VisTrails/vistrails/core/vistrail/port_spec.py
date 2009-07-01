
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

from db.domain import DBPortSpec

class PortSpec(DBPortSpec):

    ##########################################################################
    # Constructors and copy

    def __init__(self, *args, **kwargs):
        DBPortSpec.__init__(self, *args, **kwargs)
        if self.id is None:
            self.id = -1
        
    def __copy__(self):
        return PortSpec.do_copy(self)

    def do_copy(self, new_ids=False, id_scope=None, id_remap=None):
        cp = DBPortSpec.do_copy(self, new_ids, id_scope, id_remap)
        cp.__class__ = PortSpec
        return cp

    @staticmethod
    def convert(_port_spec):
        _port_spec.__class__ = PortSpec

    ##########################################################################
    # Properties

    id = DBPortSpec.db_id
    name = DBPortSpec.db_name
    type = DBPortSpec.db_type
    spec = DBPortSpec.db_spec

    ##########################################################################
    # Operators
    
    def __str__(self):
        """__str__() -> str - Returns a string representation of an PortSpec
        object. 

        """
        rep = "<portSpec id=%s name=%s type=%s spec=%s />"
        return  rep % (str(self.id), str(self.name), 
                       str(self.type), str(self.spec))

    # FIXME expand this
    def __eq__(self, other):
        """ __eq__(other: PortSpec) -> boolean
        Returns True if self and other have the same attributes. Used by == 
        operator. 
        
        """
        if type(other) != type(self):
            return False
        return (self.name == other.name and
                self.type == other.type and
                self.spec == other.spec)

    def __ne__(self, other):
        return not self.__eq__(other)

################################################################################
# Testing

import unittest
import copy
from db.domain import IdScope

class TestPortSpec(unittest.TestCase):

    def create_port_spec(self, id_scope=IdScope()):
        # FIXME add a valid port spec
        port_spec = PortSpec(id=id_scope.getNewId(PortSpec.vtType),
                             name='SetValue',
                             type='input',
                             spec='valid spec goes here')
        return port_spec

    def test_copy(self):
        id_scope = IdScope()
        
        s1 = self.create_port_spec(id_scope)
        s2 = copy.copy(s1)
        self.assertEquals(s1, s2)
        self.assertEquals(s1.id, s2.id)
        s3 = s1.do_copy(True, id_scope, {})
        self.assertEquals(s1, s3)
        self.assertNotEquals(s1.id, s3.id)

    def test_serialization(self):
        import core.db.io
        s1 = self.create_port_spec()
        xml_str = core.db.io.serialize(s1)
        s2 = core.db.io.unserialize(xml_str, PortSpec)
        self.assertEquals(s1, s2)
        self.assertEquals(s1.id, s2.id)
