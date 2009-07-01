
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

from db.domain import DBWorkflow, DBAdd, DBDelete, DBAction, DBAbstractionRef, \
    DBModule, DBConnection, DBPort, DBFunction, DBParameter, DBGroup
from db.services.action_chain import getActionChain, getCurrentOperationDict, \
    getCurrentOperations
from db import VistrailsDBException
import copy
import datetime
import getpass

def update_id_scope(vistrail):
    for action in vistrail.db_actions:
        vistrail.idScope.updateBeginId('action', action.db_id+1)
        if action.db_session is not None:
            vistrail.idScope.updateBeginId('session', action.db_session + 1)
        for operation in action.db_operations:
            vistrail.idScope.updateBeginId('operation', operation.db_id+1)
            if operation.vtType == 'add' or operation.vtType == 'change':
                # update ids of data
                vistrail.idScope.updateBeginId(operation.db_what, 
                                               getNewObjId(operation)+1)
                if operation.db_data is None:
                    if operation.vtType == 'change':
                        operation.db_objectId = operation.db_oldObjId
                vistrail.db_add_object(operation.db_data)
#     print ' *** vistrail objects:'
#     for key in sorted(vistrail.db_objects.keys()):
#         print '%s %s' % (key[0], key[1])
        for annotation in action.db_annotations:
            vistrail.idScope.updateBeginId('annotation', annotation.db_id+1)
    for abstraction in vistrail.db_abstractions:
        vistrail.idScope.updateBeginId('abstraction', abstraction.db_id+1)

def materializeWorkflow(vistrail, version):
    # construct path up through tree and perform each action
    if vistrail.db_has_action_with_id(version):
        workflow = DBWorkflow()
        #      for action in getActionChain(vistrail, version):
        #    oldPerformAction(action, workflow)
        performActions(getActionChain(vistrail, version), 
                            workflow)
        workflow.db_id = version
        workflow.db_vistrailId = vistrail.db_id
        return workflow
    elif version == 0:
        return DBWorkflow()
    else:
        raise VistrailsDBException("invalid workflow version %s" % version)

def expandGroups(vistrail, workflow, module_remap=None):
    workflow.__class__ = DBWorkflow
    def get_tmp_id(type):
        return -workflow.tmp_id.getNewId(type)
    
    found_group = False
    if module_remap is None:
        module_remap = {}
    add_modules = []
    add_connections = []
    delete_modules = {}
    delete_connections = {}
    in_conns = {}
    out_conns = {}
    for module in workflow.db_modules:
        if module.vtType == DBGroup.vtType:
            delete_modules[module.db_id] = module
            found_group = True
            id_remap = {}
#             abstraction = \
#                 vistrail.db_abstractions_id_index[module.db_abstraction_id]
#             sub_workflow = materializeWorkflow(abstraction, module.db_version)
            sub_workflow = module.pipeline

            # need to copy modules and connections over to workflow
            # except that connections from abstraction module to "outside"
            # need to be remapped "inside"
            for a_module in sub_workflow.db_modules:
                if not ((a_module.db_name == 'InputPort' or
                         a_module.db_name == 'OutputPort') and
                        a_module.db_package == \
                            'edu.utah.sci.vistrails.basic'):
                    new_id = get_tmp_id(DBModule.vtType)
                    id_remap[a_module.db_id] = new_id
                    module_remap[new_id] = (module.db_id, 
                                            a_module.db_id,
                                            module.db_id,
                                            module.db_version)
                    a_module.db_id = new_id
                    add_modules.append(a_module)

            for a_connection in sub_workflow.db_connections:
                source = a_connection.db_ports_type_index['source']
                dest = a_connection.db_ports_type_index['destination']
                # currently we don't allow pass through connections so the 
                # if/elf is ok
                if source.db_name == 'InternalPipe' and \
                        source.db_moduleName == 'InputPort':
                    in_module = \
                        sub_workflow.db_modules_id_index[source.db_moduleId]
                    for function in in_module.db_functions:
                        if function.name == 'name':
                            name = function.db_parameters[0].strValue
                        elif function.name == 'spec':
                            spec = function.db_parameters[0].strValue
                        elif function.name == 'old_name':
                            old_name = function.db_parameters[0].strValue
                    in_conns[(module.db_id, name, spec)] = a_connection
                    a_module = \
                        sub_workflow.db_modules_id_index[dest.db_moduleId]
                    dest.db_moduleId = id_remap[dest.db_moduleId]

                    for function in module.db_functions:
                        if function.name == name:
                            new_id = get_tmp_id(DBFunction.vtType)
                            new_function = function.do_copy()
                            new_function.db_id = new_id
                            new_function.db_name = old_name
                            for new_param in new_function.db_parameters:
                                param_id = get_tmp_id(DBParameter.vtType)
                                new_param.db_id = param_id
                            a_module.add_function(new_function)
                        
                elif dest.db_name == 'InternalPipe' and \
                        dest.db_moduleName == 'OutputPort':
                    out_module = \
                        sub_workflow.db_modules_id_index[dest.db_moduleId]
                    for function in out_module.db_functions:
                        if function.name == 'name':
                            name = function.db_parameters[0].strValue
                        elif function.name == 'spec':
                            spec = function.db_parameters[0].strValue
                    out_conns[(module.db_id, name, spec)] = a_connection
                    source.db_moduleId = id_remap[source.db_moduleId]
                else:
                    dest.db_moduleId = id_remap[dest.db_moduleId]
                    source.db_moduleId = id_remap[source.db_moduleId]
                    a_connection.db_id = get_tmp_id(DBConnection.vtType)
                    add_connections.append(a_connection)                

    for connection in workflow.db_connections:
        source = connection.db_ports_type_index['source']
        dest = connection.db_ports_type_index['destination']
        new_source = None
        new_dest = None
        if source.db_moduleId in delete_modules:
            a_connection = out_conns[(source.db_moduleId,
                                      source.db_name, source.db_spec)]
            a_source = a_connection.db_ports_type_index['source']
            new_source = a_source.do_copy()
            new_source.db_id = get_tmp_id(DBPort.vtType)
        if dest.db_moduleId in delete_modules:
            a_connection = in_conns[(dest.db_moduleId,
                                     dest.db_name, dest.db_spec)]
            a_dest = a_connection.db_ports_type_index['destination']
            new_dest = a_dest.do_copy()
            new_dest.db_id = get_tmp_id(DBPort.vtType)
        if new_source is not None or new_dest is not None:
            if new_source is None:
                new_source = source.do_copy()
                new_source.db_id = get_tmp_id(DBPort.vtType)
            if new_dest is None:
                new_dest = dest.do_copy()
                new_dest.db_id = get_tmp_id(DBPort.vtType)

            new_connection = \
                DBConnection(id=get_tmp_id(DBConnection.vtType),
                             ports=[new_source, new_dest])
            add_connections.append(new_connection)
            delete_connections[connection.db_id] = connection

    for connection in delete_connections.itervalues():
        workflow.db_delete_connection(connection)
    for module in delete_modules.itervalues():
        workflow.db_delete_module(module)
    for module in add_modules:
        workflow.db_add_module(module)
    for connection in add_connections:
        workflow.db_add_connection(connection)

    if not found_group:
        return (workflow, module_remap)
    return expandGroups(vistrail, workflow, module_remap)

def expandWorkflow(vistrail, workflow=None, version=None):
    if workflow is None:
        if version is not None:
            workflow = materializeWorkflow(vistrail, version)
        else:
            msg = "need to specify workflow or version"
            raise VistrailsDBException(msg)
    else:
        workflow = workflow.do_copy()
    return expandGroups(vistrail, workflow)
        
def performAction(action, workflow):
    if action.actionType == 'add':
        for operation in action.db_operations:
            workflow.db_add_object(operation.db_data, 
                                   operation.db_parentObjType,
                                   operation.db_parentObjId)
    elif action.actionType == 'change':
        for operation in action.db_operations:
            workflow.db_change_object(operation.db_data,
                                      operation.db_parentObjType,
                                      operation.db_parentObjId)
    elif action.actionType == 'delete':
        for operation in action.operations:
            workflow.db_delete_object(operation.db_objectId,
                                      operation.db_what,
                                      operation.db_parentObjType,
                                      operation.db_parentObjId)
    else:
        msg = "Unrecognized action type '%s'" % action.db_actionType
        raise Exception(msg)

def performDeletes(deleteOps, workflow):
    for operation in deleteOps:
        workflow.db_delete_object(getOldObjId(operation), operation.db_what,
                                  operation.db_parentObjType,
                                  operation.db_parentObjId)

def performAdds(addOps, workflow):
    for operation in addOps:
#         print "operation %d: %s %s" % (operation.db_id, operation.vtType,
#                                        operation.db_what)
#         print "    to:  %s %s" % (operation.db_parentObjType, 
#                                   operation.db_parentObjId)
        workflow.db_add_object(operation.db_data,
                               operation.db_parentObjType,
                               operation.db_parentObjId)

def performActions(actions, workflow):
    # get the current actions and run addObject on the workflow
    # note that delete actions have been removed and
    # a change after an add is effectively an add if the add is discarded
    performAdds(getCurrentOperations(actions), workflow)

def synchronize(old_vistrail, new_vistrail):
    current_version = new_vistrail.db_currentVersion
    id_remap = {}
    for action in new_vistrail.db_actions:
        if action.is_new:
            new_action = action.do_copy(True, old_vistrail.idScope, id_remap)
            old_vistrail.db_add_action(new_action)
        else:
            # it must exist in the old vistrail, too
            old_action = old_vistrail.db_actions_id_index[action.db_id]
            # use knowledge that we replace old notes...
            for annotation in action.db_deleted_annotations:
                if old_action.db_has_annotation_with_id(annotation.db_id):
                    old_action.db_delete_annotation(annotation)
                else:
                    # FIXME conflict!
                    # we know that the annotation that was there isn't anymore
                    print 'possible notes conflict'
                    if old_action.db_has_annotation_with_key('notes'):
                        old_annotation = \
                            old_action.db_get_annotation_by_key('notes')
                        old_action.db_delete_annotation(old_annotation)
                    else:
                        # we don't have to do anything
                        pass
            for annotation in action.db_annotations:
                if annotation.is_new:
                    new_annotation = annotation.do_copy(True, 
                                                        old_vistrail.idScope,
                                                        id_remap)
                    old_action.db_add_annotation(new_annotation)

    for tag in new_vistrail.db_deleted_tags:
        if old_vistrail.db_has_tag_with_id(tag.db_id):
            old_vistrail.db_delete_tag(tag)
        else:
            # FIXME conflict!
            # we know the tag that was there isn't anymore
            print 'possible tag conflict'
            # we don't have to do anything here, though
            pass

    for tag in new_vistrail.db_tags:
        if tag.is_new:
            new_tag = tag.do_copy(False)
            # remap id
            try:
                new_tag.db_id = id_remap[(DBAction.vtType, new_tag.db_id)]
            except KeyError:
                pass
            try:
                old_tag = old_vistrail.db_tags_name_index[new_tag.db_name]
            except KeyError:
                # FIXME conflict!
                print "tag conflict--name already used"
                old_vistrail.db_delete_tag(old_tag)
            try:
                old_tag = old_vistrail.db_tags_id_index[new_tag.db_id]
            except KeyError:
                print 'possible tag conflict -- WILL NOT GET HERE!'
                old_vistrail.db_delete_tag(old_tag)
            old_vistrail.db_add_tag(new_tag)

    new_version = \
        id_remap.get((DBAction.vtType, current_version), current_version)
    old_vistrail.db_currentVersion = new_version

################################################################################
# Analogy methods

def find_data(what, id, op_dict):
    try:
        return op_dict[(what, id)].db_data
    except KeyError:
        msg = 'cannot find data (%s, %s)'  % (what, id)
        raise Exception(msg)

def invertOperations(op_dict, adds, deletes):
    # 2008-07-08 cscheid
    # Copying is slow, so I'm just passing around the reference into
    # the dbadds and deletes. This might be dangerous.
    inverse_ops = []       
    deletes.reverse()
    for op in deletes:
        data = find_data(op.db_what, getOldObjId(op), op_dict)
        inv_op = DBAdd(id=-1,
                       what=op.db_what,
                       objectId=getOldObjId(op),
                       parentObjId=op.db_parentObjId,
                       parentObjType=op.db_parentObjType,
                       data=data
                       )
        inverse_ops.append(inv_op)
    adds.reverse()
    for op in adds:
        inv_op = DBDelete(id=-1,
                          what=op.db_what,
                          objectId=getNewObjId(op),
                          parentObjId=op.db_parentObjId,
                          parentObjType=op.db_parentObjType,
                          )
        inverse_ops.append(inv_op)
    return inverse_ops

def normalOperations(adds, deletes):
    # 2008-07-08 cscheid
    # Copying is slow, so I'm just passing around the reference into
    # the dbadds and deletes. This might be dangerous.
    new_ops = []
    for op in deletes:
        new_op = DBDelete(id=-1,
                          what=op.db_what,
                          objectId=getOldObjId(op),
                          parentObjId=op.db_parentObjId,
                          parentObjType=op.db_parentObjType,
                          )
        new_ops.append(new_op)
    for op in adds:
        new_op = DBAdd(id=-1,
                       what=op.db_what,
                       objectId=getNewObjId(op),
                       parentObjId=op.db_parentObjId,
                       parentObjType=op.db_parentObjType,
                       data=op.db_data)
        new_ops.append(new_op)
    return new_ops        

def simplifyOps(ops):
    addDict = {}
    deleteDict = {}
    opCount = -1
    for op in ops:
        op.db_id = opCount
        if op.vtType == 'add':
            addDict[(op.db_what, op.db_objectId)] = op
        elif op.vtType == 'delete':
            try:
                del addDict[(op.db_what, op.db_objectId)]
            except KeyError:
                deleteDict[(op.db_what, op.db_objectId)] = op
        elif op.vtType == 'change':
            try:
                k = addDict[(op.db_what, op.db_oldObjId)]
            except KeyError:
                addDict[(op.db_what, op.db_newObjId)] = op
            else:
                old_old_id = getOldObjId(k)
                del addDict[(op.db_what, op.db_oldObjId)]
                addDict[(op.db_what, op.db_newObjId)] = \
                    DBChange(id=opCount,
                             what=op.db_what,
                             oldObjId=old_old_id,
                             newObjId=op.db_newObjId,
                             parentObjId=op.db_parentObjId,
                             parentObjType=op.db_parentObjType,
                             )
        opCount -= 1

    deletes = deleteDict.values()
    deletes.sort(key=lambda x: -x.db_id) # faster than sort(lambda x, y: -cmp(x.db_id, y.db_id))
    adds = addDict.values()
    adds.sort(key=lambda x: -x.db_id) # faster than sort(lambda x, y: -cmp(x.db_id, y.db_id))
    return deletes + adds

def getPathAsAction(vistrail, v1, v2):
    sharedRoot = getSharedRoot(vistrail, [v1, v2])
    sharedActionChain = getActionChain(vistrail, sharedRoot)
    sharedOperationDict = getCurrentOperationDict(sharedActionChain)
    v1Actions = getActionChain(vistrail, v1, sharedRoot)
    v2Actions = getActionChain(vistrail, v2, sharedRoot)
    (v1AddDict, v1DeleteDict) = getOperationDiff(v1Actions, 
                                                 sharedOperationDict)
    (v2AddDict, v2DeleteDict) = getOperationDiff(v2Actions,
                                                 sharedOperationDict)
    
    # need to invert one of them (v1)
    v1Adds = v1AddDict.values()
    v1Adds.sort(key=lambda x: x.db_id) # faster than sort(lambda x, y: cmp(x.db_id, y.db_id))
    v1Deletes = v1DeleteDict.values()
    v1Deletes.sort(key=lambda x: x.db_id) # faster than sort(lambda x, y: cmp(x.db_id, y.db_id))
    v1InverseOps = invertOperations(sharedOperationDict, v1Adds, v1Deletes)
    
    # need to normalize ops of the other (v2)
    v2Adds = v2AddDict.values()
    v2Adds.sort(key=lambda x: x.db_id) # faster than sort(lambda x, y: cmp(x.db_id, y.db_id))
    v2Deletes = v2DeleteDict.values()
    v2Deletes.sort(key=lambda x: x.db_id) # faster than sort(lambda x, y: cmp(x.db_id, y.db_id))
    v2Ops = normalOperations(v2Adds, v2Deletes)

    allOps = v1InverseOps + v2Ops
    simplifiedOps = simplifyOps(allOps)
    return DBAction(id=-1, 
                    operations=simplifiedOps,
                    )

def addAndFixActions(startDict, actions):
    curDict = copy.copy(startDict)
    # print curDict
    for action in actions:
#         print "fixing action:", action.db_id
        new_ops = []
        for op in action.db_operations:
#             print "op:", op.vtType, op.db_what, getOldObjId(op)
#             print "   ", op.db_parentObjType, op.db_parentObjId
            if op.vtType == 'add':
                if op.db_parentObjId is None or \
                        curDict.has_key((op.db_parentObjType, 
                                         op.db_parentObjId)):
                    curDict[(op.db_what, op.db_objectId)] = op
                    new_ops.append(op)                    
            elif op.vtType == 'change':
                if curDict.has_key((op.db_what, op.db_oldObjId)) and \
                        (op.db_parentObjId is None or \
                             curDict.has_key((op.db_parentObjType, 
                                              op.db_parentObjId))):
                    del curDict[(op.db_what, op.db_oldObjId)]
                    curDict[(op.db_what, op.db_newObjId)] = op
                    new_ops.append(op)
            elif op.vtType == 'delete':
                if (op.db_parentObjId is None or
                    curDict.has_key((op.db_parentObjType, 
                                     op.db_parentObjId))) and \
                    curDict.has_key((op.db_what, op.db_objectId)):
                    del curDict[(op.db_what, op.db_objectId)]
                    new_ops.append(op)
        action.db_operations = new_ops
    return curDict

def fixActions(vistrail, v, actions):
    startingChain = getActionChain(vistrail, v)
    startingDict = getCurrentOperationDict(startingChain)
    addAndFixActions(startingDict, actions)
    
################################################################################
# Diff methods

def getSharedRoot(vistrail, versions):
    # base case is 0
    current = copy.copy(versions)
    while 0 not in current:
        maxId = max(current)
        if current.count(maxId) == len(current):
            return maxId
        else:
            newId = vistrail.db_get_action_by_id(maxId).db_prevId
            for i, v in enumerate(current):
                if v == maxId:
                    current[i] = newId
    return 0

def getOperationDiff(actions, operationDict):
    addDict = {}
    deleteDict = {}
    for action in actions:
#         print 'action: %d' % action.db_id
        for operation in action.db_operations:
            if operation.vtType == 'add':
#                 print "add: %s %s" % (operation.db_what, 
#                                       operation.db_objectId)
                addDict[(operation.db_what, 
                         operation.db_objectId)] = operation
            elif operation.vtType == 'delete':
#                 print "del: %s %s" % (operation.db_what, 
#                                       operation.db_objectId)
                if operationDict.has_key((operation.db_what,
                                          operation.db_objectId)):
                    deleteDict[(operation.db_what,
                                operation.db_objectId)] = operation
#                     del operationDict[(operation.db_what, 
#                                        operation.db_objectId)]
                elif addDict.has_key((operation.db_what,
                                      operation.db_objectId)):
                    del addDict[(operation.db_what,
                                 operation.db_objectId)]
                else:
                    pass
            elif operation.vtType == 'change':
#                 print "chg: %s %s %s" % (operation.db_what, 
#                                          operation.db_oldObjId,
#                                          operation.db_newObjId)
                if operationDict.has_key((operation.db_what,
                                          operation.db_oldObjId)):
                    deleteDict[(operation.db_what,
                                operation.db_oldObjId)] = operation
#                     del operationDict[(operation.db_what, 
#                                        operation.db_oldObjId)]
                elif addDict.has_key((operation.db_what,
                                      operation.db_oldObjId)):
                    del addDict[(operation.db_what, operation.db_oldObjId)]

                addDict[(operation.db_what,
                         operation.db_newObjId)] = operation
            else:
                msg = "Unrecognized operation '%s'" % operation.vtType
                raise Exception(msg)

    return (addDict, deleteDict)

def updateOperationDict(operationDict, deleteOps, addOps):
    for operation in deleteOps:
        if operationDict.has_key((operation.db_what, getOldObjId(operation))):
            del operationDict[(operation.db_what, getOldObjId(operation))]
        else:
            msg = "Illegal operation: " + operation
    for operation in addOps:
        operationDict[(operation.db_what, getNewObjId(operation))] = operation
    return operationDict

def getObjects(actions):
    objects = {}
    for action in actions:
        for operation in action.db_operations:
            if not objects.has_key(operation.db_what):
                objects[operation.db_what] = []
            object = copy.copy(operation.db_data)
            objects[operation.db_what].append(object)
    return objects

def getVersionDifferences(vistrail, versions):
    sharedRoot = getSharedRoot(vistrail, versions)
    sharedActionChain = getActionChain(vistrail, sharedRoot)
    sharedOperationDict = getCurrentOperationDict(sharedActionChain)

    vOnlySorted = []
    for v in versions:
        vActions = getActionChain(vistrail, v, sharedRoot)
        (vAddDict, vDeleteDict) = getOperationDiff(vActions, 
                                                   sharedOperationDict)
        vOnlyAdds = vAddDict.values()
        vOnlyAdds.sort(key=lambda x: x.db_id)
        vOnlyDeletes = vDeleteDict.values()
        vOnlyDeletes.sort(key=lambda x: x.db_id)
        vOpDict = copy.copy(sharedOperationDict)
        updateOperationDict(vOpDict, vOnlyDeletes, vOnlyAdds)
        vOps = vOpDict.values()
        vOps.sort(key=lambda x: x.db_id)
        vOnlySorted.append((vOnlyAdds, vOnlyDeletes, vOps))

    sharedOps = sharedOperationDict.values()
    sharedOps.sort(key=lambda x: x.db_id)

    return (sharedOps, vOnlySorted)

def heuristicModuleMatch(m1, m2):
    """takes two modules and returns 1 if exact match,
    0 if module names match, -1 if no match
    
    """
    if m1.db_name == m2.db_name:
        m1_functions = copy.copy(m1.db_get_functions())
        m2_functions = copy.copy(m2.db_get_functions())
        if len(m1_functions) != len(m2_functions):
            return 0
        for f1 in m1_functions[:]:
            match = None
            for f2 in m2_functions:
                isMatch = heuristicFunctionMatch(f1, f2)
                if isMatch == 1:
                    match = f2
                    break
            if match is not None:
                m1_functions.remove(f1)
                m2_functions.remove(f2)
            else:
                return 0
        if len(m1_functions) == len(m2_functions) == 0:
            return 1
        else:
            return 0
    return -1

def heuristicFunctionMatch(f1, f2):
    """takes two functions and returns 1 if exact match,
    0 if function names match, -1 if no match

    """
    if f1.db_name == f2.db_name:
        f1_parameters = copy.copy(f1.db_get_parameters())
        f2_parameters = copy.copy(f2.db_get_parameters())
        if len(f1_parameters) != len(f2_parameters):
            return 0
        for p1 in f1_parameters[:]:
            match = None
            for p2 in f2_parameters:
                isMatch = heuristicParameterMatch(p1, p2)
                if isMatch == 1:
                    match = p2
                    break
            if match is not None:
                f1_parameters.remove(p1)
                f2_parameters.remove(match)
            else:
                return 0
        if len(f1_parameters) == len(f2_parameters) == 0:
            return 1
        else:
            return 0
    return -1

def heuristicParameterMatch(p1, p2):
    """takes two parameters and returns 1 if exact match,
    0 if partial match (types match), -1 if no match

    """
    if p1.db_type == p2.db_type and p1.db_pos == p2.db_pos:
        if p1.db_val == p2.db_val:
            return 1
        else:
            return 0
    return -1

def heuristicConnectionMatch(c1, c2):
    """takes two connections and returns 1 if exact match,
    0 if partial match (currently undefined), -1 if no match

    """
    c1_ports = copy.copy(c1.db_get_ports())
    c2_ports = copy.copy(c2.db_get_ports())
    for p1 in c1_ports[:]:
        match = None
        for p2 in c2_ports:
            isMatch = heuristicPortMatch(p1, p2)
            if isMatch == 1:
                match = p2
                break
            elif isMatch == 0:
                match = p2
        if match is not None:
            c1_ports.remove(p1)
            c2_ports.remove(match)
        else:
            return -1
    if len(c1_ports) == len(c2_ports) == 0:
        return 1
    return -1

def heuristicPortMatch(p1, p2):
    """takes two ports and returns 1 if exact match,
    0 if partial match, -1 if no match
    
    """
    if p1.db_moduleId == p2.db_moduleId:
        return 1
    elif p1.db_type == p2.db_type and \
            p1.db_moduleName == p2.db_moduleName and \
            p1.sig == p2.sig:
        return 0
    return -1

def function_sig(function):
    return (function.db_name,
            [(param.db_type, param.db_val)
             for param in function.db_get_parameters()])

def getParamChanges(m1, m2, heuristic_match):
    paramChanges = []
    # need to check to see if any children of m1 and m2 are affected
    m1_functions = m1.db_get_functions()
    m2_functions = m2.db_get_functions()
    m1_unmatched = []
    m2_unmatched = []
    for f1 in m1_functions:
        # see if m2 has f1, too
        f2 = m2.db_get_function(f1.db_id)
        if f2 is None:            
            m1_unmatched.append(f1)
        else:
            # function is same, parameters have changed
            paramChanges.append((function_sig(f1), function_sig(f2)))
#             functionMatch = True
#             f1_params = f1.db_get_parameters()
#             f2_params = f2.db_get_parameters()
#             for p1 in f1_params:
#                 if f2.db_get_parameter(p1.db_id) is None:
#                     functionMatch = False
#                     m1_unmatched.append(f1)
#                     break
#             for p2 in f2_params:
#                 if f1.db_get_parameter(p2.db_id) is None:
#                     functionMatch = False
#                     m2_unmatched.append(f2)
#                     break
#             if functionMatch:


    for f2 in m2_functions:
        # see if m1 has f2, too
        if m1.db_get_function(f2.db_id) is None:
            m2_unmatched.append(f2)

    if len(m1_unmatched) + len(m2_unmatched) > 0:
        if heuristic_match and len(m1_unmatched) > 0 and len(m2_unmatched) > 0:
            # do heuristic matches
            for f1 in m1_unmatched[:]:
                matched = False
                for f2 in m2_unmatched:
                    matchValue = heuristicFunctionMatch(f1, f2)
                    if matchValue == 1:
                        # best match so quit
                        matched = f1
                        break
                    elif matchValue == 0:
                        # match, but not exact so continue to look
                        matched = f1
                if matched:
                    paramChanges.append((function_sig(f1), function_sig(f2)))
                    m1_unmatched.remove(f1)
                    m2_unmatched.remove(f2)

        for f in m1_unmatched:
            paramChanges.append((function_sig(f), (None, None)))
        for f in m2_unmatched:
            paramChanges.append(((None, None), function_sig(f)))
        
    return paramChanges

def getOldObjId(operation):
    if operation.vtType == 'change':
        return operation.db_oldObjId
    return operation.db_objectId

def getNewObjId(operation):
    if operation.vtType == 'change':
        return operation.db_newObjId
    return operation.db_objectId

def setOldObjId(operation, id):
    if operation.vtType == 'change':
        operation.db_oldObjId = id
    else:
        operation.db_objectId = id

def setNewObjId(operation, id):
    if operation.vtType == 'change':
        operation.db_newObjId = id
    else:
        operation.db_objectId = id

def getWorkflowDiff(vistrail, v1, v2, heuristic_match=True):
    (sharedOps, vOnlyOps) = \
        getVersionDifferences(vistrail, [v1, v2])

    sharedWorkflow = DBWorkflow()
    performAdds(sharedOps, sharedWorkflow)

    # FIXME better to do additional ops (and do deletes) or do this?
    v1Workflow = DBWorkflow()
    v1Ops = vOnlyOps[0][2]
    performAdds(v1Ops, v1Workflow)

    v2Workflow = DBWorkflow()
    v2Ops = vOnlyOps[1][2]
    performAdds(v2Ops, v2Workflow)

    # FIXME connections do not check their ports
    sharedModuleIds = []
    sharedConnectionIds = []
    sharedFunctionIds = {}
    for op in sharedOps:
        if op.what == 'module' or op.what == 'abstractionRef':
            sharedModuleIds.append(getNewObjId(op))
        elif op.what == 'connection':
            sharedConnectionIds.append(getNewObjId(op))
        elif op.what == 'function':
            sharedFunctionIds[getNewObjId(op)] = op.db_parentObjId
    
    vOnlyModules = []
    vOnlyConnections = []
    paramChgModules = {}
    for (vAdds, vDeletes, _) in vOnlyOps:
        moduleDeleteIds = []
        connectionDeleteIds = []
        for op in vDeletes:
            if op.what == 'module' or op.what == 'abstractionRef':
                moduleDeleteIds.append(getOldObjId(op))
                if getOldObjId(op) in sharedModuleIds:
                    sharedModuleIds.remove(getOldObjId(op))
                if paramChgModules.has_key(getOldObjId(op)):
                    del paramChgModules[getOldObjId(op)]
            elif op.what == 'function' and op.db_parentObjType == 'module' \
                    and op.db_parentObjId in sharedModuleIds:
                # have a function change
                paramChgModules[op.db_parentObjId] = None
                sharedModuleIds.remove(op.db_parentObjId)
            elif op.what == 'parameter' and op.db_parentObjType == 'function' \
                    and sharedFunctionIds.has_key(op.db_parentObjId):
                # have a parameter change
                moduleId = sharedFunctionIds[op.db_parentObjId]
                if moduleId in sharedModuleIds:
                    paramChgModules[moduleId] = None
                    sharedModuleIds.remove(moduleId)
            elif op.what == 'connection':
                connectionDeleteIds.append(getOldObjId(op))
                if getOldObjId(op) in sharedConnectionIds:
                    sharedConnectionIds.remove(getOldObjId(op))

        moduleAddIds = []
        connectionAddIds = []
        for op in vAdds:
            if op.what == 'module' or op.what == 'abstractionRef':
                moduleAddIds.append(getNewObjId(op))
            elif (op.what == 'function' and
                  (op.db_parentObjType == 'module' or
                   op.db_parentObjType == 'abstractionRef') and
                  op.db_parentObjId in sharedModuleIds):
                # have a function change
                paramChgModules[op.db_parentObjId] = None
                sharedModuleIds.remove(op.db_parentObjId)
            elif op.what == 'parameter' and op.db_parentObjType == 'function' \
                    and sharedFunctionIds.has_key(op.db_parentObjId):
                # have a parameter change
                moduleId = sharedFunctionIds[op.db_parentObjId]
                if moduleId in sharedModuleIds:
                    paramChgModules[moduleId] = None
                    sharedModuleIds.remove(moduleId)
            elif op.what == 'connection':
                connectionAddIds.append(getOldObjId(op))

        vOnlyModules.append((moduleAddIds, moduleDeleteIds))
        vOnlyConnections.append((connectionAddIds, connectionDeleteIds))

    sharedModulePairs = [(id, id) for id in sharedModuleIds]
    v1Only = vOnlyModules[0][0]
    v2Only = vOnlyModules[1][0]
    for id in vOnlyModules[1][1]:
        if id not in vOnlyModules[0][1]:
            v1Only.append(id)
    for id in vOnlyModules[0][1]:
        if id not in vOnlyModules[1][1]:
            v2Only.append(id)

    sharedConnectionPairs = [(id, id) for id in sharedConnectionIds]
    c1Only = vOnlyConnections[0][0]
    c2Only = vOnlyConnections[1][0]
    for id in vOnlyConnections[1][1]:
        if id not in vOnlyConnections[0][1]:
            c1Only.append(id)
    for id in vOnlyConnections[0][1]:
        if id not in vOnlyConnections[1][1]:
            c2Only.append(id)

    paramChgModulePairs = [(id, id) for id in paramChgModules.keys()]

    # add heuristic matches
    if heuristic_match:
        # match modules
        for (m1_id, m2_id) in paramChgModulePairs[:]:
            m1 = v1Workflow.db_get_module(m1_id)
            m2 = v2Workflow.db_get_module(m2_id)
            if heuristicModuleMatch(m1, m2) == 1:
                paramChgModulePairs.remove((m1_id, m2_id))
                sharedModulePairs.append((m1_id, m2_id))

        for m1_id in v1Only[:]:
            m1 = v1Workflow.db_get_module(m1_id)
            match = None
            for m2_id in v2Only:
                m2 = v2Workflow.db_get_module(m2_id)
                isMatch = heuristicModuleMatch(m1, m2)
                if isMatch == 1:
                    match = (m1_id, m2_id)
                    break
                elif isMatch == 0:
                    match = (m1_id, m2_id)
            if match is not None:
                if isMatch == 1:
                    v1Only.remove(match[0])
                    v2Only.remove(match[1])
                    sharedModulePairs.append(match)
                else:
                    v1Only.remove(match[0])
                    v2Only.remove(match[1])
                    paramChgModulePairs.append(match)

        # match connections
        for c1_id in c1Only[:]:
            c1 = v1Workflow.db_get_connection(c1_id)
            match = None
            for c2_id in c2Only:
                c2 = v2Workflow.db_get_connection(c2_id)
                isMatch = heuristicConnectionMatch(c1, c2)
                if isMatch == 1:
                    match = (c1_id, c2_id)
                    break
                elif isMatch == 0:
                    match = (c1_id, c2_id)
            if match is not None:
                # don't have port changes yet
                c1Only.remove(match[0])
                c2Only.remove(match[1])
                sharedConnectionPairs.append(match)
                    
    paramChanges = []
    #     print sharedModulePairs
    #     print paramChgModulePairs
    for (m1_id, m2_id) in paramChgModulePairs:
        m1 = v1Workflow.db_get_module(m1_id)
        m2 = v2Workflow.db_get_module(m2_id)
        paramChanges.append(((m1_id, m2_id),
                             getParamChanges(m1, m2, heuristic_match)))

    return (v1Workflow, v2Workflow, 
            sharedModulePairs, v1Only, v2Only, paramChanges,
            sharedConnectionPairs, c1Only, c2Only)

################################################################################

import unittest
import core.system

class TestDBVistrailService(unittest.TestCase):
    def test_parameter_heuristic(self):
        from core.vistrail.module_param import ModuleParam
        
        param1 = ModuleParam(id=0, pos=0, type='String', val='abc')
        param2 = ModuleParam(id=1, pos=0, type='String', val='abc')
        param3 = ModuleParam(id=2, pos=1, type='Float', val='1.0')
        param4 = ModuleParam(id=3, pos=0, type='String', val='def')
        param5 = ModuleParam(id=4, pos=1, type='String', val='abc')

        # test basic equality
        assert heuristicParameterMatch(param1, param2) == 1
        # test basic inequality
        assert heuristicParameterMatch(param1, param3) == -1
        # test partial match
        assert heuristicParameterMatch(param1, param4) == 0
        # test position inequality
        assert heuristicParameterMatch(param1, param5) == -1

    def test_function_heuristic(self):
        from core.vistrail.module_param import ModuleParam
        from core.vistrail.module_function import ModuleFunction
        
        param1 = ModuleParam(id=0, pos=0, type='String', val='abc')
        param2 = ModuleParam(id=1, pos=1, type='Float', val='1.0')
        param3 = ModuleParam(id=2, pos=0, type='String', val='abc')
        param4 = ModuleParam(id=3, pos=1, type='Float', val='1.0')
        param5 = ModuleParam(id=4, pos=0, type='String', val='abc')
        param6 = ModuleParam(id=5, pos=1, type='Float', val='2.0')

        function1 = ModuleFunction(name='f1', parameters=[param1, param2])
        function2 = ModuleFunction(name='f1', parameters=[param3, param4])
        function3 = ModuleFunction(name='f1', parameters=[param5, param6])
        function4 = ModuleFunction(name='f2', parameters=[param1, param2])
        function5 = ModuleFunction(name='f1', parameters=[param1])

        # test basic equality
        assert heuristicFunctionMatch(function1, function2) == 1
        # test partial match
        assert heuristicFunctionMatch(function1, function3) == 0
        # test basic inequality
        assert heuristicFunctionMatch(function1, function4) == -1
        # test length inequality
        assert heuristicFunctionMatch(function1, function5) == 0

    def test_module_heuristic(self):
        from core.vistrail.module_param import ModuleParam
        from core.vistrail.module_function import ModuleFunction
        from core.vistrail.module import Module

        param1 = ModuleParam(id=0, pos=0, type='String', val='abc')
        param2 = ModuleParam(id=1, pos=1, type='Float', val='1.0')
        param3 = ModuleParam(id=2, pos=0, type='String', val='abc')
        param4 = ModuleParam(id=3, pos=1, type='Float', val='1.0')
        param5 = ModuleParam(id=4, pos=0, type='Integer', val='2')
        param6 = ModuleParam(id=5, pos=0, type='Integer', val='2')

        function1 = ModuleFunction(name='f1', parameters=[param1, param2])
        function2 = ModuleFunction(name='f1', parameters=[param3, param4])
        function3 = ModuleFunction(name='f2', parameters=[param5])
        function4 = ModuleFunction(name='f2', parameters=[param6])
        function5 = ModuleFunction(name='f1', parameters=[param2, param4])
        function6 = ModuleFunction(name='f2', parameters=[param5])

        module1 = Module(name='m1', functions=[function1, function3])
        module2 = Module(name='m1', functions=[function2, function4])
        module3 = Module(name='m2', functions=[function1, function2])
        module4 = Module(name='m1', functions=[function5])
        module5 = Module(name='m1', functions=[function5, function6])

        # test basic equality
        assert heuristicModuleMatch(module1, module2) == 1
        # test basic inequality
        assert heuristicModuleMatch(module1, module3) == -1
        # test length inequality
        assert heuristicModuleMatch(module1, module4) == 0
        # test parameter change inequality
        assert heuristicModuleMatch(module1, module5) == 0

if __name__ == '__main__':
    unittest.main()
