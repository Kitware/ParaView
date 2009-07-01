
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
## Copyright (C) 2008, 2009 VisTrails, Inc. All rights reserved.
##
############################################################################

##############################################################################

def A_get_snapshot(self):
    if self.db_has_annotation_with_key(self.ANNOTATION_SNAPSHOT):
        return \
            int(self.db_get_annotation_by_key(self.ANNOTATION_SNAPSHOT).value)
    return 0

import core.vistrail
core.vistrail.action.Action._get_snapshot = A_get_snapshot
core.vistrail.action.Action.ANNOTATION_SNAPSHOT = '__snapshot__'
core.vistrail.action.Action.snapshot = property(core.vistrail.action.Action._get_snapshot)
