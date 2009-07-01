
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

from core.data_structures.point import Point
from db.domain import DBLocation

class Location(DBLocation, Point):

    ##########################################################################
    # Constructors and copy

    def __init__(self, *args, **kwargs):
        DBLocation.__init__(self, *args, **kwargs)
        if self.id is None:
            self.id = -1
        
    def __copy__(self):
        return Location.do_copy(self)

    def do_copy(self, new_ids=False, id_scope=None, id_remap=None):
        cp = DBLocation.do_copy(self, new_ids, id_scope, id_remap)
        cp.__class__ = Location
        return cp

    ##########################################################################
    # DB Conversion

    @staticmethod
    def convert(_location):
        _location.__class__ = Location

    ##########################################################################
    # Properties

    id = DBLocation.db_id
    x = DBLocation.db_x
    y = DBLocation.db_y
    
    ##########################################################################
    # Operators
    
    def __str__(self):
        """__str__() -> str - Returns a string representation of an Annotation
        object. 

        """
        rep = "<location id=%s x=%s y=%s/>"
        return  rep % (str(self.id), str(self.x), str(self.y))


    eq_delta = 0.0001
    def __eq__(self, other):
        """ __eq__(other: Location) -> boolean
        Returns True if self and other have the same attributes. Used by == 
        operator. 
        
        """
        if type(other) != type(self):
            return False
        # Skip property lookup for performance
        return ((self._db_x - other._db_x) ** 2 +
                (self._db_y - other._db_y)) ** 2 < 1e-8

    def __ne__(self, other):
        return not self.__eq__(other)

    def __neg__(self):
        """ __neg__() -> Location
        Compute a point p such that: self + p == Location(0,0), 
        and return a Location
        
        """
        return Location(x=-self.db_x,y=-self.db_y)

    def __add__(self, other):
        """ __add__(other: Location) -> Location
        Returns a point p such that: self + other == p, and return a Location
        
        """
        return Location(x=(self.db_x + other.db_x), y=(self.db_y + other.db_y))

    def __sub__(self, other):
        """ __sub__(other: Location) -> Location
        Returns a point p such that: self - other == p, and return a Location

        """
        return Location(x=(self.db_x - other.db_x), y=(self.db_y - other.db_y))

    def __mul__(self, other):
        """ __mul__(other: float) -> Location
        Interprets self as a vector to perform a scalar multiplication and
        return a Location

        """
        return Location(x=(self.db_x * other), y=(self.db_y * other))

    def __rmul__(self, other):
        """ __rmul__(other: float) -> Location
        Interprets self as a vector to perform a scalar multiplication and
        return a Location

        """
        return Location(x=(self.db_x * other), y=(self.db_y * other))
    
################################################################################
# Testing

import unittest
import copy
import random
from db.domain import IdScope

class TestLocation(unittest.TestCase):

    @staticmethod
    def assert_double_equals(a, b, eps = 0.00001):
        assert abs(a-b) < eps

    def create_location(self, id_scope=IdScope()):
        location = Location(id=id_scope.getNewId(Location.vtType),
                            x=12.34567,
                            y=14.65431)
        return location

    def test_copy(self):
        id_scope = IdScope()
        
        loc1 = self.create_location(id_scope)
        loc2 = copy.copy(loc1)
        self.assertEquals(loc1, loc2)
        self.assertEquals(loc1.id, loc2.id)
        loc3 = loc1.do_copy(True, id_scope, {})
        self.assertEquals(loc1, loc3)
        self.assertNotEquals(loc1.id, loc3.id)

    def test_serialization(self):
        import core.db.io
        loc1 = self.create_location()
        xml_str = core.db.io.serialize(loc1)
        loc2 = core.db.io.unserialize(xml_str, Location)
        self.assertEquals(loc1, loc2)
        self.assertEquals(loc1.id, loc2.id)

    def test_add_length(self):
        """Uses triangle inequality to exercise add and length"""
        for i in xrange(100):
            x = Location(x=random.uniform(-1.0, 1.0), y=random.uniform(-1.0, 1.0))
            y = Location(x=random.uniform(-1.0, 1.0), y=random.uniform(-1.0, 1.0))
            assert (x+y).length() <= x.length() + y.length()

    def test_mul_length(self):
        """Uses vector space properties to exercise mul, rmul and length"""
        for i in xrange(100):
            x = Location(x=random.uniform(-1.0, 1.0), y=random.uniform(-1.0, 1.0))
            s = random.uniform(0.0, 10.0)
            self.assert_double_equals(s * x.length(), (s * x).length())
            self.assert_double_equals(s * x.length(), (x * s).length())

    def test_comparison_operators(self):
        """ Test comparison operators """
        a = Location(x=0, y=1)
        b = Location(x=0, y=1)
        assert a == b
        assert a != None
        b = Location(x=0, y=0.1)
        assert a != b
