
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

from core import modules
from core.common import *
from core.data_structures.bijectivedict import Bidict
from core.modules.module_utils import FilePool
from core.modules.vistrails_module import ModuleConnector, ModuleError
from core.utils import DummyView
import copy
import core.interpreter.base
import core.interpreter.utils
import core.vistrail.pipeline
import atexit

################################################################################

class Interpreter(core.interpreter.cached.CachedInterpreter):

    def clean_non_cacheable_modules(self):
        non_cacheable_modules = [i for
                                 (i, mod) in self._objects.iteritems()]
        self.clean_modules(non_cacheable_modules)

    __instance = None
    @staticmethod
    def get():
        if not Interpreter.__instance:
            instance = Interpreter()
            Interpreter.__instance = instance
            def cleanup():
                instance._file_pool.cleanup()
            atexit.register(cleanup)
        return Interpreter.__instance
        

################################################################################
