
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

import copy
from db.versions.v0_9_3.domain import DBVistrail, DBAction, DBTag, DBModule, \
    DBConnection, DBPortSpec, DBFunction, DBParameter, DBLocation, DBAdd, \
    DBChange, DBDelete, DBAnnotation, DBPort, DBAbstractionRef, DBGroup

def translateVistrail(_vistrail):
    vistrail = DBVistrail()

    for _action in _vistrail.db_actions:
        ops = []
        for op in _action.db_operations:
            if op.vtType == 'add':
                data = convert_data(op.db_data)
                ops.append(DBAdd(id=op.db_id,
                                 what=op.db_what,
                                 objectId=op.db_objectId,
                                 parentObjId=op.db_parentObjId,
                                 parentObjType=op.db_parentObjType,
                                 data=data))
            elif op.vtType == 'change':
                data = convert_data(op.db_data)
                ops.append(DBChange(id=op.db_id,
                                    what=op.db_what,
                                    oldObjId=op.db_oldObjId,
                                    newObjId=op.db_newObjId,
                                    parentObjId=op.db_parentObjId,
                                    parentObjType=op.db_parentObjType,
                                    data=data))
            elif op.vtType == 'delete':
                ops.append(DBDelete(id=op.db_id,
                                    what=op.db_what,
                                    objectId=op.db_objectId,
                                    parentObjId=op.db_parentObjId,
                                    parentObjType=op.db_parentObjType))
        annotations = []
        for annotation in _action.db_annotations:
            key = annotation.db_key
            if key == 'notes':
                key = '__notes__'
            annotations.append(DBAnnotation(id=annotation.db_id,
                                            key=key,
                                            value=annotation.db_value))
        session = _action.db_session
        if not session:
            session = None
        else:
            session = _action.db_session

        action = DBAction(id=_action.db_id,
                          prevId=_action.db_prevId,
                          date=_action.db_date,
                          user=_action.db_user,
                          prune=_action.db_prune,
                          session=session,
                          operations=ops,
                          annotations=annotations)
        vistrail.db_add_action(action)

    for _tag in _vistrail.db_tags:
        tag = DBTag(id=_tag.db_id,
                    name=_tag.db_name)
        vistrail.db_add_tag(tag)

    vistrail.db_version = '0.9.3'
    return vistrail

def convert_data(child):
    if child.vtType == 'module':
        return DBModule(id=child.db_id,
                        cache=child.db_cache,
                        name=child.db_name,
                        namespace=child.db_namespace,
                        package=child.db_package,
                        version=child.db_version,
                        tag=child.db_tag)
    elif child.vtType == 'abstractionRef':
        return DBAbstractionRef(id=child.db_id,
                                name=child.db_name,
                                cache=child.db_cache,
                                abstraction_id=child.db_abstraction_id,
                                version=child.db_version)
    elif child.vtType == 'connection':
        return DBConnection(id=child.db_id)
    elif child.vtType == 'portSpec':
        return DBPortSpec(id=child.db_id,
                          name=child.db_name,
                          type=child.db_type,
                          spec=child.db_spec)
    elif child.vtType == 'function':
        return DBFunction(id=child.db_id,
                          pos=child.db_pos,
                          name=child.db_name)
    elif child.vtType == 'parameter':
        return DBParameter(id=child.db_id,
                           pos=child.db_pos,
                           name=child.db_name,
                           type=child.db_type,
                           val=child.db_val,
                           alias=child.db_alias)
    elif child.vtType == 'location':
        return DBLocation(id=child.db_id,
                          x=child.db_x,
                          y=child.db_y)
    elif child.vtType == 'annotation':
        return DBAnnotation(id=child.db_id,
                            key=child.db_key,
                            value=child.db_value)
    elif child.vtType == 'port':
        return DBPort(id=child.db_id,
                      type=child.db_type,
                      moduleId=child.db_moduleId,
                      moduleName=child.db_moduleName,
                      name=child.db_name,
                      spec=child.db_spec)
    elif child.vtType == 'group':
        return DBGroup(id=child.db_id,
                       workflow=child.db_workflow,
                       cache=child.db_cache,
                       name=child.db_name,
                       namespace=child.db_namespace,
                       package=child.db_package,
                       version=child.db_version,
                       tag=child.db_tag)
                       
