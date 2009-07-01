
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
"""Simple stack data structure."""

###############################################################################

class EmptyStack(Exception):
    pass


class Stack(object):

    def __init__(self):
        object.__init__(self)
        self.__cell = []
        self.__size = 0

    def top(self):
        """Returns the top of the stack."""
        if not self.__cell:
            raise EmptyStack()
        else:
            return self.__cell[0]

    def push(self, obj):
        """Pushes an element onto the stack."""
        self.__cell = [obj, self.__cell]
        self.__size += 1

    def pop(self):
        """Pops the top off of the stack."""
        if not self.__cell:
            raise EmptyStack()
        else:
            self.__cell = self.__cell[1]
            self.__size -= 1

    def __len__(self):
        return self.__size

    def __get_size(self):
        return self.__size

    size = property(__get_size, doc="The size of the stack.")

###############################################################################

import unittest

class TestStack(unittest.TestCase):

    def test_basic(self):
        s = Stack()
        self.assertEquals(s.size, 0)
        s.push(10)
        self.assertEquals(s.top(), 10)
        self.assertEquals(len(s), 1)
        s.pop()
        self.assertEquals(len(s), 0)
        self.assertEquals(0, s.size)

    def test_pop_empty_raises(self):
        s = Stack()
        self.assertRaises(EmptyStack, s.pop)

    def test_top_empty_raises(self):
        s = Stack()
        self.assertRaises(EmptyStack, s.top)

if __name__ == '__main__':
    unittest.main()
