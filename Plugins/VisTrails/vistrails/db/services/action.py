
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

from db.domain import DBAction, DBAdd, DBDelete, DBChange
import copy

def create_delete_op_chain(object, parent=(None, None)):
    opChain = []
    for (obj, parentType, parentId) in object.db_children(parent):
        op = DBDelete(id=-1,
                      what=obj.vtType,
                      objectId=obj.db_id,
                      parentObjType=parentType,
                      parentObjId=parentId,
                      )
        opChain.append(op)
    return opChain

def create_add_op_chain(object, parent=(None, None)):
    opChain = []
    object = copy.copy(object)
    adds = object.db_children(parent, True)
    adds.reverse()
    for (obj, parentType, parentId) in adds:
        op = DBAdd(id=-1,
                   what=obj.vtType,
                   objectId=obj.db_id,
                   parentObjType=parentType,
                   parentObjId=parentId,
                   data=obj,
                   )
        opChain.append(op)
    return opChain

def create_change_op_chain(old_obj, new_obj, parent=(None,None)):
    opChain = []
    new_obj = copy.copy(new_obj)
    deletes = old_obj.db_children(parent)
    deletes.pop()
    for (obj, parentType, parentId) in deletes:
        op = DBDelete(id=-1,
                      what=obj.vtType,
                      objectId=obj.db_id,
                      parentObjType=parentType,
                      parentObjId=parentId,
                      )
        opChain.append(op)

    adds = new_obj.db_children(parent, True)
    (obj, parentType, parentId) = adds.pop()
    op = DBChange(id=-1,
                  what=obj.vtType,
                  oldObjId=old_obj.db_id,
                  newObjId=obj.db_id,
                  parentObjType=parentType,
                  parentObjId=parentId,
                  data=new_obj,
                  )
    opChain.append(op)

    adds.reverse()
    for (obj, parentType, parentId) in adds:
        op = DBAdd(id=-1,
                   what=obj.vtType,
                   objectId=obj.db_id,
                   parentObjType=parentType,
                   parentObjId=parentId,
                   data=obj,
                   )
    return opChain

def create_copy_op_chain(object, parent=(None,None), id_scope=None):
    opChain = []
    id_remap = {}
    object = copy.copy(object)

    adds = object.db_children(parent, True)
    adds.reverse()
    for (obj, parentType, parentId) in adds:
        if parentId is not None:
            parentId = id_remap[(parentType, parentId)]
        new_id = id_scope.getNewId(obj.vtType)
        id_remap[(obj.vtType, obj.db_id)] = new_id
        obj.db_id = new_id
        op = DBAdd(id=-1,
                   what=obj.vtType,
                   objectId=obj.db_id,
                   parentObjType=parentType,
                   parentObjId=parentId,
                   data=obj,
                   )
        opChain.append(op)
    return opChain
    

def create_action(action_list):
    """create_action(action_list: list) -> DBAction
    where action_list is a list of tuples
     (
      type, 
      object, 
      parent_type=None,
      parent_id=None,
      new_obj=None
     )
    and the method returns an action that accomplishes all of the operations.

    Example: create_action([('add', module1), ('delete', connection2)]

    """
    ops = []
    for tuple in action_list:
        if tuple[0] == 'add' and len(tuple) >= 2:
            if len(tuple) >= 4:
                ops.extend(create_add_op_chain(tuple[1], (tuple[2], tuple[3])))
            else:
                ops.extend(create_add_op_chain(tuple[1]))
        elif tuple[0] == 'delete' and len(tuple) >= 2:
            if len(tuple) >= 4:
                ops.extend(create_delete_op_chain(tuple[1], 
                                                  (tuple[2], tuple[3])))
            else:
                ops.extend(create_delete_op_chain(tuple[1]))
        elif tuple[0] == 'change' and len(tuple) >= 3:
            if len(tuple) >= 5:
                ops.extend(create_change_op_chain(tuple[1], tuple[2],
                                                  (tuple[3], tuple[4])))
            else:
                ops.extend(create_change_op_chain(tuple[1], tuple[2]))
        else:
            msg = "unable to interpret action tuple " + tuple.__str__()
            raise Exception(msg)
    action = DBAction(id=-1,
                      operations=ops)
    return action

def create_action_from_ops(ops):
    action = DBAction(id=-1,
                      operations=ops)
    return action
