
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

import core.interpreter.cached
import core.interpreter.noncached

cached_interpreter = core.interpreter.cached.CachedInterpreter
noncached_interpreter = core.interpreter.noncached.Interpreter
__default_interpreter = cached_interpreter

from PyQt4 import QtCore

##############################################################################

def set_cache_configuration(field, value):
    assert field == 'useCache'
    if value:
        set_default_interpreter(cached_interpreter)
    else:
        set_default_interpreter(noncached_interpreter)

def connect_to_configuration(configuration):
    configuration.subscribe('useCache', set_cache_configuration)

def get_default_interpreter():
    """Returns an instance of the default interpreter class."""
    return __default_interpreter.get()

def set_default_interpreter(interpreter_class):
    """Sets the default interpreter class."""
    global __default_interpreter
    __default_interpreter = interpreter_class

##############################################################################

import unittest

class TestDefaultInterpreter(unittest.TestCase):

    def test_set(self):
        old_interpreter = type(get_default_interpreter())
        try:
            set_default_interpreter(noncached_interpreter)
            self.assertEquals(type(get_default_interpreter()),
                              noncached_interpreter)
            set_default_interpreter(cached_interpreter)
            self.assertEquals(type(get_default_interpreter()),
                              cached_interpreter)
        finally:
            set_default_interpreter(old_interpreter)
            self.assertEquals(type(get_default_interpreter()),
                              old_interpreter)
