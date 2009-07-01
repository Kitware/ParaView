
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
""" Module used when running  vistrails uninteractively """
import core.interpreter.default
from core.utils import (VistrailsInternalError, expression,
                        DummyView)
from core.db.locator import XMLFileLocator
from core.vistrail.vistrail import Vistrail

################################################################################
    
def run_and_get_results(w_list, parameters=''):
    """run_and_get_results(w_list: list of (locator, version), parameters: str)
    Run all workflows in w_list, and returns an interpreter result object.
    version can be a tag name or a version id.
    
    """
    elements = parameters.split(",")
    aliases = {}
    result = []
    for locator, workflow in w_list:
        v = locator.load()
        if type(workflow) == type("str"):
            version = v.get_tag_by_name(workflow).time
        elif type(workflow) == type(1):
            version = workflow
        else:
            msg = "Invalid version tag or number: %s" % workflow
            raise VistrailsInternalError(msg)

        pip = v.getPipeline(workflow)
        for e in elements:
            pos = e.find("=")
            if pos != -1:
                key = e[:pos].strip()
                value = e[pos+1:].strip()
            
                if pip.has_alias(key):
                    (vttype, pId,_,_) = pip.aliases[key]
                    parameter = pip.db_get_object(vttype,pId)
                    ptype = parameter.type
                    aliases[key] = (ptype,expression.parse_expression(value))
        view = DummyView()
        interpreter = core.interpreter.default.get_default_interpreter()

        run = interpreter.execute(None, pip, locator, version, view, aliases)
        result.append(run)
    return result
    
def run(w_list, parameters=''):
    """run(w_list: list of (locator, version), parameters: str) -> boolean
    Run all workflows in w_list, version can be a tag name or a version id.
    Returns False in case of error. 
    """
    results = run_and_get_results(w_list, parameters)
    for result in results:
        (objs, errors, executed) = (result.objects,
                                    result.errors, result.executed)
        for i in objs.iterkeys():
            if errors.has_key(i):
                return False
    return True

def cleanup():
    core.interpreter.cached.CachedInterpreter.cleanup()

################################################################################
#Testing

import core.packagemanager
import core.system
import sys
import unittest
import core.vistrail
import random
from core.vistrail.module import Module

class TestConsoleMode(unittest.TestCase):

    def setUp(self, *args, **kwargs):
        manager = core.packagemanager.get_package_manager()
        if manager.has_package('edu.utah.sci.vistrails.console_mode_test'):
            return

        old_path = sys.path
        sys.path.append(core.system.vistrails_root_directory() +
                        '/tests/resources')
        m = __import__('console_mode_test')
        sys.path = old_path
        d = {'console_mode_test': m}
        manager.add_package('console_mode_test')
        manager.initialize_packages(d)

    def test1(self):
        locator = XMLFileLocator(core.system.vistrails_root_directory() +
                                 '/tests/resources/dummy.xml')
        result = run([(locator, "int chain")])
        self.assertEquals(result, True)

    def test_tuple(self):
        from core.vistrail.module_param import ModuleParam
        from core.vistrail.module_function import ModuleFunction
        from core.vistrail.module import Module
        interpreter = core.interpreter.default.get_default_interpreter()
        v = DummyView()
        p = core.vistrail.pipeline.Pipeline()
        params = [ModuleParam(type='Float',
                              val='2.0',
                              ),
                  ModuleParam(type='Float',
                              val='2.0',
                              )]
        p.add_module(Module(id=0,
                           name='TestTupleExecution',
                           package='edu.utah.sci.vistrails.console_mode_test',
                           functions=[ModuleFunction(name='input',
                                                     parameters=params)],
                           ))
        interpreter.execute(None, p, 'foo', 1, v, None)

    def test_python_source(self):
        locator = XMLFileLocator(core.system.vistrails_root_directory() +
                                 '/tests/resources/pythonsource.xml')
        result = run([(locator,"testPortsAndFail")])
        self.assertEquals(result, True)

    def test_python_source_2(self):
        locator = XMLFileLocator(core.system.vistrails_root_directory() +
                                 '/tests/resources/pythonsource.xml')
        result = run_and_get_results([(locator, "test_simple_success")])[0]
        self.assertEquals(len(result.executed), 1)

    def test_dynamic_module_error(self):
        locator = XMLFileLocator(core.system.vistrails_root_directory() + 
                                 '/tests/resources/dynamic_module_error.xml')
        result = run([(locator, "test")])
        self.assertEquals(result, False)

    def test_change_parameter(self):
        locator = XMLFileLocator(core.system.vistrails_root_directory() + 
                                 '/tests/resources/test_change_vistrail.xml')
        result = run([(locator, "v1")])
        self.assertEquals(result, True)

        result = run([(locator, "v2")])
        self.assertEquals(result, True)

    def test_ticket_73(self):
        # Tests serializing a custom-named module to disk
        locator = XMLFileLocator(core.system.vistrails_root_directory() + 
                                 '/tests/resources/test_ticket_73.xml')
        v = locator.load()

        import tempfile
        import os
        (fd, filename) = tempfile.mkstemp()
        os.close(fd)
        locator = XMLFileLocator(filename)
        try:
            locator.save(v)
        finally:
            os.remove(filename)

if __name__ == '__main__':
    unittest.main()
