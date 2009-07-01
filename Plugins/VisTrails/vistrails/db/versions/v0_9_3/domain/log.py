
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

from auto_gen import DBLog as _DBLog
from auto_gen import DBAbstractionRef, DBModule
from id_scope import IdScope

import copy

class DBLog(_DBLog):

    def __init__(self, *args, **kwargs):
  _DBLog.__init__(self, *args, **kwargs)
        self.id_scope = IdScope(1,
                              {DBAbstractionRef.vtType: DBModule.vtType})

    def __copy__(self):
        return DBLog.do_copy(self)

    def do_copy(self, new_ids=False, id_scope=None, id_remap=None):
        cp = _DBLog.do_copy(self, new_ids, id_scope, id_remap)
        cp.__class__ = DBLog
        cp.id_scope = copy.copy(self.id_scope)
