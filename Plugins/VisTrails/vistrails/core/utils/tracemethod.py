
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
"""Defines a trace decorator that traces function calls. This is not currently
thread-safe. It won't crash, but the bump() printouts might not be correct.

Also defines report_stack, a decorator that dumps the traceback whenever
a method gets called."""

import sys
import traceback
from core.data_structures.stack import Stack
_output_file = sys.stderr

__current_method_name = Stack()

def _indent():
    _output_file.write(' ' * (len(__current_method_name)-1))

def trace_method_options(method,
                         with_args=False,
                         with_kwargs=False,
                         with_return=False):
    """trace_method_options is a method decorator that traces
    entry-exit of functions. It also prints args, kwargs and return
    values if optional parameters with_args, with_kwargs and
    with_return are set to True."""
    def decorated(self, *args, **kwargs):
        __current_method_name.push([method.__name__, 0])
        try:
            _indent()
            _output_file.write(method.__name__ +  ".enter")
            if with_args:
                _output_file.write(" (args: ")
                _output_file.write(str([str(arg) for arg in args]))
                _output_file.write(")")
            if with_kwargs:
                _output_file.write(" (kwargs: ")
                kwarglist = [(k, str(v)) for (k,v) in kwargs.iteritems()]
                kwarglist.sort()
                _output_file.write(str(kwarglist))
                _output_file.write(")")
            _output_file.write('\n')
            result = method(self, *args, **kwargs)
            _indent()
            _output_file.write(method.__name__ + ".exit")
            if with_return:
                _output_file.write(" (return: %s)" % str(result))
            _output_file.write('\n')
        finally:
            __current_method_name.pop()
        return result
    return decorated

def trace_method(method):
    return trace_method_options(method)

def trace_method_args(method):
    return trace_method_options(method, with_args=True)

def bump_trace():
    __current_method_name.top()[1] += 1
    _indent()
    _output_file.write('%s.%s\n' % tuple(__current_method_name.top()))

def report_stack(method):
    def decorated(self, *args, **kwargs):
        print "-" * 78
        try:
            print "Method: " + method.im_class.__name__ + '.' + method.__name__
        except:
            pass
        try:
            print "Function: " + method.func_name
        except:
            pass
        traceback.print_stack()
        print "-" * 78
        return method(self, *args, **kwargs)
    return decorated
        
###############################################################################

import unittest
import tempfile
import os

@trace_method
def test_fun(p1):
    return p1 + 5

@trace_method
def test_fun_2(p1):
    bump_trace()
    result = test_fun(p1) + 3
    bump_trace()
    return result

class TestTraceMethod(unittest.TestCase):

    def test_trace_1(self):
        global _output_file
        (fd, name) = tempfile.mkstemp()
        os.close(fd)
        _output_file = file(name, 'w')

        x = test_fun(10)
        self.assertEquals(x, 15)
        
        _output_file.close()
        _output_file = sys.stderr

        output = "".join(file(name, 'r').readlines())
        self.assertEquals(output,
                          'test_fun.enter\n' +
                          'test_fun.exit\n')
        os.unlink(name)

    def test_trace_2(self):
        global _output_file
        (fd, name) = tempfile.mkstemp()
        os.close(fd)
        _output_file = file(name, 'w')

        x = test_fun_2(10)
        self.assertEquals(x, 18)
        
        _output_file.close()
        _output_file = sys.stderr

        output = "".join(file(name, 'r').readlines())
        self.assertEquals(output,
                          'test_fun_2.enter\n' +
                          'test_fun_2.1\n' +
                          ' test_fun.enter\n' +
                          ' test_fun.exit\n' +
                          'test_fun_2.2\n' +
                          'test_fun_2.exit\n')
        os.unlink(name)

if __name__ == '__main__':
    unittest.main()
