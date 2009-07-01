
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

from core.vistrail.annotation import Annotation
from db.domain import DBModuleExec

class ModuleExec(DBModuleExec):
    """ Class that stores info for logging a module execution. """

    def __init__(self, *args, **kwargs):
        DBModuleExec.__init__(self, *args, **kwargs)

    def __copy__(self):
        return self.do_copy()

    def do_copy(self):
        cp = DBModuleExec.__copy__(self)
        cp.__class__ = ModuleExec
        return cp

    @staticmethod
    def convert(_module_exec):
        if _module_exec.__class__ == ModuleExec:
            return
        _module_exec.__class__ = ModuleExec
        for annotation in _module_exec.annotations:
            Annotation.convert(annotation)

    ##########################################################################
    # Properties

    id = DBModuleExec.db_id
    ts_start = DBModuleExec.db_ts_start
    ts_end = DBModuleExec.db_ts_end
    cached = DBModuleExec.db_cached
    completed = DBModuleExec.db_completed
    abstraction_id = DBModuleExec.db_abstraction_id
    abstraction_version = DBModuleExec.db_abstraction_version
    module_id = DBModuleExec.db_module_id
    module_name = DBModuleExec.db_module_name
    machine_id = DBModuleExec.db_machine_id
    error = DBModuleExec.db_error

    def _get_duration(self):
        if self.db_ts_end is not None:
            return self.db_ts_end - self.db_ts_start
        return None
    duration = property(_get_duration)

    def _get_annotations(self):
        return self.db_annotations
    def _set_annotations(self, annotations):
        self.db_annotations = annotations
    annotations = property(_get_annotations, _set_annotations)
    def add_annotation(self, annotation):
        self.db_add_annotation(annotation)

    
