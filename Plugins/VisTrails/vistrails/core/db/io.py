
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

from core.vistrail.action import Action
from core.vistrail.operation import AddOp, ChangeOp, DeleteOp
import db.services.io
import db.services.vistrail
import db.services.action
from xml.dom.minidom import parse, getDOMImplementation

def get_db_vistrail_list(config):
    return db.services.io.get_db_vistrail_list(config)

def get_workflow(vt, version):
    from core.vistrail.pipeline import Pipeline
    workflow = db.services.vistrail.materializeWorkflow(vt, version)
    Pipeline.convert(workflow)
    return workflow

def expand_workflow(vt, workflow=None, version=None):
    from core.vistrail.pipeline import Pipeline
    (workflow, module_remap) = \
        db.services.vistrail.expandWorkflow(vt, workflow, version)
    Pipeline.convert(workflow)
    return (workflow, module_remap)

def open_workflow(filename):
    from core.vistrail.pipeline import Pipeline
    workflow = db.services.io.open_workflow_from_xml(filename)
    Pipeline.convert(workflow)
    return workflow

def save_workflow(workflow, filename):
    db.services.io.save_workflow_to_xml(workflow, filename)

def unserialize(str, klass):
    """returns VisTrails entity given an XML serialization

    """
    obj = db.services.io.unserialize(str, klass.vtType)
    if obj:
        #maybe we should also put a try except here
        klass.convert(obj)
    return obj

def serialize(object):
    """returns XML serialization for any VisTrails entity

    """
    return db.services.io.serialize(object)

def get_workflow_diff(vt, v1, v2):
    from core.vistrail.pipeline import Pipeline
    (v1, v2, pairs, v1Only, v2Only, paramChanges, _, _, _) = \
        db.services.vistrail.getWorkflowDiff(vt, v1, v2, True)
    Pipeline.convert(v1)
    v1.set_abstraction_map(vt.abstractionMap)
    Pipeline.convert(v2)
    v2.set_abstraction_map(vt.abstractionMap)
    #     print 'pairs:', pairs
    #     print 'v1Only:', v1Only
    #     print 'v2Only:', v2Only
    #     print 'paramChanges:', paramChanges
    return (v1, v2, pairs, v1Only, v2Only, paramChanges)

def get_workflow_diff_with_connections(vt, v1, v2):
    from core.vistrail.pipeline import Pipeline
    (v1, v2, mPairs, v1Only, v2Only, paramChanges, cPairs, c1Only, c2Only) = \
        db.services.vistrail.getWorkflowDiff(vt, v1, v2, False)
    Pipeline.convert(v1)
    v1.set_abstraction_map(vt.abstractionMap)
    Pipeline.convert(v2)
    v2.set_abstraction_map(vt.abstractionMap)
    #     print 'mPairs:', mPairs
    #     print 'v1Only:', v1Only
    #     print 'v2Only:', v2Only
    #     print 'paramChanges:', paramChanges
    #     print 'cPairs:', cPairs
    #     print 'c1Only:', c1Only
    #     print 'c2Only:', c2Only
    return (v1, v2, pairs, v1Only, v2Only, paramChanges,
            cPairs, c1Only, c2Only)

def getPathAsAction(vt, v1, v2):
    a = db.services.vistrail.getPathAsAction(vt, v1, v2)
    Action.convert(a)
    return a

def fixActions(vt, v, actions):
    return db.services.vistrail.fixActions(vt, v, actions)

def convert_operation_list(ops):
    for op in ops:
        if op.vtType == 'add':
            AddOp.convert(op)
        elif op.vtType == 'change':
            ChangeOp.convert(op)
        elif op.vtType == 'delete':
            DeleteOp.convert(op)
        else:
            raise Exception("Unknown operation type '%s'" % op.vtType)

def create_action(action_list):
    """create_action(action_list: list) -> Action
    where action_list is a list of tuples
     (
      type, 
      object, 
      parent_type=None,
      parent_id=None,
      new_obj=None
     )
    and the method returns a *single* action that accomplishes all 
    of the operations.

    Examples: create_action([('add', module1), ('delete', connection2)]
              create_action([('add', param1, 'function', function1),
                             ('change', func3, 'module', module1, func2)])
    Note that create_action([('add', module)]) adds a module and *all* of its
    children.
    """
    action = db.services.action.create_action(action_list)
    Action.convert(action)
    return action
    
def create_add_op_chain(object, parent=(None, None)):
    """create_add_op_chain(object: object, 
                           parent=(type : str, id : long)) -> [op]
    where [op] is a list of operations to add the given object and its
    children to a workflow.
    """
    ops = db.services.action.create_add_op_chain(object, parent)
    convert_operation_list(ops)
    return ops

def create_change_op_chain(old_obj, new_obj, parent=(None,None)):
    """create_change_op_chain(old_obj: object, new_obj: object, 
                              parent=(type : str, id : long)) -> [op]
    where [op] is a list of operations to change the given object and its
    children to the new object in a workflow.
    """
    ops = db.services.action.create_change_op_chain(old_obj, new_obj, parent)
    convert_operation_list(ops)
    return ops

def create_delete_op_chain(object, parent=(None, None)):
    """create_delete_op_chain(object: object, 
                              parent=(type : str, id : long)) -> [op]
    where [op] is a list of operations to delete the given object and its
    children from a workflow.
    """
    ops = db.services.action.create_delete_op_chain(object, parent)
    convert_operation_list(ops)
    return ops
