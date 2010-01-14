"""
async_io_helper module

example usage:

from __future__ import generators
import async_io_helper

helper = async_io_helper.new_helper()

def get_helper():
    return helper

@async_io_helper.wrap(get_helper)
def count(io_helper=None):
    if not io_helper: return
    count = 1
    while 1:
        print "Current count:", count
        yield "Press 'c' to increase the count or hit 'q' to quit: "
        if io_helper.get_input() == 'c': count += 1
        if io_helper.get_input() == 'q': return

count()
print helper.get_output()

helper.handle_input('c')
print helper.get_output()

helper.handle_input('q')

"""

def new_helper():
    return _helper()

def wrap(get_io_helper_func):
    """A decorator that takes one argument.  The argument should be an instance
    of the helper class returned by new_helper().  This decorator wraps a method
    so that is may perform asynchronous IO using the helper instance.  The
    method being wrapped should take a keyword argument 'io_helper' which will
    be set to the helper instance passed in."""
    def decorator_factory(func):
        def func_wrapper(*args, **kwargs):
            if not "io_helper" in kwargs:
                kwargs["io_helper"] = get_io_helper_func()
            helper = kwargs["io_helper"]
            helper._generator = func(*args, **kwargs)
            helper._next()
        func_wrapper.__doc__ = func.__doc__
        func_wrapper.__name__ = func.__name__
        return func_wrapper

    return decorator_factory


class _helper(object):

    def __init__(self):
        self._clear()

    def get_input(self):
        return self._input_line

    def get_output(self):
        return self._output

    def handle_input(self, line):
        """Handle an input line.  Returns false if the input is not used
        because there is no object waiting on asynchronous input.  Returns
        true if the input line is fed to an object that was waiting for
        asynchronous input."""
        if not self._generator: return False
        if not isinstance(line, str):
            raise ValueError("Expected string argument")
        self._input_line = line
        self._next()
        return True

    def _next(self):
        try: self._output = self._generator.next()
        except StopIteration: self._clear()
        except Exception, err:

            # Uncomment for debugging.
            #import sys
            #import traceback
            #traceback.print_exc(file=sys.stdout)

            self._clear()
            raise(err)

    def _clear(self):
        self._input_line = str()
        self._generator = None
        self._output = None



#############################################################################
#
# The following code is not needed for this module's main functionality.
# You can call start_prompt(helper, locals()) to an interpreter that mimics
# the standard python prompt but allows you to call asynchronous IO methods
# that use the helper passed to start_prompt.  You can use this for testing
# or from a gui with embeded python.
#

import cmd
import code

def start_prompt(io_helper, locals_dict=None):
    p = _prompt(io_helper, locals_dict)
    p.cmdloop()

class _prompt(cmd.Cmd):
    def __init__(self, io_helper, locals_dict):
        cmd.Cmd.__init__(self)
        self.prompt = ">>> "
        self._interp = code.InteractiveConsole(locals_dict)
        self._ioHelper = io_helper
    def _exitcmd(self, s):
        if s == "EOF": print
        return s == "exit()" or s == "EOF"
    def _prepare_cmd(self, s):
        return s.rstrip().replace("\r\n", "\n").replace("\r", "\n") + "\n"
    def onecmd(self, s):
        prompt = ">>> "
        if not self._ioHelper.handle_input(s):
            if self._exitcmd(s): return True
            if self._interp.push(self._prepare_cmd(s)): prompt = "... "
        if self._ioHelper.get_output():
            prompt = self._ioHelper.get_output()
        self.prompt = prompt
        return False

