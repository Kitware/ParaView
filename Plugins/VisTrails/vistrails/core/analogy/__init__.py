
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

from core.data_structures.bijectivedict import Bidict
from itertools import imap, chain
import core.modules.module_registry
import core.db.io
from core.requirements import MissingRequirement
from core.vistrail.port import PortEndPoint
from core.vistrail.module import Module
import copy
from core.vistrail.pipeline import Pipeline
reg = core.modules.module_registry.registry

##########################################################################

from eigen import *

_debug = True

def perform_analogy_on_vistrail(vistrail,
                                version_a, version_b,
                                version_c, alpha=0.85):
    """perform_analogy(vistrail, version_a, version_b, version_c,
                       alpha=0.15): action
    Creates a new action version_d to the vistrail such that the difference
    between a and b is the same as between c and d, and returns this
    action."""

    ############################################################################
    # STEP 1: find mapping from a to c

    #     pipeline_c = Pipeline(vistrail.actionChain(version_c))
    #     pipeline_a = Pipeline(vistrail.actionChain(version_a))

    if _debug:
        print 'version_a:', version_a
        print 'version_b:', version_b
        print 'version_c:', version_c

    pipeline_a = core.db.io.get_workflow(vistrail, version_a)
    pipeline_c = core.db.io.get_workflow(vistrail, version_c)
    
    e = EigenPipelineSimilarity2(pipeline_a, pipeline_c, alpha=alpha)
    e._debug = _debug

    (input_module_remap,
     output_module_remap,
     combined_module_remap) = e.solve()

    if _debug:
        print 'Input remap'
        print input_module_remap
        print 'Output remap'
        print output_module_remap
        print 'Combined remap'
        print combined_module_remap

    module_remap = combined_module_remap
    if _debug:
        print "Computing names..."
        
    def name_remap(d):
        return dict([(from_id,
                      pipeline_c.modules[to_id].name)
                     for (from_id, to_id)
                     in d.iteritems()])

    module_name_remap = name_remap(module_remap)
    input_module_name_remap = name_remap(input_module_remap)
    output_module_name_remap = name_remap(output_module_remap)

    # find connection remap
    connection_remap = {}
    for a_connect in pipeline_a.connections.itervalues():
        # FIXME assumes that all connections have both source and dest
        a_source = a_connect.source.moduleId
        a_dest = a_connect.destination.moduleId
        match = None
        for c_connect in pipeline_c.connections.itervalues():
            if (output_module_remap[a_source] == c_connect.source.moduleId and 
                input_module_remap[a_dest] == c_connect.destination.moduleId):
                match = c_connect
                if (a_connect.source.sig == c_connect.source.sig and 
                    a_connect.destination.sig == c_connect.destination.sig):
                    break
        if match is not None:
            connection_remap[a_connect.id] = c_connect.id

    # find function remap

    # construct total remap
    id_remap = {}
    for (a_id, c_id) in module_remap.iteritems():
        id_remap[('module', a_id)] = c_id
    for (a_id, c_id) in connection_remap.iteritems():
        id_remap[('connection', a_id)] = c_id

    ############################################################################
    # STEP 2: find actions to be remapped (b-a)

    # this creates a new action with new operations
    baAction = core.db.io.getPathAsAction(vistrail, version_a, version_b)

    for operation in baAction.operations:
        print "ba_op0:", operation.id,  operation.vtType, operation.what, 
        print operation.objectId, "to", operation.parentObjType,
        print operation.parentObjId

    ############################################################################
    # STEP 3: remap (b-a) using mapping in STEP 1 so it can be applied to c

    # for all module references, update the module ids according to the remap
    # need to consider modules, parent_obj_ids, ports
    # if things don't make sense, they're cut out in STEP 4, not here

    ops = []
    for op in baAction.operations:
        if op.vtType == 'delete':
            if op.what == 'module':
                if module_remap.has_key(op.old_obj_id):
                    module = pipeline_c.modules[module_remap[op.old_obj_id]]
                    ops.extend(core.db.io.create_delete_op_chain(module))
                else:
                    ops.append(op)
            elif op.what == 'connection':
                if connection_remap.has_key(op.old_obj_id):
                    conn = pipeline_c.connections[connection_remap[ \
                            op.old_obj_id]]
                    ops.extend(core.db.io.create_delete_op_chain(conn))
                else:
                    ops.append(op)
            else:
                ops.append(op)
        elif op.vtType == 'add' or op.vtType == 'change':
            old_id = op.objectId
            new_id = vistrail.idScope.getNewId(op.what)
            op.objectId = new_id
            op.data.db_id = new_id
            id_remap[(op.what, old_id)] = new_id
            if op.what == 'module':
                module_name_remap[old_id] = op.data.name

            if op.parentObjId is not None and \
                    id_remap.has_key((op.parentObjType, 
                                      op.parentObjId)):
                op.parentObjId = id_remap[(op.parentObjType,
                                           op.parentObjId)]
            if op.what == 'port':
                port = op.data
                if id_remap.has_key(('module', port.moduleId)):
                    if port.type == 'source':
                        try:
                            port.moduleName = output_module_name_remap[port.moduleId]
                            port.moduleId = output_module_remap[port.moduleId]
                            m = pipeline_c.modules[port.moduleId]
                            d = reg.get_descriptor_by_name(m.package, m.name)
                            def remap():
                                pspec = core.modules.module_registry.PortSpec.from_sigstring(port.db_spec)
                                all_ports = reg.all_source_ports(d)
                                # print "pspec", pspec
                                # First try to find a perfect match
                                for (klass_name, ports) in all_ports:
                                    for candidate_port in ports:
                                        if (candidate_port.spec.type_equals(pspec) and
                                            candidate_port.name == port.name):
                                            #print "found perfect match"
                                            return True
                                # Now try to find an imperfect one
                                for (klass_name, ports) in all_ports:
                                    for candidate_port in ports:
                                        print candidate_port.spec
                                        if candidate_port.spec.type_equals(pspec):
                                            port.name = candidate_port.name
                                            #print "found imperfect match"
                                            return True
                                return False
                            if not remap():
                                print "COULD NOT FIND source MATCH!!!"
                        except KeyError:
                            # This happens when the module was added as part of the analogy
                            port.moduleName = module_name_remap[port.moduleId]
                            port.moduleId = id_remap[('module', port.moduleId)]
                    elif port.type == 'destination':
                        try:
                            port.moduleName = input_module_name_remap[port.moduleId]
                            port.moduleId = input_module_remap[port.moduleId]
                            m = pipeline_c.modules[port.moduleId]
                            d = reg.get_descriptor_by_name(m.package, m.name)
                            def remap():
                                pspec = core.modules.module_registry.PortSpec.from_sigstring(port.db_spec)
#                                 print "This is the spec", port.spec, port.db_spec
                                all_ports = reg.all_destination_ports(d)
                                # First try to find a perfect match
                                for (klass_name, ports) in all_ports:
                                    for candidate_port in ports:
                                        if (candidate_port.spec.type_equals(pspec) and
                                            candidate_port.name == port.name):
                                            # print "found perfect match"
                                            return True
                                # Now try to find an imperfect one
                                for (klass_name, ports) in all_ports:
                                    for candidate_port in ports:
                                        if candidate_port.spec.type_equals(pspec):
                                            port.name = candidate_port.name
                                            # print "found imperfect match"
                                            return True
                                return False
                            if not remap():
                                print "COULD NOT FIND destination MATCH!!!"
                            remap()
                        except KeyError:
                            # This happens when the module was added as part of the analogy
                            port.moduleName = module_name_remap[port.moduleId]
                            port.moduleId = id_remap[('module', port.moduleId)]
                        
            ops.append(op)
    baAction.operations = ops
    # baAction should now have remapped everything

    ############################################################################
    # STEP 4: apply remapped (b-a) to c

    # some actions cannot be applied because they reference stuff that
    # isn't in c.  need to no-op for them
    
    # want to take the pipeline for c and simply apply (b-a), but return
    # False for operations that don't make sense.
    
    # get operationDict for c, do update with baAction but discard ops
    # that don't make sense

    for operation in baAction.operations:
        print "ba_op1:", operation.id, operation.vtType, operation.what, 
        print operation.objectId, "to", operation.parentObjType,
        print operation.parentObjId

    baAction.prevId = version_c
    core.db.io.fixActions(vistrail, version_c, [baAction])
    print "got here"
    for operation in baAction.operations:
        print "ba_op2:", operation.id, operation.vtType, operation.what, 
        print operation.objectId, "to", operation.parentObjType,
        print operation.parentObjId
    # this will be taken care by the controller
    #vistrail.add_action(baAction, version_c)
    return baAction
