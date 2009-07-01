
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

from db import VistrailsDBException
from db.versions.v0_8_0.domain import DBAdd, DBAnnotation, DBChange, DBDelete

# two step process
# 1. remap all the old "notes" so that they exist in the id scope
# 2. remap all the annotations that were numbered correctly
# note that for 2, we don't need to worry about uniqueness -- they are unique
# but step 1 may have taken some of their ids...

def translateVistrail(vistrail):
    id_remap = {}
    for action in vistrail.db_get_actions():
        # don't need to change key idx since none of that changes
        new_action_idx = {}
        for annotation in action.db_get_annotations():
            annotation.db_id = vistrail.idScope.getNewId(DBAnnotation.vtType)
            new_action_idx[annotation.db_id] = annotation
        action.db_annotations_id_index = new_action_idx

        for operation in action.db_get_operations():
            # never have annotations as parent objs so 
            # don't have to worry about those ids
            if operation.db_what == DBAnnotation.vtType:
                if operation.vtType == 'add':
                    new_id = vistrail.idScope.getNewId(DBAnnotation.vtType)
                    old_id = operation.db_objectId
                    operation.db_objectId = new_id
                    operation.db_data.db_id = new_id
                    id_remap[old_id] = new_id
                elif operation.vtType == 'change':
                    changed_id = operation.db_oldObjId
                    if id_remap.has_key(changed_id):
                        operation.db_oldObjId = id_remap[changed_id]
                    else:
                        raise VistrailsDBException('cannot translate')

                    new_id = vistrail.idScope.getNewId(DBAnnotation.vtType)
                    old_id = operation.db_newObjId
                    operation.db_newObjId = new_id
                    operation.db_data.db_id = new_id
                    id_remap[old_id] = new_id
                elif operation.vtType == 'delete':
                    old_id = operation.db_objectId
                    if id_remap.has_key(old_id):
                        operation.db_objectId = id_remap[old_id]
                    else:
                        raise VistrailsDBException('cannot translate')

    vistrail.db_version = '0.8.1'
    return vistrail
