
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

from core.log.module_exec import ModuleExec
from db.domain import DBWorkflowExec

class WorkflowExec(DBWorkflowExec):
    """ Class that stores info for logging a workflow execution. """

    def __init__(self, *args, **kwargs):
        DBWorkflowExec.__init__(self, *args, **kwargs)

    def __copy__(self):
        return self.do_copy()

    def do_copy(self):
        cp = DBWorkflowExec.__copy__(self)
        cp.__class__ = WorkflowExec
        return cp

    @staticmethod
    def convert(_wf_exec):
        if _wf_exec.__class__ == WorkflowExec:
            return
        _wf_exec.__class__ = WorkflowExec
        for module_exec in _wf_exec.module_execs:
            ModuleExec.convert(module_exec)
            

    ##########################################################################
    # Properties

    id = DBWorkflowExec.db_id
    user = DBWorkflowExec.db_user
    ip = DBWorkflowExec.db_ip
    session = DBWorkflowExec.db_session
    vt_version = DBWorkflowExec.db_vt_version
    ts_start = DBWorkflowExec.db_ts_start
    ts_end = DBWorkflowExec.db_ts_end
    parent_type = DBWorkflowExec.db_parent_type
    parent_id = DBWorkflowExec.db_parent_id
    parent_version = DBWorkflowExec.db_parent_version
    name = DBWorkflowExec.db_name
    completed = DBWorkflowExec.db_completed

    def _get_duration(self):
        if self.db_ts_end is not None:
            return self.db_ts_end - self.db_ts_start
        return None
    duration = property(_get_duration)

    def _get_module_execs(self):
        return self.db_module_execs
    module_execs = property(_get_module_execs)
    def add_module_exec(self, module_exec):
        self.db_add_module_exec(module_exec)
