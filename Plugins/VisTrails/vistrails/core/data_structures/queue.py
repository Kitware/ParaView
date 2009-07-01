
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

################################################################################

class Queue(object):

    def __init__(self, capacity=8):
        """ Queue(capacity: int) -> Queue
        Initialize the queue with a default capacity of zero in size

        """
        self.__buffer = [None] * capacity
        self.__capacity = capacity
        self.__size = 0
        self.__begin = 0
        self.__end = 0
        
    def __len__(self):
        """ __len__() -> int
        Compute the number of elements when using the built-in len()
        and return an int

        """
        l = self.__end - self.__begin
        if l < 0:
            return self.__capacity + l
        else:
            return l
        
    def front(self):
        """ front() -> element type
        Get the front of the queue and return an element type

        """
        return self.__buffer[self.__begin]
    
    def back(self):
        """ back() -> element type
        Get the back (last) element of the queue and return an element type

        """
        return self.__buffer[(self.__end + self.__capacity - 1) %
                             self.__capacity]
    
    def push(self, obj):
        """ push(obj: element type) -> None
        Push obj onto the back queue and return nothing

        """
        # olen = len(self)
        if (self.__end + 1) % self.__capacity == self.__begin:
            self.__rebuffer(self.__capacity * 2)
        self.__buffer[self.__end] = obj
        self.__end += 1
        if self.__end == self.__capacity:
            self.__end = 0
        # nlen = len(self)
        # assert nlen == olen + 1

    class EmptyQueue(Exception):
        pass
    
    def pop(self):
        """ pop() -> element type
        Pop the front element of the queue and return an element type

        """
        # olen = len(self)
        if len(self) == 0:
            raise self.EmptyQueue()
        r = self.__buffer[self.__begin]
        self.__buffer[self.__begin] = None
        self.__begin += 1
        if self.__begin == self.__capacity:
            self.__begin = 0
        if self.__capacity > 8 and (self.__capacity / len(self)) >= 4:
            self.__rebuffer(self.__capacity / 2)
        # nlen = len(self)
        # assert olen == nlen + 1
        return r

    def capacity(self):
        return self.__capacity

    def __rebuffer(self, newcapacity):
        """ __rebuffer(newcapacity: int) -> none
        Reallocate the internal buffer to fit at newcapacity elements and
        return nothing

        """
        nb = [None] * newcapacity
        if self.__begin < self.__end:
            l = self.__end - self.__begin
            nb[0:l] = self.__buffer[self.__begin:self.__end]
            self.__begin = 0
            self.__end = l
        else:
            l1 = self.__capacity - self.__begin
            l2 = l1 + self.__end
            nb[0:l1] = self.__buffer[self.__begin:]
            nb[l1:l2] = self.__buffer[:self.__end]
            self.__begin = 0
            self.__end = l2
        self.__buffer = nb
        self.__capacity = newcapacity
        
    def __str__(self):
        """ __str__() -> str
        Format the queue for serialization and return a string

        """
        return str(self.__buffer)

################################################################################

import unittest
import random

class TestQueue(unittest.TestCase):
    
    def test_basic(self):
        """Test push/pop operations"""
        q = Queue()
        q.push(1)
        q.push(2)
        q.push(3)
        q.push(4)
        self.assertEquals(q.pop(), 1)
        self.assertEquals(q.pop(), 2)
        self.assertEquals(q.pop(), 3)
        self.assertEquals(q.pop(), 4)
        
    def test_expand_basic(self):
        """Test if the queue is expanding its capacity right with push()"""
        q = Queue()
        for i in xrange(512):
            assert len(q) == i
            q.push(i)
            assert len(q) == i+1
        self.assertEquals(len(q), 512)
        for i in xrange(512):
            assert len(q) == 512 - i
            self.assertEquals(q.pop(), i)
            assert len(q) == 511 - i
            
    def test_expand_contract(self):
        """Test if the queue is expanding and contracting with push()/pop()"""
        def run_it():
            pushed = 0
            popped = 0
            q = Queue()
            for t in xrange(100):
                for i in xrange(100):
                    # Test expand with high probability
                    a = random.choice([0,0,0,0,0,1])
                    if (a == 0) or (len(q) == 0):
                        olen = len(q)
                        q.push(pushed)
                        nlen = len(q)
                        assert nlen == olen+1, "push invariant"
                        pushed += 1
                    else:
                        olen = len(q)
                        v = q.pop()
                        nlen = len(q)
                        assert olen == nlen+1, "pop invariant"
                        self.assertEquals(v, popped)
                        popped += 1
                for i in xrange(100):
                    # Test contract with high probability
                    a = random.choice([1,1,1,1,1,0])
                    if (a == 0) or (len(q) == 0):
                        olen = len(q)
                        q.push(pushed)
                        nlen = len(q)
                        assert nlen == olen+1, "push invariant"
                        pushed += 1
                    else:
                        olen = len(q)
                        v = q.pop()
                        nlen = len(q)
                        assert olen == nlen+1, "pop invariant"
                        self.assertEquals(v, popped)
                        popped += 1
        # run test 100 times
        for i in xrange(1):
            run_it()

    def test_pop(self):
        x = Queue()
        try:
            x.pop()
        except x.EmptyQueue:
            pass
        else:
            assert False

if __name__ == '__main__':
    unittest.main()
