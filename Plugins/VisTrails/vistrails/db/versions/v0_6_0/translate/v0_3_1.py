
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

import sys
import copy
from datetime import datetime
from time import strptime

from core.data_structures.graph import Graph
from db.versions.v0_6_0.domain import DBVistrail, DBAction, DBTag, DBModule, \
    DBConnection, DBPortSpec, DBFunction, DBParameter, DBLocation, DBAdd, \
    DBChange, DBDelete, DBAnnotation, DBPort

def convertDate(date):
    if date is not None and date != '':
        return datetime(*strptime(date, '%d %b %Y %H:%M:%S')[0:6])
    return datetime(1900, 1, 1)

def translateVistrail(_vistrail):
    vistrail = DBVistrail()
    id_scope = vistrail.idScope
    for _action in _vistrail.db_get_actions():
#         print 'translating action %s' % _action.db_time
        functionName = 'translate%s%sAction' % \
            (_action.db_what[0].upper(), _action.db_what[1:])
        thisModule = sys.modules[__name__]
        action = getattr(thisModule, functionName)(_action, id_scope)
        vistrail.db_add_action(action)
    for _tag in _vistrail.db_get_tags():
        tag = DBTag(id=_tag.db_time,
                    name=_tag.db_name)
        vistrail.db_add_tag(tag)

    convertIds(vistrail)
#     for action in vistrail.getActions():
#         print '%s %s' % (action.id, action.operations)
    vistrail.db_version = '0.6.0'
    return vistrail

def createAction(_action, operations, id_scope):
    annotations = {}
    if _action.db_notes is not None and _action.db_notes.strip() != '':
        annotations['notes'] = DBAnnotation(id=id_scope.getNewId(DBAnnotation.vtType),
                                            key='notes',
                                            value=_action.db_notes.strip(),
                                            )
    return DBAction(id=_action.db_time,
                    prevId=_action.db_parent,
                    date=convertDate(_action.db_date),
                    user=_action.db_user,
                    operations=operations,
                    annotations=annotations,
                    )

def translateAddModuleAction(_action, id_scope):
    operations = []
    for _module in _action.db_datas:
        # note that we're just blindly setting all cache tags to 1 since
        # v0.3.1 didn't really switch this
        module = DBModule(id=_module.db_id,
                          name=_module.db_name,
                          cache=1,
                          location=DBLocation(id=_module.db_id,
                                              x=_module.db_x,
                                              y=_module.db_y))
        module.db_location.relative = False
        operation = DBAdd(id=_action.db_time,
                          what='module',
                          objectId=_module.db_id,
                          data=module)
        operations.append(operation)

    return createAction(_action, operations, id_scope)

def translateAddConnectionAction(_action, id_scope):
    operations = []
    for _connection in _action.db_datas:
        source = DBPort(id=_connection.db_id,
                        type='source',
                        moduleId=_connection.db_sourceId,
                        moduleName=_connection.db_sourceModule,
                        sig=_connection.db_sourcePort)
        destination = DBPort(id=_connection.db_id,
                             type='destination',
                             moduleId=_connection.db_destinationId,
                             moduleName=_connection.db_destinationModule,
                             sig=_connection.db_destinationPort)
        connection = DBConnection(id=_connection.db_id,
                                  ports=[source, destination])
        operation = DBAdd(id=_action.db_time,
                          what='connection',
                          objectId=_connection.db_id,
                          data=connection)
        operations.append(operation)

    return createAction(_action, operations, id_scope)

def translateChangeParameterAction(_action, id_scope):
    operations = []
    for _set in _action.db_datas:
        parameter = DBParameter(id=_set.db_parameterId,
                                pos=_set.db_parameterId,
                                name=_set.db_parameter,
                                alias=_set.db_alias,
                                val=_set.db_value,
                                type=_set.db_type)
        function = DBFunction(id=_set.db_functionId,
                              pos=_set.db_functionId,
                              name=_set.db_function,
                              parameters=[parameter])
        operation = DBChange(id=_action.db_time,
                             what='function',
                             oldObjId=_set.db_functionId,
                             parentObjId=_set.db_moduleId,
                             parentObjType='module',
                             data=function)
        operations.append(operation)

    return createAction(_action, operations, id_scope)

def translateAddModulePortAction(_action, id_scope):
    operations = []
    for _portSpec in _action.db_datas:
        # ids need to be checked
        portSpec = DBPortSpec(id=_portSpec.db_moduleId,
                              name=_portSpec.db_portName,
                              type=_portSpec.db_portType,
                              spec=_portSpec.db_portSpec)
        operation = DBAdd(id=_action.db_time,
                          what='portSpec',
                          objectId=(_portSpec.db_portName,
                                    _portSpec.db_portType),
                          parentObjId=_portSpec.db_moduleId,
                          parentObjType='module',
                          data=portSpec)
        operations.append(operation)

    return createAction(_action, operations, id_scope)

def translateChangeAnnotationAction(_action, id_scope):
    operations = []
    for _annotation in _action.db_datas:
        if _annotation.db_key.strip() != '' or \
                _annotation.db_value.strip() != '':
            annotation = DBAnnotation(id=-1,
                                      key=_annotation.db_key,
                                      value=_annotation.db_value)
            operation = DBChange(id=_action.db_time,
                                 what='annotation',
                                 oldObjId=_annotation.db_key,
                                 parentObjId=_annotation.db_moduleId,
                                 parentObjType='module',
                                 data=annotation)
            operations.append(operation)

    return createAction(_action, operations, id_scope)

def translateDeleteModuleAction(_action, id_scope):
    operations = []
    for _module in _action.db_datas:
        operation = DBDelete(id=_action.db_time,
                             what='module',
                             objectId=_module.db_moduleId)
        operations.append(operation)

    return createAction(_action, operations, id_scope)

def translateDeleteConnectionAction(_action, id_scope):
    operations = []
    for _connection in _action.db_datas:
        operation = DBDelete(id=_action.db_time,
                             what='connection',
                             objectId=_connection.db_connectionId)
        operations.append(operation)

    return createAction(_action, operations, id_scope)

def translateDeleteFunctionAction(_action, id_scope):
    operations = []
    for _function in _action.db_datas:
        operation = DBDelete(id=_action.db_time,
                             what='function',
                             objectId=_function.db_functionId,
                             parentObjId=_function.db_moduleId,
                             parentObjType='module')
        operations.append(operation)

    return createAction(_action, operations, id_scope)

def translateDeleteAnnotationAction(_action, id_scope):
    operations = []
    for _annotation in _action.db_datas:
        operation = DBDelete(id=_action.db_time,
                             what='annotation',
                             objectId=_annotation.db_key)
        operations.append(operation)

    return createAction(_action, operations, id_scope)

def translateDeleteModulePortAction(_action, id_scope):
    operations = []
    for _portSpec in _action.db_datas:
        operation = DBDelete(id=_action.db_time,
                             what='portSpec',
                             objectId=(_portSpec.db_portName,
                                       _portSpec.db_portType),
                             parentObjId=_portSpec.db_moduleId,
                             parentObjType='module')
        operations.append(operation)

    return createAction(_action, operations, id_scope)

def translateMoveModuleAction(_action, id_scope):
    operations = []
    for _location in _action.db_datas:
        location = DBLocation(id=_location.db_id,
                              x=_location.db_dx,
                              y=_location.db_dy)
        location.relative = True

        operation = DBChange(id=_action.db_time,
                             what='location',
                             oldObjId=_location.db_id,
                             parentObjId=_location.db_id,
                             parentObjType='module',
                             data=location)
        operations.append(operation)

    return createAction(_action, operations, id_scope)

### UPDATE IDS ###

def convertIds(vistrail):
    actions = vistrail.db_get_actions()
    actions.sort(key=lambda x: x.db_id)
    objectDict = {}
#    refDict = {'objectDict': objectDict}

    graph = Graph()
    for action in actions:
        graph.add_vertex(action.db_id)
        graph.add_edge(action.db_prevId, action.db_id)

    def convertAction(actionId):
#         print 'converting %s' % actionId
        if actionId == 0:
            return
        allOps = []
        action = vistrail.db_get_action(actionId)
#         objectDict = refDict['objectDict']
#         if action.actionType == 'delete' or action.actionType == 'change':
#             action.objectDict = copy.deepcopy(objectDict)
#         else:
#             action.objectDict = objectDict
        for operation in action.db_get_operations():
            allOps.extend(convertOperation(vistrail,
                                           objectDict,
                                           operation.vtType,
                                           operation))
        action.db_operations = allOps

    def removeObjects(actionId):
        if actionId == 0:
            return
#        print "removeObjects(%s)" % actionId
        action = vistrail.db_get_action(actionId)

        # need to reverse ops here
        reverseOps = action.db_get_operations()
        reverseOps.reverse()
        for operation in reverseOps:
            parentList = getTypeIdList(operation)
            removeObject(operation.db_what,
                         operation.db_oldId,
                         objectDict,
                         parentList[:-1])
        reverseOps.reverse()


    graph.dfs(enter_vertex=convertAction,
              leave_vertex=removeObjects)

def getTypeIdList(operation):
    if operation.db_what in ('module', 'connection'):
        return [(operation.db_what, operation.db_oldId)]
    elif operation.db_what in \
            ('function', 'portSpec', 'location', 'annotation'):
        return [('module', operation.db_oldParentId),
                (operation.db_what, operation.db_oldId)]
    elif operation.db_what in ('port'):
        return [('connection', operation.db_oldParentId),
                (operation.db_what, operation.db_oldId)]
    elif operation.db_what in ('parameter'):
        return [('module', operation.db_moduleId),
                ('function', operation.db_oldParentId),
                ('parameter', operation.db_oldId)]
    else:
        print "unknown type: '%s'" % operation.db_what
        return [(operation.db_what, operation.db_oldId)]  
      
def getOldId(object):
    if object.vtType == 'annotation':
        return object.db_key
    elif object.vtType == 'port':
        return object.db_type
    elif object.vtType == 'portSpec':
        return (object.db_name, object.db_type)
    else:
        return object.getPrimaryKey()

def getChildren(object):
    childList = []
    if object.vtType == 'module':
        childList = object.db_get_functions() + \
            object.db_get_portSpecs() + \
            object.db_get_annotations()
        childList.append(object.db_location)
        object.db_functions = []
        object.db_portSpecs = {}
        object.db_annotations = {}
        object.db_location = None
    elif object.vtType == 'connection':
         childList = object.db_get_ports()
         object.db_ports = []
    elif object.vtType == 'function':
        childList =  object.db_get_parameters()
        object.db_parameters = []
    return childList

def captureObject(object, objectDict, newId, parentList):
    
#    print "capturing %s" % object
    currentDict = objectDict
    for key in parentList:
        (objType, objId) = key
#        (currentId, newDict, _) = currentDict[(objType, objId)]
#        currentDict = newDict
        (objList, curIdx) = currentDict[(objType, objId)]
        currentDict = objList[curIdx][1]
    oldId = getOldId(object)
#    print "capture: %s %s" % (object.vtType, oldId)
#    currentDict[(object.vtType, oldId)] = (newId, {}, object)
    if not currentDict.has_key((object.vtType, oldId)):
        currentDict[(object.vtType, oldId)] = ([], -1)
    (curList, curIdx) = currentDict[(object.vtType, oldId)]
    curList.append((newId, {}, object, curIdx))
    currentDict[(object.vtType, oldId)] = (curList, len(curList) - 1)

def captureDelete(objType, objId, objectDict, parentList):
    currentDict = objectDict
    for (aType, aId) in parentList:
#        (currentId, newDict, _) = currentDict[(objType, objId)]
#        currentDict = newDict
        (objList, curIdx) = currentDict[(aType, aId)]
        currentDict = objList[curIdx][1]

#    print "captureDelete: %s %s" % (objType, objId)
    if not currentDict.has_key((objType, objId)):
        raise Exception("invalid delete")
    (curList, curIdx) = currentDict[(objType, objId)]
    curList.append((-1, {}, None, curIdx))
    currentDict[(objType, objId)] = (curList, len(curList) - 1)

def removeObject(oldObjType, oldId, objectDict, parentList):
#     print '%s %s' % (oldObjType, oldId)
#     print objectDict
#     print parentList

    try:
        currentDict = objectDict
        for key in parentList:
            (objType, objId) = key
#            (currentId, newDict, _) = currentDict[(objType, objId)]
#            currentDict = newDict
            (objList, objIdx) = currentDict[(objType, objId)]
            currentDict = objList[objIdx][1]
#        print "remove: %s %s" % (oldObjType, oldId)
        (curList, curIdx) = currentDict[(oldObjType, oldId)]
#        print "ok"
        newIdx = curList[curIdx][3]
#        del curList[curIdx]
        currentDict[(oldObjType, oldId)] = (curList, newIdx)
    except KeyError:
        print "cannot remove (%s, %s)" % (oldObjType, oldId)
        print parentList
        print objList
        print "index: %s"  % objIdx

def findNewId(typeIdList, objectDict):
    try:
        currentDict = objectDict
        for key in typeIdList:
#            (currentId, currentDict, currentObj) = currentDict[key]
            (objList, curIdx) = currentDict[key]
            if curIdx == -1:
                return (None, None)
            (currentId, currentDict, currentObj, _) = objList[curIdx]
        if currentId == -1:
            return (None, None)
        return (currentId, currentObj)
    except KeyError:
        pass
    return (None, None)

def getChildList(typeIdList, objectDict):
    try:
        currentDict = objectDict
        for (objType, objOldId) in typeIdList:
#            (currentId, currentDict, _) = currentDict[(objType, objOldId)]
            (objList, curIdx) = currentDict[(objType, objOldId)]
            if curIdx == -1:
                return {}
            currentDict = objList[curIdx][1]
        return currentDict
    except KeyError:
        pass
    return {}
      
def createOperation(actionType, objId, objType, parentId, parentType,
                    object=None):
    if actionType == 'add':
        operation = DBAdd(what=objType,
                          objectId=objId,
                          parentObjId=parentId,
                          parentObjType=parentType,
                          data=object)

    elif actionType == 'change':
        operation = DBChange(what=objType,
                             oldObjId=objId,
                             parentObjId=parentId,
                             parentObjType=parentType,
                             data=object)
    elif actionType == 'delete':
        operation = DBDelete(what=objType,
                             objectId=objId,
                             parentObjId=parentId,
                             parentObjType=parentType)
    else:
        msg = "Cannot find actionType='%s'" % actionType
        raise Exception(msg)

    return operation

def convertChangeToAdd(operation):
    return DBAdd(what=operation.db_what,
                 objectId=operation.db_newObjId,
                 parentObjId=operation.db_parentObjId,
                 parentObjType=operation.db_parentObjType,
                 data=operation.db_data)
    
def convertOperation(vistrail, objectDict, actionType, operation):
    newOps = []
    if actionType == 'add':
        object = operation.db_data
        if object.vtType == 'parameter' and object.db_pos == -1:
            return newOps
        operation.db_oldId = operation.db_objectId
        if operation.db_what == 'annotation':
            operation.db_oldId = object.db_key
        elif operation.db_what == 'port':
            operation.db_oldId = object.db_type
        operation.db_oldParentId = operation.db_parentObjId
        parentList = getTypeIdList(operation)


        newId = vistrail.idScope.getNewId(object.vtType)
        captureObject(object, objectDict, newId, parentList[:-1])
        operation.db_objectId = newId
        oldId = object.getPrimaryKey()
        if object.vtType == 'annotation':
            oldId = object.db_key
        elif object.vtType == 'port':
            oldId = object.db_type
        if hasattr(object, 'db_id'):
            object.db_id = newId

        # set parent ids correctly...
        operation.db_id = vistrail.idScope.getNewId('operation')
        if operation.db_parentObjId is not None:
            oldParentObjId = operation.db_parentObjId
            operation.db_parentObjId = findNewId(parentList[:-1], objectDict)[0]
        if object.vtType == 'port':
            object.db_moduleId = \
                findNewId([('module', object.db_moduleId)], objectDict)[0]
#         if object.vtType == 'connection':
#             for port in object.db_ports.itervalues():
#                 port.db_moduleId = \
#                     findNewId([('module', port.db_moduleId)], objectDict)[0]

        newOps.append(operation)

        # set child operations
        children = getChildren(object)
        for child in children:
            # hack to get around fact that location ids are wrong
            if child.vtType == 'location':
                child.db_id = oldId
            newOp = createOperation('add',
                                    child.getPrimaryKey(),
                                    child.vtType,
                                    oldId,
                                    object.vtType,
                                    child)

            # hack to get moduleId at parameter level
            if child.vtType == 'parameter':
                newOp.db_moduleId = oldParentObjId
            newOps.extend(convertOperation(vistrail, 
                                           objectDict, 
                                           'add',
                                           newOp))
            newOp.db_parentObjId = newId
    elif actionType == 'change':
        object = operation.db_data
        if object.vtType == 'parameter' and object.db_pos == -1:
            return newOps
        operation.db_oldId = operation.db_oldObjId
        if operation.db_what == 'annotation':
            operation.db_oldId = object.db_key
        elif operation.db_what == 'port':
            operation.db_oldId = object.db_type
        operation.db_oldParentId = operation.db_parentObjId
        parentList = getTypeIdList(operation)

        # need to get changed id as new id if have one
        (foundId, foundObj) = findNewId(parentList, objectDict)
        if foundId is not None:
            if foundObj.vtType == 'function' and \
                    foundObj.db_pos == object.db_pos and \
                    foundObj.db_name == object.db_name:
                # don't create new function, convert parameter
                for parameter in object.db_parameters:
                    newOp = createOperation('change',
                                            parameter.getPrimaryKey(),
                                            parameter.vtType,
                                            object.getPrimaryKey(),
                                            object.vtType,
                                            parameter)
                    newOp.db_moduleId = operation.db_parentObjId
                    newOps.extend(convertOperation(vistrail,
                                                   objectDict,
                                                   'change',
                                                   newOp))
                    newOp.db_parentObjId = foundId
                return newOps
            else:
                if foundObj.vtType == 'location' and object.relative == True:
                    object.db_x += foundObj.db_x
                    object.db_y += foundObj.db_y
                    object.relative = False
                # get new id for new object
                newId = vistrail.idScope.getNewId(object.vtType)
                operation.db_oldObjId = foundId
                operation.db_newObjId = newId
        else:
            # get new id for new object
            newId = vistrail.idScope.getNewId(object.vtType)
            operation.db_oldObjId = -1
            operation.db_newObjId = newId
            anOldId = operation.db_oldId
            anOldParentId = operation.db_parentObjId
            if hasattr(operation,'db_moduleId'):
                aModuleId = operation.db_moduleId
            else:
                aModuleId = None
            operation = convertChangeToAdd(operation)
            operation.db_oldId = anOldId
            operation.db_oldParentId = operation.db_parentObjId
            operation.db_moduleId = aModuleId

        # need to do child deletes first
        childDict = getChildList(parentList, objectDict)
        for k,v in childDict.items():
            (objType, objId) = k
#            (newId, newDict) = v
#            print 'creating delete for %s'  % objType
            newOp = createOperation('delete',
                                    objId,
                                    objType,
                                    object.getPrimaryKey(),
                                    object.vtType)
            # hack to get moduleId at parameter level
            if objType == 'parameter':
                newOp.db_moduleId = operation.db_parentObjId
            newOps.extend(convertOperation(vistrail,
                                           objectDict,
                                           'delete',
                                           newOp))
            newOp.db_parentObjId = newId
        # don't reverse -- ordering is correct
        # newOps.reverse()

        # set new object id
        captureObject(object, objectDict, newId, parentList[:-1])
#        operation.db_objectId = newId
        oldId = object.getPrimaryKey()
        if object.vtType == 'annotation':
            oldId = object.db_key
        elif object.vtType == 'port':
            oldId = object.db_type
        if hasattr(object, 'db_id'):
            object.db_id = newId

        # set parent ids correctly...
        operation.db_id = vistrail.idScope.getNewId('operation')
        if operation.db_parentObjId is not None:
            oldParentObjId = operation.db_parentObjId
            operation.db_parentObjId = findNewId(parentList[:-1], objectDict)[0]
        if object.vtType == 'port':
            object.db_moduleId = \
                findNewId([('module', object.db_moduleId)], objectDict)[0]
#         if object.vtType == 'connection':
#             for port in object.db_ports.itervalues():
#                 port.db_moduleId = \
#                     findNewId([('module', port.db_moduleId)], objectDict)[0]
        newOps.append(operation)

        # set child operations
        children = getChildren(operation.db_data)
        for child in children:
#            print 'creating add for %s' % child.vtType
            newOp = createOperation('add',
                                    child.getPrimaryKey(),
                                    child.vtType,
                                    oldId,
                                    object.vtType,
                                    child)
            # hack to get moduleId at parameter level
            if child.vtType == 'parameter':
                newOp.db_moduleId = oldParentObjId
            newOps.extend(convertOperation(vistrail, 
                                           objectDict, 
                                           'add',
                                           newOp))
            newOp.db_parentObjId = newId
    elif actionType == 'delete':
        operation.db_oldId = operation.db_objectId
#         if operation.db_what == 'annotation':
#             operation.db_oldId = object.db_key
#         elif operation.db_what == 'port':
#             operation.db_oldId = object.db_type
        operation.db_oldParentId = operation.db_parentObjId
        parentList = getTypeIdList(operation)

        # get new id for delete operation
        (newId, _) = findNewId(parentList, objectDict)
#        print 'found new id:  %s' % newId
        if newId is None:
            msg = "Cannot find id: %s" % parentList
            print msg
#            raise Exception(msg)
            return []

        # need to do child deletes first
        childDict = getChildList(parentList, objectDict)
        for k,v in childDict.items():
            (objType, objId) = k
#            (newId, newDict) = v
            newOp = createOperation('delete',
                                    objId,
                                    objType,
                                    operation.db_objectId,
                                    operation.db_what)
            # hack to get moduleId at parameter level
            if objType == 'parameter':
                newOp.db_moduleId = operation.db_parentObjId
            newOps.extend(convertOperation(vistrail,
                                           objectDict,
                                           'delete',
                                           newOp))
            newOp.db_parentObjId = newId
#        newOps.reverse()

        captureDelete(operation.db_what, operation.db_objectId, objectDict, 
                      parentList[:-1])
        operation.db_objectId = newId
        
        # set parent ids correctly
        operation.db_id = vistrail.idScope.getNewId('operation')
        if operation.db_parentObjId is not None:
            operation.db_parentObjId = findNewId(parentList[:-1], objectDict)[0]
        newOps.append(operation)

    return newOps
