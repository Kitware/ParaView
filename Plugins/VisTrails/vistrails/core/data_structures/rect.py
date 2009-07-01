
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
from core.data_structures.point import Point

################################################################################
# Rect

class Rect(object):
    def __init__(self, lower_left=None, upper_right=None):
        """ Rect(lower_left: Point, upper_right: Point) -> Rect
        Creates a Rect given the lower_left and the upper_right points. It creates
        a copy of the given points. Return a Rect

        """        
        if lower_left == None:
            self.lower_left = Point()
        else:
            self.lower_left = copy.copy(lower_left)

        if upper_right == None:
            self.upper_right = Point()
        else:
            self.upper_right = copy.copy(upper_right)
        self.check_extent()

    @staticmethod
    def create(left, right, down, up):
        """ create(left: float, right: float, down: float, up: float) -> Point
        Creates a Rect from four float extents and return a Rect

        """
        return Rect(Point(min(left, right),
                          min(up, down)),
                    Point(max(left, right),
                          max(down, up)))
    
    def set_left(self, x):
        """ set_left(x: float) -> None
        Sets the left limit of the Rect and return nothing

        """
        self.lower_left.x = x

    def set_right(self, x):
        """ set_right(x: float) -> None
        Sets the right limit of the Rect and return nothing

        """
        self.upper_right.x = x

    def set_up(self,y):
        """ set_up(y: float) -> None
        Sets the upper limit of the Rect and return nothing

        """
        self.upper_right.y = y

    def set_down(self, y):
        """ set_down(y: float) -> None
        Sets the lower limit of the Rect and return nothing

        """
        self.lower_left.y = y

    def center(self):
        """ center() -> Point
        Compute the center of the Rect and return a Point

        """
        return (self.upper_right + self.lower_left) * 0.5

    def check_extent(self):
        """ check_extent() -> None
        Makes sure left limit is less than right limit, and lower limit is less
        than upper limit. In other words, ensures that, immediately after the
        call:
        self.lower_left.x <= self.upper_right.x and
        self.lower_left.y <= self.upper_right.y

        """
        if self.lower_left.x > self.upper_right.x:
            dlx = self.lower_left.x
            self.lower_left.x = self.upper_right.x
            self.upper_right.y = dlx

        if self.lower_left.y > self.upper_right.y:
            dly = self.lower_left.y
            self.lower_left.y = self.upper_right.y
            self.upper_right.y = dly

################################################################################
# Unit tests

import unittest
import random

class TestRect(unittest.TestCase):

    def test_create(self):
        """Exercises Rect.create()"""
        for i in xrange(100):
            a = random.uniform(-1.0, 1.0)
            b = random.uniform(-1.0, 1.0)
            c = random.uniform(-1.0, 1.0)
            d = random.uniform(-1.0, 1.0)

            r = Rect.create(a, b, c, d)
            assert r.lower_left.x == min(a, b)
            assert r.upper_right.x == max(a, b)
            assert r.lower_left.y == min(c, d)
            assert r.upper_right.y == max(c, d)

if __name__ == '__main__':
    unittest.main()
