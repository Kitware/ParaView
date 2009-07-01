
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
import math

################################################################################
# Point

class Point(object):
    """Point is a point in 2D. It behaves as a vector, too, because that's
    convenient."""

    ##########################################################################
    # Constructor
    
    def __init__(self, x=0, y=0):
        """ Point(x: float, y: float) -> Point
        Initialize and return a Point
        
        """
        self.x = float(x)
        self.y = float(y)

    # copy is implicit - all fields are immutable

    ##########################################################################

    def reset(self,x,y):
        """ reset(x: float, y: float) -> None
        Resets the point to given coordinates and return nothing

        """
        self.x = float(x)
        self.y = float(y)

    def is_inside(self, rect):
        """ is_inside(rect: Rect) -> boolean
        Check if the point is falling inside rect and return a boolean
        
        """
        return (self.x >= rect.lower_left.x and
                self.x <= rect.upper_right.x and
                self.y >= rect.lower_left.y and
                self.y <= rect.upper_right.y)
    
    def length(self):
        """ length() -> float
        Interprets self as a vector to compute the L_2 norm and return a float

        """
        return math.sqrt(self.x * self.x + self.y * self.y)

    ##########################################################################
    # Debugging

    def show_comparison(self, other):
        if type(self) != type(other):
            print "Type mismatch"
            return
        l = (self - other).length()
        if l >= self.eq_delta:
            print "Points are too far away:"
            print self
            print other
        else:
            print "No difference found: delta is %f" % l
            assert self == other

    ##########################################################################
    # Operators

    def __str__(self):
        return "(%f, %f)@%X" % (self.x, self.y, id(self))

    def __neg__(self):
        """ __neg__() -> Point
        Compute a point p such that: self + p == Point(0,0), and return a Point
        
        """
        return Point(-self.x,-self.y)

    def __add__(self, other):
        """ __add__(other: Point) -> Point
        Returns a point p such that: self + other == p, and return a Point
        
        """
        return Point(self.x + other.x, self.y + other.y)

    def __sub__(self, other):
        """ __sub__(other: Point) -> Point
        Returns a point p such that: self - other == p, and return a Point

        """
        return Point(self.x - other.x, self.y - other.y)

    def __mul__(self, other):
        """ __mul__(other: float) -> Point
        Interprets self as a vector to perform a scalar multiplication and
        return a Point

        """
        return Point(self.x * other, self.y * other)

    eq_delta = 0.0001
    def __eq__(self, other):
        """__eq__(other: Point) -> boolean 
        Two points are equal if they have the same components 
        
        """
        if type(self) != type(other):
            return False
        return (self.x - other.x) ** 2 + (self.y - other.y) ** 2 < 1e-8
        
    def __ne__(self, other):
        """__ne__(other: Point) -> boolean 
        Two points are differenr if they don't have the same components 
        
        """
        return not self.__eq__(other)

    def __rmul__(self, other):
        """ __rmul__(other: float) -> Point
        Interprets self as a vector to perform a scalar multiplication and
        return a Point

        """
        return Point(self.x * other, self.y * other)


################################################################################
# Unit tests

import unittest
import random

class TestPoint(unittest.TestCase):

    @staticmethod
    def assert_double_equals(a, b, eps = 0.00001):
        assert abs(a-b) < eps

    def test_add_length(self):
        """Uses triangle inequality to exercise add and length"""
        for i in xrange(100):
            x = Point(random.uniform(-1.0, 1.0), random.uniform(-1.0, 1.0))
            y = Point(random.uniform(-1.0, 1.0), random.uniform(-1.0, 1.0))
            assert (x+y).length() <= x.length() + y.length()

    def test_mul_length(self):
        """Uses vector space properties to exercise mul, rmul and length"""
        for i in xrange(100):
            x = Point(random.uniform(-1.0, 1.0), random.uniform(-1.0, 1.0))
            s = random.uniform(0.0, 10.0)
            self.assert_double_equals(s * x.length(), (s * x).length())
            self.assert_double_equals(s * x.length(), (x * s).length())

    def test_comparison_operators(self):
        """ Test comparison operators """
        a = Point(0, 1)
        b = Point(0, 1)
        assert a == b
        assert a != None
        b = Point(0, 0.1)
        assert a != b

if __name__ == '__main__':
    unittest.main()
        
