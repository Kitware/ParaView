
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
"""Defines a time decorator that times function calls."""

import time

def time_method(method):
    def decorated(self, *args, **kwargs):
        t = time.time()
        try:
            return method(self, *args, **kwargs)
        finally:
            print time.time() - t
    return decorated

def time_call(callable_):
    def decorated(*args, **kwargs):
        t = time.time()
        try:
            return callable_(*args, **kwargs)
        finally:
            print time.time() - t
    return decorated

##############################################################################
