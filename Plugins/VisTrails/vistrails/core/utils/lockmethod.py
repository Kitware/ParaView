
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
def lock_method(lock):
    
    def decorator(method):
        def decorated(self, *args, **kwargs):
            try:
                lock.acquire()
                result = method(self, *args, **kwargs)
            finally:
                lock.release()
            return result
        decorated.func_doc = method.__doc__
        return decorated

    return decorator

##############################################################################

import unittest
import threading

class TestLockMethod(unittest.TestCase):

    lock = threading.Lock()
    @lock_method(lock)
    def foo(self):
        self.assertEquals(self.lock.locked(), True)

    @lock_method(lock)
    def foo_throws(self):
        raise Exception()

    @lock_method(lock)
    def foo_finally(self):
        try:
            raise Exception
            return False
        finally:
            return True

    @lock_method(lock)
    def foo_docstring(self):
        """FOO"""
        pass

    def test_common(self):
        self.assertEquals(self.lock.locked(), False)
        self.foo()
        self.assertEquals(self.lock.locked(), False)

    def test_throws(self):
        self.assertEquals(self.lock.locked(), False)
        self.assertRaises(Exception, self.foo_throws)
        self.assertEquals(self.lock.locked(), False)

    def test_finally(self):
        self.assertEquals(self.lock.locked(), False)
        self.assertEquals(self.foo_finally(), True)
        self.assertEquals(self.lock.locked(), False)

    def test_docstring(self):
        self.assertEquals(self.foo_docstring.__doc__, "FOO")
        

if __name__ == '__main__':
    unittest.main()
