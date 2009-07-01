
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
"""
This module defines common functions and exception class definitions
used all over VisTrails.
"""
from core.utils.enum import enum
from core.utils.timemethod import time_method, time_call
from core.utils.tracemethod import trace_method, bump_trace, report_stack, \
     trace_method_options, trace_method_args
from core.utils.color import ColorByName
from core.utils.lockmethod import lock_method
import errno
import itertools

################################################################################

def invert(d):
    """invert(dict) -> dict. Returns an inverted dictionary by
    switching key-value pairs. If you use this repeatedly,
    consider switching the underlying data structure to a
    core.data_structures.bijectivedict.Bidict instead."""
    return dict([[v,k] for k,v in d.items()])

################################################################################

def unimplemented():
    """Raises UnimplementedException."""
    raise UnimplementedException()

def abstract():
    """Raises AbstractException.""" 
    raise AbstractException()

################################################################################

class NoMakeConnection(Exception):
    """NoMakeConnection is raised when a VisConnection doesn't know
    how to create a live version of itself. This is an internal error
    that should never be seen by a user. Please report a bug if you
    see this."""
    def __init__(self, conn):
        self.conn = conn
    def __str__(self):
        return "Connection %s has no makeConnection method" % self.conn

class NoSummon(Exception):
    """NoSummon is raised when a VisObject doesn't know how to create
    a live version of itself. This is an internal error that should
    never be seen by a user. Please report a bug if you see this."""
    def __init__(self, obj):
        self.obj = obj
    def __str__(self):
        return "Module %s has no summon method" % self.obj

class UnimplementedException(Exception):
    """UnimplementedException is raised when some interface hasn't
    been implemented yet. This is an internal error that should never
    be seen by a user. Please report a bug if you see this."""
    def __str__(self):
  return "Object is Unimplemented"

class AbstractException(Exception):
    """AbstractException is raised when an abstract method is called.
    This is an internal error that should never be seen by a
    user. Please report a bug if you see this."""
    def __str__(self):
  return "Abstract Method was called"

class VistrailsInternalError(Exception):
    """VistrailsInternalError is raised when an unexpected internal
    inconsistency happens. This is (clearly) an internal error that
    should never be seen by a user. Please report a bug if you see
    this."""
    def __init__(self, msg):
        self.emsg = msg
    def __str__(self):
        return "Vistrails Internal Error: " + str(self.emsg)

class VersionTooLow(Exception):
    """VersionTooLow is raised when you're running an outdated version
    of some necessary software or package."""
    def __init__(self, sw, required_version):
        self.sw = sw
        self.required_version = required_version
    def __str__(self):
  return ("Your version of '" +
                sw +
                "' is too low. Please upgrade to " +
                required_version +
                " or later")

class InvalidModuleClass(Exception):
    """InvalidModuleClass is raised when there's something wrong with
a class that's being registered as a module within VisTrails."""

    def __init__(self, klass):
        self.klass = klass

    def __str__(self):
        return ("class '%s' cannot be registered in VisTrails. Please" +
                " consult the documentation.") % self.klass.__name__

class ModuleAlreadyExists(Exception):
    """ModuleAlreadyExists is raised when trying to add a class that
    is already in the module registry."""

    def __init__(self, identifier, moduleName):
        self._identifier = identifier
        self._name = moduleName

    def __str__(self):
        return ("'%s, %s' cannot be registered in VisTrails because of another "
                "module with the same identifier and name already exists." %
                (self._identifier,
                 self._name))

################################################################################

# Only works for functions with NO kwargs!
def memo_method(method):
    """memo_method is a method decorator that memoizes results of the
    decorated method, trading off memory for time by caching previous
    results of the calls."""
    attrname = "_%s_memo_result" % id(method)
    memo = {}
    def decorated(self, *args):
        try:
            return memo[args]
        except KeyError:
            result = method(self, *args)
            memo[args] = result
            return result
    warn = "(This is a memoized method: Don't mutate the return value you're given.)"
    if method.__doc__:
        decorated.__doc__ = method.__doc__ + "\n\n" + warn
    else:
        decorated.__doc__ = warn
    return decorated

##############################################################################
# Profiling, utilities

_profiled_list = []
def profile(func):
    """profile is a method decorator that profiles the calls of a
    given method using cProfile. You need to get the decorated method
    programmatically later to get to the profiler stats. It will be
    available as the attribute 'profiler_object' on the decorated
    result.

    From there, you can simply call save_all_profiles(), and that will
    take the list of all profiled methods and save them to different
    files.

    If you like manual labor, you probably want to do something like this:

    >>> po = ...... .profiler_object
    >>> po.dump_stats('/tmp/some_temporary_file')
    >>> import pstats
    >>> ps = pstats.Stats('/tmp/some_temporary_file')
    >>> ps.sort_stats('time') # or cumtime, or calls, or others - see doc
    >>> ps.print_stats()

    """

    # Notice that on ubuntu you will need
    # sudo apt-get install python-profiler

    try:
        import cProfile as prof
    except ImportError:
        import profile as prof

    pobject = prof.Profile()
    def method(*args, **kwargs):
        return pobject.runcall(func, *args, **kwargs)

    method.profiler_object = pobject
    _profiled_list.append((func.__name__, method))
    return method

def get_profiled_methods():
    return _profiled_list

def save_profile_to_disk(callable_, filename):
    callable_.profiler_object.dump_stats(filename)

def save_all_profiles():
    # This is internal because core.system imports core.utils... :/
    import core.system
    td = core.system.temporary_directory()
    for (name, method) in get_profiled_methods():
        fout = td + name + '.pyp'
        print fout
        method.profiler_object.dump_stats(fout)

##############################################################################

def debug(func):
    """debug is a method decorator that invokes the python integrated
    debugger in a given method. Use it to step through tricky
    code. Note that pdb is not integrated with emacs or anything like
    that, so you'll need a shell to see what's going on.

    """
    import pdb

    def method(*args, **kwargs):
        return pdb.runcall(func, *args, **kwargs)
    return method

################################################################################

def all(bool_list, pred = lambda x: x):
    """all(list, [pred]) -> Boolean - Returns true if all elements are
    true.  If pred is given, it is applied to the list elements first"""
    for b in bool_list:
        if not pred(b):
            return False
    return True

def any(bool_list, pred = lambda x: x):
    """any(bool_list, [pred]) -> Boolean - Returns true if any element
    is true.  If pred is given, it is applied to the list elements
    first"""
    for b in bool_list:
        if pred(b):
            return True
    return False

def iter_index(iterable, item):
    """iter_index(iterable, item) -> int - Iterates through iterator
    until item is found, and returns the index inside the iterator.

    iter_index is analogous to list.index for iterators."""
    try:
        itor = itertools.izip(iterable, itertools.count(0))
        return itertools.dropwhile(lambda (v,c): v != item, itor).next()[1]
    except StopIteration:
        return -1
                                              

def eprint(*args):
    """eprint(*args) -> False - Prints the arguments, then returns
    false. Useful inside a lambda expression, for example."""
    for v in args:
        print v,
    print

def uniq(l):
    """uniq(l) -> List. Returns a new list consisting of elements that
    test pairwise different for equality. Requires all elements to be
    sortable, and runs in O(n log n) time."""
    if len(l) == 0:
        return []
    a = copy.copy(l)
    a.sort()
    l1 = a[:-1] 
    l2 = a[1:]
    return [a[0]] + [next for (i, next) in itertools.izip(l1, l2) if i != next]

class InstanceObject(object):
    """InstanceObject is a convenience class created to facilitate
    creating of one-off objects with many fields. It simply translates
    the passed kwargs on the constructor to a set of fields with
    the right values."""
    def __init__(self, **kw):
        self.__dict__.update(kw)

    def __str__(self):
        pre = "(%s " % self.__class__.__name__
        items = [('%s: %s' % (k, str(v)))
                 for (k, v)
                 in sorted(self.__dict__.items())]
        items_str = ('\n' + (' ' * len(pre))).join(items)
        post = ')@%X' % id(self)
        return pre + items_str + post
    
    def write_source(self, prefix=""):
        result = ""
        for (k, v) in sorted(self.__dict__.items()):
            if isinstance(v, InstanceObject):
                newprefix = prefix + "." + k
                result += v.write_source(newprefix)
            else:
                result += prefix
                result += "." + str(k) + " = " 
                if type(v) == type('string'):
                    result +=  "'" + str(v) + "'\n"
                else:
                    result += str(v) + "\n"
        return result

def append_to_dict_of_lists(dict, key, value):
    """Appends /value/ to /dict/[/key/], or creates entry such that
    /dict/[/key/] == [/value/]."""
    try:
        dict[key].append(value)
    except KeyError:
        dict[key] = [value]

def version_string_to_list(version):
    """version_string_to_list converts a version string to a list of
    numbers and strings:

    version_string('0.1') -> [0, 1]
    version_string('0.9.9alpha') -> [0, 9, '9alpha']

    """
    def convert(value):
        try:
            return int(value)
        except ValueError:
            return value
    return [convert(value) for value in version.split('.')]

##############################################################################
# DummyView

class DummyView(object):
    def set_module_active(*args, **kwargs): pass
    def set_module_computing(*args, **kwargs): pass
    def set_module_success(*args, **kwargs): pass
    def set_module_error(*args, **kwargs): pass
    def set_module_not_executed(*args, **kwargs): pass

##############################################################################    
# FIXME: Add tests
def no_interrupt(callable_, *args, **kwargs):
    """no_interrupt(callable_, *args, **kwargs) -> return arguments
    from callable.

    Calls callable_ with *args and **kwargs and keeps retrying as long as call
is interrupted by the OS. This makes calling read more convenient when
using output from the subprocess module."""
    while True:
        try:
            return callable_(*args, **kwargs)
        except IOError, e:
            if e.errno == errno.EINTR:
                continue
            else:
                raise

################################################################################

import unittest

class _TestFibo(object):
    @memo_method
    def f(self, x):
        if x == 0: return 0
        if x == 1: return 1
        return self.f(x-1) + self.f(x-2)

class TestCommon(unittest.TestCase):
    def test_append_to_dict_of_lists(self):
        f = {}
        self.assertEquals(f.has_key(1), False)
        append_to_dict_of_lists(f, 1, 1)
        self.assertEquals(f.has_key(1), True)
        self.assertEquals(f[1], [1])
        append_to_dict_of_lists(f, 1, 1)
        self.assertEquals(f.has_key(1), True)
        self.assertEquals(f[1], [1, 1])
        append_to_dict_of_lists(f, 1, 2)
        self.assertEquals(f.has_key(1), True)
        self.assertEquals(f[1], [1, 1, 2])
        append_to_dict_of_lists(f, 2, "Foo")
        self.assertEquals(f.has_key(2), True)
        self.assertEquals(f[2], ["Foo"])
        
    def test_memo(self):
        import time
        t1 = time.time()
        for i in xrange(10000):
            _TestFibo().f(102)
        t2 = time.time()
        for i in xrange(10000):
            _TestFibo().f(104)
        t3 = time.time()
        for i in xrange(10000):
            _TestFibo().f(106)
        t4 = time.time()
        d1 = t2 - t1
        d2 = t3 - t2
        d3 = t4 - t3
        if d1 == 0: r1 = 0
        else: r1 = d2 / d1
        if d2 == 0: r2 = 0
        else: r2 = d3 / d2
        self.assertEquals(r1 < 2.618, True)
        self.assertEquals(r2 < 2.618, True)

    def test_memo_2(self):
        count = [0]
        class C1(object):
            pass
        class C2(object):
            pass
        class TestClassMemo(object):
            def __init__(self, cell):
                self.cell = cell
            @memo_method
            def f(self, cl, x):
                self.cell[0] += 1
                return x
        t = TestClassMemo(count)
        self.assertEquals(count[0], 0)
        t.f(C1, 0)
        self.assertEquals(count[0], 1)
        t.f(C1, 0)
        self.assertEquals(count[0], 1)
        t.f(C1, 1)
        self.assertEquals(count[0], 2)
        t.f(C2, 0)
        self.assertEquals(count[0], 3)

    def test_version_string_to_list(self):
        self.assertEquals(version_string_to_list("0.1"), [0, 1])
        self.assertEquals(version_string_to_list("1.0.2"), [1, 0, 2])
        self.assertEquals(version_string_to_list("1.0.2beta"), [1, 0, '2beta'])
        
if __name__ == '__main__':
    unittest.main()
