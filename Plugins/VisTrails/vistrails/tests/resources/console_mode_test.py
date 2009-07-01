
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

"""Testing package for console_mode"""

##############################################################################

import core.modules
import core.modules.module_registry
from core.modules.vistrails_module import Module, ModuleError, NotCacheable
from core.modules.basic_modules import Float, Integer

identifier = 'edu.utah.sci.vistrails.console_mode_test'
version = '0.9.0'
name = 'Console Mode Tests'

class TestTupleExecution(Module):

    def compute(self):
        v1, v2 = self.getInputFromPort('input')
        self.setResult('output', v1 + v2)


class TestDynamicModuleError(Module):

    def compute(self):
        c = TestDynamicModuleError()
        c.die()

    def die(self):
        raise ModuleError(self, "I died!")

class TestChangeVistrail(NotCacheable, Module):

    def compute(self):
        if self.hasInputFromPort('foo'):
            v1 = self.getInputFromPort('foo')
        else:
            v1 = 0
        if v1 != 12:
            self.change_parameter('foo', v1 + 1)

class TestCustomNamed(Module):

    pass


class TestOptionalPorts(Module):

    pass

##############################################################################

def initialize():
    reg = core.modules.module_registry
    reg.add_module(TestTupleExecution)
    reg.add_input_port(TestTupleExecution, 'input', [Float, Float])
    reg.add_output_port(TestTupleExecution, 'output', (Float, 'output'))
    reg.add_module(TestDynamicModuleError)
    reg.add_module(TestChangeVistrail)
    reg.add_input_port(TestChangeVistrail, 'foo', Integer)

    reg.add_module(TestCustomNamed, name='different name')
    reg.add_input_port(TestCustomNamed, 'input', Float)

    reg.add_module(TestOptionalPorts)
    reg.add_input_port(TestOptionalPorts, 'foo', Float, optional=True)
    reg.add_output_port(TestOptionalPorts, 'foo', Float, optional=True)

