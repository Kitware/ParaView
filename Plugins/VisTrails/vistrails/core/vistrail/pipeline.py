
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
##TODO Tests
""" This module defines the class Pipeline """

from core.cache.hasher import Hasher
from core.data_structures.bijectivedict import Bidict
from core.data_structures.graph import Graph
from core.debug import DebugPrint
from core.modules.module_registry import registry, ModuleRegistry
from core.utils import VistrailsInternalError
from core.utils import expression, append_to_dict_of_lists
from core.utils.uxml import named_elements
from core.vistrail.abstraction import Abstraction
from core.vistrail.abstraction_module import AbstractionModule
from core.vistrail.connection import Connection
from core.vistrail.group import Group
from core.vistrail.module import Module
from core.vistrail.module_function import ModuleFunction
from core.vistrail.module_param import ModuleParam
from core.vistrail.plugin_data import PluginData
from core.vistrail.port import Port, PortEndPoint
from core.vistrail.port_spec import PortSpec
from db.domain import DBWorkflow
from types import ListType
import core.vistrail.action
import core.modules.module_registry
from core.utils import profile

from xml.dom.minidom import getDOMImplementation, parseString
import copy
import sha

##############################################################################

class Pipeline(DBWorkflow):
    """ A Pipeline is a set of modules and connections between them. """
    
    def __init__(self, *args, **kwargs):
        """ __init__() -> Pipelines
        Initializes modules, connections and graph.

        """
        self.clear()

  DBWorkflow.__init__(self, *args, **kwargs)
        if self.id is None:
            self.id = 0
        if self.name is None:
            self.name = 'untitled'
        for module in self.module_list:
            if module.vtType == Module.vtType:
                Module.convert(module)
            elif module.vtType == AbstractionModule.vtType:
                AbstractionModule.convert(module)
            elif module.vtType == Group.vtType:
                Group.convert(module)
            self.graph.add_vertex(module.id)
        for connection in self.connection_list:
            self.graph.add_edge(connection.source.moduleId,
                                connection.destination.moduleId,
                                connection.id)
        self.abstraction_map = {}
            
    def __copy__(self):
        """ __copy__() -> Pipeline - Returns a clone of itself """ 
        return Pipeline.do_copy(self)

    def do_copy(self, new_ids=False, id_scope=None, id_remap=None):
        cp = DBWorkflow.do_copy(self, new_ids, id_scope, id_remap)
        cp.__class__ = Pipeline
        if id_remap:
            vertex_map = dict((old, new) for ((type, old), new) in id_remap.iteritems()
                              if type == 'module')
            edge_map = dict((old, new) for ((type, old), new) in id_remap.iteritems()
                              if type == 'connection')
            cp.graph = Graph.map_vertices(self.graph, vertex_map, edge_map)
        else:
            cp.graph = copy.copy(self.graph)
        cp.aliases = Bidict([(k,copy.copy(v))
                           for (k,v)
                           in self.aliases.iteritems()])
        cp._connection_signatures = Bidict([(k,copy.copy(v))
                                            for (k,v)
                                            in self._connection_signatures.iteritems()])
        cp._subpipeline_signatures = Bidict([(k,copy.copy(v))
                                             for (k,v)
                                             in self._subpipeline_signatures.iteritems()])
        cp._module_signatures = Bidict([(k,copy.copy(v))
                                        for (k,v)
                                        in self._module_signatures.iteritems()])
        cp.abstraction_map = self.abstraction_map
        return cp

    @staticmethod
    def convert(_workflow):
        if _workflow.__class__ == Pipeline:
            return
        # do clear plus get the modules and connections
  _workflow.__class__ = Pipeline
        _workflow.graph = Graph()
        _workflow.aliases = Bidict()
        _workflow._subpipeline_signatures = Bidict()
        _workflow._module_signatures = Bidict()
        _workflow._connection_signatures = Bidict()
  for _module in _workflow.db_modules:
            if _module.vtType == Module.vtType:
                Module.convert(_module)
            elif _module.vtType == AbstractionModule.vtType:
                AbstractionModule.convert(_module)
            elif _module.vtType == Group.vtType:
                Group.convert(_module)
            _workflow.graph.add_vertex(_module.id)
  for _connection in _workflow.db_connections:
            Connection.convert(_connection)
            _workflow.graph.add_edge( \
                _connection.source.moduleId,
                _connection.destination.moduleId,
                _connection.db_id)
        for _abstraction in _workflow.db_abstractions:
            Abstraction.convert(_abstraction)
        for _plugin_data in _workflow.db_plugin_datas:
            PluginData.convert(_plugin_data)
        #there should be another way to do this
        for _obj in _workflow.objects.itervalues():
            if _obj.vtType == 'function':
                for _par in _obj.parameters:
                    _workflow.change_alias(_par.alias,
                                           _par.vtType,
                                           _par.real_id,
                                           _obj.vtType,
                                           _obj.real_id)
        _workflow.abstraction_map = {}

    ##########################################################################

    def find_method(self, module_id, parameter_name):
        """find_method(module_id, parameter_name) -> int.

        Finds the function_id for a given method name.
        Returns -1 if method name is not there.

        WARNING: Might not work for overloaded methods (where types
        also matter)
        """
        try:
            return [f.name
                    for f
                    in self.get_module_by_id(module_id).functions].index(parameter_name)
        except ValueError:
            return -1

    ##########################################################################
    # Properties

    id = DBWorkflow.db_id
    name = DBWorkflow.db_name
    plugin_datas = DBWorkflow.db_plugin_datas

    def _get_modules(self):
        return self.db_modules_id_index
    modules = property(_get_modules)
    def _get_module_list(self):
        return self.db_modules
    module_list = property(_get_module_list)

    def _get_connections(self):
        return self.db_connections_id_index
    connections = property(_get_connections)
    def _get_connection_list(self):
        return self.db_connections
    connection_list = property(_get_connection_list)

    def set_abstraction_map(self, map):
        self.abstraction_map = map
        for module in self.module_list:
            if module.vtType == AbstractionModule.vtType:
                module.abstraction = self.abstraction_map[module.abstraction_id]

    # ONLY FOR COPY/PASTE...
    def add_abstraction(self, abstraction):
        """only used for copy/paste--abstractions live on the vistrail"""
        self.db_add_abstraction(abstraction)
    def get_abstractions(self):
        """only used for copy/paste--abstractions live on the vistrail"""
        return self.db_abstractions

    def clear(self):
        """clear() -> None. Erases pipeline contents."""
        if hasattr(self, 'db_modules'):
            for module in self.db_modules:
                self.db_delete_module(module)
        if hasattr(self, 'db_connections'):
            for connection in self.db_connections:
                self.db_delete_connection(connection)
        self.graph = Graph()
        self.aliases = Bidict()
        self._subpipeline_signatures = Bidict()
        self._module_signatures = Bidict()
        self._connection_signatures = Bidict()

    def get_tmp_id(self, type):
        """get_tmp_id(type: str) -> long
        returns a temporary id for a workflow item.  Use the idScope on the
        vistrail for permanent ids.
        """

        return -self.tmp_id.getNewId(type)

    def fresh_module_id(self):
        return self.get_tmp_id(Module.vtType)
    def fresh_connection_id(self):
        return self.get_tmp_id(Connection.vtType)

    def check_connection(self, c):
        """check_connection(c: Connection) -> boolean 
        Checks semantics of connection
          
        """
        if c.source.endPoint != Port.SourceEndPoint:
            return False
        if c.destination.endPoint != Port.DestinationEndPoint:
            return False
        if not self.has_module_with_id(c.sourceId):
            return False
        if not self.has_module_with_id(c.destinationId):
            return False
        if c.source.type != c.destination.type:
            return False
        return True
    
    def connects_at_port(self, p):
        """ connects_at_port(p: Port) -> list of Connection 
        Returns a list of Connections that connect at port p
        
        """
        result = []
        if p.endPoint == Port.DestinationEndPoint:
            el = self.graph.edges_to(p.moduleId)
            for (edgeto, edgeid) in el:
                dest = self.connection[edgeid].destination
                if VTKRTTI().intrinsicPortEqual(dest, p):
                    result.append(self.connection[edgeid])
        elif p.endPoint == Port.SourceEndPoint:
            el = self.graph.edges_from(p.moduleId)
            for (edgeto, edgeid) in el:
                source = self.connection[edgeid].source
                if VTKRTTI().intrinsicPortEqual(source, p):
                    result.append(self.connection[edgeid])
        else:
            raise VistrailsInternalError("port with bogus information")
        return result
    
    def perform_action_chain(self, actionChain):
        # BEWARE: if actionChain is long, you're probably better off
        # going through general_action_chain, because it optimizes
        # away unnecessary operations.
        for action in actionChain: 
            self.perform_action(action)

    def perform_action(self, action):
        for operation in action.operations:
            self.perform_operation(operation)

    def perform_operation_chain(self, opChain):
        for op in opChain:
            self.perform_operation(op)

    def perform_operation(self, op):
        # print "doing %s %s" % (op.vtType, op.what)
        if op.db_what == 'abstractionRef' or op.db_what == 'group':
            what = 'module'
        else:
            what = op.db_what
        funname = '%s_%s' % (op.vtType, what)

        try:
            f = getattr(self, funname)
            if op.vtType == 'add':
                f(op.data, op.parentObjId, op.parentObjType)
            elif op.vtType == 'delete':
                f(op.objectId, op.parentObjId, op.parentObjType)
            elif op.vtType == 'change':
                f(op.oldObjId, op.data, op.parentObjId, op.parentObjType)
        except AttributeError:
            db_funname = 'db_%s_object' % op.vtType
            try:
                f = getattr(self, db_funname)
                if op.vtType == 'add':
                    f(op.data, op.parentObjType, op.parentObjId)
                elif op.vtType == 'delete':
                    f(op.objectId, op.what, op.parentObjType, op.parentObjId)
                elif op.vtType == 'change':
                    f(op.oldObjId, op.data, op.parentObjType, op.parentObjId)
            except AttributeError:
                msg = "Pipeline cannot execute '%s' operation" % op.vtType
                raise VistrailsInternalError(msg)

    def add_module(self, m, *args):
        """add_module(m: Module) -> None 
        Add new module to pipeline
          
        """
        if self.has_module_with_id(m.id):
            raise VistrailsInternalError("duplicate module id")
#         self.modules[m.id] = copy.copy(m)
        if m.vtType == AbstractionModule.vtType:
            m.abstraction = self.abstraction_map[m.abstraction_id]
        self.db_add_object(m)
        self.graph.add_vertex(m.id)

    def change_module(self, old_id, m, *args):
        if not self.has_module_with_id(old_id):
            raise VistrailsInternalError("module %s doesn't exist" % old_id)
        self.db_change_object(old_id, m)
        self.graph.delete_vertex(old_id)
        self.graph.add_vertex(m.id)

    def delete_module(self, id, *args):
        """delete_module(id:int) -> None 
        Delete a module from pipeline given an id.

        """
        if not self.has_module_with_id(id):
            raise VistrailsInternalError("id missing in modules")

        # we're hiding the necessary operations by doing this!
        for (_, conn_id) in self.graph.adjacency_list[id][:]:
            self.delete_connection(conn_id)
        for (_, conn_id) in self.graph.inverse_adjacency_list[id][:]:
            self.delete_connection(conn_id)

        # self.modules.pop(id)
        self.db_delete_object(id, Module.vtType)
        self.graph.delete_vertex(id)
        if id in self._module_signatures:
            del self._module_signatures[id]
        if id in self._subpipeline_signatures:
            del self._subpipeline_signatures[id]

    def add_connection(self, c, *args):
        """add_connection(c: Connection) -> None 
        Add new connection to pipeline.
          
        """
        if self.has_connection_with_id(c.id):
            raise VistrailsInternalError("duplicate connection id " + str(c.id))
#         self.connections[c.id] = copy.copy(c)
        self.db_add_object(c)
        if c.source is not None and c.destination is not None:
            assert(c.sourceId != c.destinationId)        
            self.graph.add_edge(c.sourceId, c.destinationId, c.id)
            self.ensure_connection_specs([c.id])

    def change_connection(self, old_id, c, *args):
        """change_connection(old_id: long, c: Connection) -> None
        Deletes connection identified by old_id and adds connection c

        """
        if not self.has_connection_with_id(old_id):
            raise VistrailsInternalError("connection %s doesn't exist" % old_id)

        old_conn = self.connections[old_id]
        if old_conn.source is not None and old_conn.destination is not None:
            self.graph.delete_edge(old_conn.sourceId, old_conn.destinationId,
                                   old_conn.id)
        if old_id in self._connection_signatures:
            del self._connection_signatures[old_id]
        self.db_change_object(old_id, c)        
        if c.source is not None and c.destination is not None:
            assert(c.sourceId != c.destinationId)
            self.graph.add_edge(c.sourceId, c.destinationId, c.id)
            self.ensure_connection_specs([c.id])

    def delete_connection(self, id, *args):
        """ delete_connection(id:int) -> None 
        Delete connection identified by id from pipeline.
           
        """

        if not self.has_connection_with_id(id):
            raise VistrailsInternalError("id %s missing in connections" % id)
        conn = self.connections[id]
        # self.connections.pop(id)
        self.db_delete_object(id, 'connection')
        if conn.source is not None and conn.destination is not None and \
                (conn.destinationId, conn.id) in \
                self.graph.edges_from(conn.sourceId):
            self.graph.delete_edge(conn.sourceId, conn.destinationId, conn.id)
        if id in self._connection_signatures:
            del self._connection_signatures[id]
        
    def add_parameter(self, param, parent_id, 
                      parent_type=ModuleFunction.vtType):
        self.db_add_object(param, parent_type, parent_id)
        if not self.has_alias(param.alias):
            self.change_alias(param.alias, 
                              param.vtType, 
                              param.real_id,
                              parent_type,
                              parent_id)

    def delete_parameter(self, param_id, parent_id, 
                         parent_type=ModuleFunction.vtType):
        self.db_delete_object(param_id, ModuleParam.vtType,
                              parent_type, parent_id)
        self.remove_alias(ModuleParam.vtType, param_id, parent_type, parent_id)

    def change_parameter(self, old_param_id, param, parent_id, 
                         parent_type=ModuleFunction.vtType):
        self.remove_alias(ModuleParam.vtType, old_param_id, 
                          parent_type, parent_id)
        self.db_change_object(old_param_id, param,
                              parent_type, parent_id)
        if not self.has_alias(param.alias):
            self.change_alias(param.alias, 
                              param.vtType, 
                              param.real_id,
                              parent_type,
                              parent_id)

    def add_port(self, port, parent_id, parent_type=Connection.vtType):
        self.db_add_object(port, parent_type, parent_id)
        connection = self.connections[parent_id]
        if connection.source is not None and \
                connection.destination is not None:
            self.graph.add_edge(connection.sourceId, 
                                connection.destinationId, 
                                connection.id)

    def delete_port(self, port_id, parent_id, parent_type=Connection.vtType):
        connection = self.connections[parent_id]
        if len(connection.ports) >= 2:
            self.graph.delete_edge(connection.sourceId, 
                                   connection.destinationId, 
                                   connection.id)
        self.db_delete_object(port_id, Port.vtType, parent_type, parent_id)
        
    def change_port(self, old_port_id, port, parent_id,
                    parent_type=Connection.vtType):
        connection = self.connections[parent_id]
        if len(connection.ports) >= 2:
            source_list = self.graph.adjacency_list[connection.sourceId]
            source_list.remove((connection.destinationId, connection.id))
            dest_list = \
                self.graph.inverse_adjacency_list[connection.destinationId]
            dest_list.remove((connection.sourceId, connection.id))
        self.db_change_object(old_port_id, port, parent_type, parent_id)
        if len(connection.ports) >= 2:
            source_list = self.graph.adjacency_list[connection.sourceId]
            source_list.append((connection.destinationId, connection.id))
            dest_list = \
                self.graph.inverse_adjacency_list[connection.destinationId]
            dest_list.append((connection.sourceId, connection.id))

    def add_port_to_registry(self, portSpec, moduleId):
        m = self.get_module_by_id(moduleId)
        m.add_port_to_registry(portSpec)

    def add_portSpec(self, port_spec, parent_id, parent_type=Module.vtType):
        self.db_add_object(port_spec, parent_type, parent_id)
        self.add_port_to_registry(port_spec, parent_id)
        
    def delete_port_from_registry(self, id, moduleId):
        m = self.get_module_by_id(moduleId)
        m.delete_port_from_registry(id)

    def delete_portSpec(self, spec_id, parent_id, parent_type=Module.vtType):
        self.delete_port_from_registry(spec_id, parent_id)
        self.db_delete_object(spec_id, PortSpec.vtType, parent_type, parent_id)

    def change_portSpec(self, old_spec_id, port_spec, parent_id,
                        parent_type=Module.vtType):
        self.delete_port_from_registry(old_spec_id, parent_id)
        self.db_change_object(old_spec_id, port_spec, parent_type, parent_id)
        self.add_port_to_registry(port_spec, parent_id)

    def add_alias(self, name, type, oId, parentType, parentId):
        """add_alias(name: str, oId: int, parentType:str, parentId: int) -> None 
        Add alias to pipeline
          
        """
        if self.has_alias(name):
            raise VistrailsInternalError("duplicate alias")
        self.aliases[name] = (type, oId, parentType, parentId)

    def remove_alias_by_name(self, name):
        """remove_alias_by_name(name: str) -> None
        Remove alias with given name """
        if self.has_alias(name):
            del self.aliases[name]

    def remove_alias(self, type, oId, parentType, parentId):
        """remove_alias(name: str, type:?, oId: int)-> None
        Remove alias identified by oId """
        try:
            oldname = self.aliases.inverse[(type,oId, parentType, parentId)]
            del self.aliases[oldname]
        except KeyError:
            pass

    def change_alias(self, name, type, oId, parentType, parentId):
        """change_alias(name: str, type:str oId:int, parentType:str,
                        parentId:int)-> None
        Change alias if name is non empty. Else remove alias
        
        """
        if name == "":
            self.remove_alias(type, oId, parentType, parentId)
        else:
            if not self.has_alias(name):
                self.remove_alias(type, oId, parentType, parentId)
                self.add_alias(name, type, oId, parentType, parentId)
                
    def get_alias_str_value(self, name):
        """ get_alias_str_value(name: str) -> str
        returns the strValue of the parameter with alias name

        """
        try:
            what, oId, parentType, parentId = self.aliases[name]
        except KeyError:
            return ''
        else:
            if what == 'parameter':
                parameter = self.db_get_object(what, oId)
                return parameter.strValue
            else:
                raise VistrailsInternalError("only parameters are supported")

    def set_alias_str_value(self, name, value):
        """ set_alias_str_value(name: str, value: str) -> None
        sets the strValue of the parameter with alias name 
        
        """
        try:
            what, oId, parentType, parentId = self.aliases[name]
        except KeyError:
            pass
        else:
            if what == 'parameter':
                #FIXME: check if a change parameter action needs to be generated
                parameter = self.db_get_object(what, oId)
                parameter.strValue = str(value)
            else:
                raise VistrailsInternalError("only parameters are supported")
        
    def get_module_by_id(self, id):
        """get_module_by_id(id: int) -> Module
        Accessor. id is the Module id.
        
        """
        result = self.modules[id]
        if result.vtType != AbstractionModule.vtType and \
                result.vtType != Group.vtType and result.package is None:
            DebugPrint.critical('module %d is missing package' % id)
            descriptor = registry.get_descriptor_from_name_only(result.name)
            result.package = descriptor.identifier
        return result
    
    def get_connection_by_id(self, id):
        """get_connection_by_id(id: int) -> Connection
        Accessor. id is the Connection id.
        
        """
        self.ensure_connection_specs([id])
        return self.connections[id]
    
    def module_count(self):
        """ module_count() -> int 
        Returns the number of modules in the pipeline.
        
        """
        return len(self.modules)
    
    def connection_count(self):
        """connection_count() -> int 
        Returns the number of connections in the pipeline.
        
        """
        return len(self.connections)
    
    def has_module_with_id(self, id):
        """has_module_with_id(id: int) -> boolean 
        Checks whether given module exists.

        """
        return id in self.modules
    
    def has_connection_with_id(self, id):
        """has_connection_with_id(id: int) -> boolean 
        Checks whether given connection exists.

        """
        return id in self.connections

    def has_alias(self, name):
        """has_alias(name: str) -> boolean 
        Checks whether given alias exists.

        """
        return name in self.aliases

    def out_degree(self, id):
        """out_degree(id: int) -> int - Returns the out-degree of a module. """
        return self.graph.out_degree(id)

    ##########################################################################
    # Caching-related

    # Modules

    def module_signature(self, module_id):
        """module_signature(module_id): string
        Returns the signature for the module with given module_id."""
        try:
            return self._module_signatures[module_id]
        except KeyError:
            m = self.modules[module_id]
            sig = registry.module_signature(self, m)
            self._module_signatures[module_id] = sig
            return sig
    
    def module_id_from_signature(self, signature):
        """module_id_from_signature(sig): int
        Returns the module_id that corresponds to the given signature.
        This must have been previously computed."""
        return self._module_signatures.inverse[signature]

    def has_module_signature(self, signature):
        return signature in self._module_signatures.inverse

    # Subpipelines

    def subpipeline_signature(self, module_id):
        """subpipeline_signature(module_id): string
        Returns the signature for the subpipeline whose sink id is module_id."""
        try:
            return self._subpipeline_signatures[module_id]
        except KeyError:
            upstream_sigs = [(self.subpipeline_signature(m) +
                              Hasher.connection_signature(
                                  self.connections[edge_id]))
                             for (m, edge_id) in
                             self.graph.edges_to(module_id)]
            module_sig = self.module_signature(module_id)
            sig = Hasher.subpipeline_signature(module_sig,
                                               upstream_sigs)
            self._subpipeline_signatures[module_id] = sig
            return sig

    def subpipeline_id_from_signature(self, signature):
        """subpipeline_id_from_signature(sig): int
        Returns the module_id that corresponds to the given signature.
        This must have been previously computed."""
        return self._subpipeline_signatures.inverse[signature]

    def has_subpipeline_signature(self, signature):
        return signature in self._subpipeline_signatures.inverse

    # Connections

    def connection_signature(self, connection_id):
        """connection_signature(id): string
        Returns the signature for the connection with given id."""
        try:
            return self._connection_signatures[connection_id]
        except KeyError:
            c = self.connections[connection_id]
            source_sig = self.subpipeline_signature(c.sourceId)
            dest_sig = self.subpipeline_signature(c.destinationId)
            sig = Hasher.connection_subpipeline_signature(c, source_sig,
                                                          dest_sig)
            self._connection_signatures[connection_id] = sig
            return sig

    def connection_id_from_signature(self, signature):
        return self._connection_signatures.inverse[signature]

    def has_connection_signature(self, signature):
        return signature in self._connection_signatures.inverse

    def refresh_signatures(self):
        self._connection_signatures = {}
        self._subpipeline_signatures = {}
        self._module_signatures = {}
        self.compute_signatures()

    def compute_signatures(self):
        """compute_signatures(): compute all module and subpipeline signatures
        for this pipeline."""
        for i in self.modules.iterkeys():
            self.subpipeline_signature(i)
        for c in self.connections.iterkeys():
            self.connection_signature(c)

    def get_subpipeline(self, module_set):
        """get_subpipeline([module_id] or subgraph) -> Pipeline

        Returns a subset of the current pipeline with the modules passed
        in as module_ids and the internal connections between them."""
        if type(module_set) == list:
            subgraph = self.graph.subgraph(module_set)
        elif type(module_set) == Graph:
            subgraph = module_set
        else:
            raise Exception("Expected list of ints or graph")
        result = Pipeline()
        for module_id in subgraph.iter_vertices():
            result.add_module(copy.copy(self.modules[module_id]))
        for (conn_from, conn_to, conn_id) in subgraph.iter_all_edges():
            result.add_connection(copy.copy(self.connections[conn_id]))
    # I haven't finished this yet. -cscheid
        raise Exception("Incomplete implementation!")
        return result

    def dump_actions(self):
        """dump_actions() -> [Action].

        Returns a list of actions that can be used to create a copy of the
        pipeline."""

        raise Exception('broken')
#         result = []
#         for m in self.modules:
#             add_module = core.vistrail.action.AddModuleAction()
#             add_module.module = copy.copy(m)
#             result.append(add_module)
#         for c in self.connections:
#             add_connection = core.vistrail.action.AddConnectionAction()
#             add_connection.connection = copy.copy(c)
#             result.append(add_connection)
#         return result

    ##########################################################################
    # Registry-related

    def ensure_old_modules_have_package_names(self):
        """ensure_old_modules_have_package_names()

        Makes sure each module has a package associated with it.

        """
        for i in self.modules.iterkeys():
            self.get_module_by_id(i)

    def ensure_connection_specs(self, connection_ids=None):
        """ensure_connection_specs(connection_ids=None) -> None.

        Computes the specs for the connections in connection_ids. If
        connection_ids is None, computes it for every connection in the pipeline.
        """
        def source_spec(port):
            module = self.get_module_by_id(port.moduleId)
            reg = module.registry or registry
            port_list = []
            def do_it(ports):
                # Following line is weird because we're in a hot-path
                port_list.extend([p for p in ports
                                  if (p._db_name ==
                                      port._db_name)])

            do_it(reg.module_source_ports(False,
                                          module.package,
                                          module.name,
                                          module.namespace))
            if len(port_list) == 0:
                # The port might still be in the original registry
                do_it(registry.module_source_ports(False, module.package,
                                                   module.name,
                                                   module.namespace))
            if len(port_list) == 0:
                print "Failed", module.package, module.name, module.namespace, port.name
                assert False
            
            # if port_list has more than one element, then it's an
            # overloaded port. Source (output) port overloads must all
            # be contravariant. This means that the spec of the
            # new port must be a subtype of the spec of the
            # original port. This induces a total ordering on the types,
            # which we use to sort the possible ports, and get the
            # most strict one.
            
            port_list.sort(lambda p1, p2:
                           (p1 != p2 and
                            issubclass(p1.spec.signature[0][0],
                                       p2.spec.signature[0][0])))
            return copy.copy(port_list[0].spec)

        def destination_spec(port):
            module = self.get_module_by_id(port.moduleId)
            reg = module.registry or registry
            port_list = []
            def do_it(ports):
                # Following line is weird because we're in a hot-path
                port_list.extend([p for p in ports
                                  if (p._db_name ==
                                      port._db_name)])

            do_it(reg.module_destination_ports(False, module.package, module.name, module.namespace))
            if len(port_list) == 0:
                # The port might still be in the original registry
                do_it(registry.module_destination_ports(False, module.package,
                                                        module.name,
                                                        module.namespace))
            if len(port_list) == 0:
                print "Failed", module.package, module.name, module.namespace, port.name
                assert False

            # if port_list has more than one element, then it's an
            # overloaded port. Destination (input) port overloads must
            # all be covariant. This means that the spec of the new
            # port must be a supertype of the spec of the original
            # port. This induces a total ordering on the types, which
            # we use to sort the possible ports, and get the most
            # general one.

            port_list.sort(lambda p1, p2:
                           (p1 != p2 and
                            issubclass(p1.spec.signature[0][0],
                                       p2.spec.signature[0][0])))
            return copy.copy(port_list[-1].spec)

        if connection_ids is None:
            connection_ids = self.connections.iterkeys()
        for conn_id in connection_ids:
            conn = self.connections[conn_id]
            if not conn.source.spec:
                conn.source.spec = source_spec(conn.source)
            if not conn.destination.spec:
                conn.destination.spec = destination_spec(conn.destination)

    def ensure_modules_are_on_registry(self, module_ids=None):
        """ensure_modules_are_on_registry(module_ids: optional list of module ids) -> None

        Queries the module registry for the module information in the
        given modules.  The only goal of this function is to trigger
        exceptions in the registry that will be treated somewhere else
        in the calling stack.
        
        If modules are not on registry, the registry will raise
        MissingModulePackage exceptions that should be caught and handled.

        if no module_ids list is given, we assume every module in the pipeline.
        """

        if module_ids == None:
            module_ids = self.modules.iterkeys()
        for mid in module_ids:
            registry.get_descriptor_by_name(self.modules[mid].package,
                                            self.modules[mid].name,
                                            self.modules[mid].namespace)

    ##########################################################################
    # Debugging

    def show_comparison(self, other):
        if type(other) != type(self):
            print "type mismatch"
            return
        if len(self.module_list) != len(other.module_list):
            print "module lists of different sizes"
            return
        if len(self.connection_list) != len(other.connection_list):
            print "Connection lists of different sizes"
            return
        for m_id, m in self.modules.iteritems():
            if not m_id in other.modules:
                print "module %d in self but not in other" % m_id
                return
            if m <> other.modules[m_id]:
                print "module %s in self doesn't match module %s in other" % (m,  other.modules[m_id])
                return
        for m_id, m in other.modules.iteritems():
            if not m_id in self.modules:
                print "module %d in other but not in self" % m_id
                return
            # no need to check equality since this was already done before
        for c_id, c in self.connections.iteritems():
            if not c_id in other.connections:
                print "connection %d in self but not in other" % c_id
                return
            if c <> other.connections[c_id]:
                print "connection %s in self doesn't match connection %s in other" % (c,  other.connections[c_id])
                return
        for c_id, c, in other.connections.iteritems():
            if not c_id in self.connections:
                print "connection %d in other but not in self" % c_id
                return
            # no need to check equality since this was already done before
        assert self == other

    ##########################################################################
    # Operators

    def __ne__(self, other):
        return not self.__eq__(other)

# There's a bug in this code that's not easily worked around: if
# modules are in different order in the list, there's no easy way to
# check for equality. The solution is to move to a check that
# takes module and connection ids into account.
#     def __eq__(self, other):
#         if type(other) != type(self):
#             return False
#         if len(self.module_list) != len(other.module_list):
#             return False
#         if len(self.connection_list) != len(other.connection_list):
#             return False
#         for f, g in zip(self.module_list, other.module_list):
#             if f != g:
#                 return False
#         for f, g in zip(self.connection_list, other.connection_list):
#             if f != g:
#                 return False
#         return True

    def __eq__(self, other):
        if type(other) != type(self):
            return False
        if len(self.module_list) != len(other.module_list):
            return False
        if len(self.connection_list) != len(other.connection_list):
            return False
        for m_id, m in self.modules.iteritems():
            if not m_id in other.modules:
                return False
            if m <> other.modules[m_id]:
                return False
        for m_id, m in other.modules.iteritems():
            if not m_id in self.modules:
                return False
            # no need to check equality since this was already done before
        for c_id, c in self.connections.iteritems():
            if not c_id in other.connections:
                return False
            if c <> other.connections[c_id]:
                return False
        for c_id, c, in other.connections.iteritems():
            if not c_id in self.connections:
                return False
            # no need to check equality since this was already done before
        return True

    def __str__(self):
        return ("(Pipeline Modules: %s Graph:%s)@%X" %
                ([(m, str(v)) for (m,v) in sorted(self.modules.items())],
                 str(self.graph),
                 id(self)))


################################################################################

import unittest
from core.vistrail.abstraction_module import AbstractionModule
from core.vistrail.connection import Connection
from core.vistrail.location import Location
from core.vistrail.module import Module
from core.vistrail.module_function import ModuleFunction
from core.vistrail.module_param import ModuleParam
from core.vistrail.port import Port
from db.domain import IdScope

class TestPipeline(unittest.TestCase):

    def create_default_pipeline(self, id_scope=None):
        if id_scope is None:
            id_scope = IdScope()
        
        p = Pipeline()
        p.id = id_scope.getNewId(Pipeline.vtType)

        def module1(p):
            def f1():
                f = ModuleFunction()
                f.real_id = id_scope.getNewId(ModuleFunction.vtType)
                f.name = 'op'
                f.returnType = 'void'
                param = ModuleParam()
                param.type = 'String'
                param.strValue = '+'
                f.params.append(param)
                return f
            def f2():
                f = ModuleFunction()
                f.real_id = id_scope.getNewId(ModuleFunction.vtType)
                f.name = 'value1'
                f.returnType = 'void'
                param = ModuleParam()
                param.type = 'Float'
                param.strValue = '2.0'
                f.params.append(param)
                return f
            def f3():
                f = ModuleFunction()
                f.real_id = id_scope.getNewId(ModuleFunction.vtType)
                f.name = 'value2'
                f.returnType = 'void'
                param = ModuleParam()
                param.type = 'Float'
                param.strValue = '4.0'
                f.params.append(param)
                return f
            m = Module()
            m.id = id_scope.getNewId(Module.vtType)
            m.name = 'PythonCalc'
            m.package = 'edu.utah.sci.vistrails.pythoncalc'
            m.functions.append(f1())
            return m
        
        def module2(p):
            def f1():
                f = ModuleFunction()
                f.real_id = id_scope.getNewId(ModuleFunction.vtType)
                f.name = 'op'
                f.returnType = 'void'
                param = ModuleParam()
                param.type = 'String'
                param.strValue = '+'
                f.params.append(param)
                return f
            m = Module()
            m.id = id_scope.getNewId(Module.vtType)
            m.name = 'PythonCalc'
            m.package = 'edu.utah.sci.vistrails.pythoncalc'
            m.functions.append(f1())
            return m
        m1 = module1(p)
        p.add_module(m1)
        m2 = module1(p)
        p.add_module(m2)
        m3 = module2(p)
        p.add_module(m3)

        c1 = Connection()
        c1.sourceId = m1.id
        c1.destinationId = m3.id
        c1.source.id = id_scope.getNewId(Port.vtType)
        c1.destination.id = id_scope.getNewId(Port.vtType)
        c1.source.name = 'value'
        c1.source.moduleName = 'PythonCalc'
        c1.destination.name = 'value1'
        c1.destination.moduleName = 'PythonCalc'
        c1.id = id_scope.getNewId(Connection.vtType)
        p.add_connection(c1)

        c2 = Connection()
        c2.sourceId = m2.id
        c2.destinationId = m3.id
        c2.source.id = id_scope.getNewId(Port.vtType)
        c2.destination.id = id_scope.getNewId(Port.vtType)
        c2.source.name = 'value'
        c2.source.moduleName = 'PythonCalc'
        c2.destination.name = 'value2'
        c2.destination.moduleName = 'PythonCalc'
        c2.id = id_scope.getNewId(Connection.vtType)

        p.add_connection(c2)
        p.compute_signatures()
        return p

    def create_pipeline2(self, id_scope=None):
        if id_scope is None:
            id_scope = IdScope(remap={AbstractionModule.vtType: Module.vtType})

        param1 = ModuleParam(id=id_scope.getNewId(ModuleParam.vtType),
                             type='Int',
                             val='1')
        param2 = ModuleParam(id=id_scope.getNewId(ModuleParam.vtType),
                             type='Float',
                             val='1.3456')
        func1 = ModuleFunction(id=id_scope.getNewId(ModuleFunction.vtType),
                               name='value',
                               parameters=[param1])
        func2 = ModuleFunction(id=id_scope.getNewId(ModuleFunction.vtType),
                               name='floatVal',
                               parameters=[param2])
        loc1 = Location(id=id_scope.getNewId(Location.vtType),
                        x=12.342,
                        y=-19.432)
        loc2 = Location(id=id_scope.getNewId(Location.vtType),
                        x=21.34,
                        y=456.234)
        m1 = Module(id=id_scope.getNewId(Module.vtType),
                    package='edu.utah.sci.vistrails.basic',
                    name='String',
                    location=loc1,
                    functions=[func1])
        m2 = AbstractionModule(id=id_scope.getNewId(AbstractionModule.vtType),
                               abstraction_id=1,
                               version=13,
                               location=loc2,
                               functions=[func2])
        source = Port(id=id_scope.getNewId(Port.vtType),
                      type='source', 
                      moduleId=m1.id, 
                      moduleName='String', 
                      name='self',
                      spec='edu.sci.utah.vistrails.basic:String')
        destination = Port(id=id_scope.getNewId(Port.vtType),
                           type='destination',
                           moduleId=m2.id,
                           moduleName='AbstractionModule',
                           name='self',
                           spec='')
        c1 = Connection(id=id_scope.getNewId(Connection.vtType),
                        ports=[source, destination])
        pipeline = Pipeline(id=id_scope.getNewId(Pipeline.vtType),
                            modules=[m1, m2],
                            connections=[c1])
        return pipeline

    def setUp(self):
        self.pipeline = self.create_default_pipeline()
        self.sink_id = 2

    def test_create_pipeline_signature(self):
        self.pipeline.subpipeline_signature(self.sink_id)

    def test_delete_signatures(self):
        """Makes sure signatures are deleted when other things are."""
        p = self.create_default_pipeline()
        m_sig_size_before = len(p._module_signatures)
        c_sig_size_before = len(p._connection_signatures)
        p_sig_size_before = len(p._subpipeline_signatures)
        p.delete_connection(0)
        p.delete_module(0)
        m_sig_size_after = len(p._module_signatures)
        c_sig_size_after = len(p._connection_signatures)
        p_sig_size_after = len(p._subpipeline_signatures)
        self.assertNotEquals(m_sig_size_before, m_sig_size_after)
        self.assertNotEquals(c_sig_size_before, c_sig_size_after)
        self.assertNotEquals(p_sig_size_before, p_sig_size_after)

    def test_delete_connections(self):
        p = self.create_default_pipeline()
        p.delete_connection(0)
        p.delete_connection(1)
        p.delete_module(2)
        self.assertEquals(len(p.connections), 0)

    def test_basic(self):
        """Makes sure pipeline can be created, modules and connections
        can be added and deleted."""
        p = self.create_default_pipeline()
    
    def test_copy(self):
        id_scope = IdScope()
        
        p1 = self.create_default_pipeline(id_scope)
        p2 = copy.copy(p1)
        self.assertEquals(p1, p2)
        self.assertEquals(p1.id, p2.id)
        p3 = p1.do_copy(True, id_scope, {})
        self.assertNotEquals(p1, p3)
        self.assertNotEquals(p1.id, p3.id)

    def test_copy2(self):
        import core.db.io

        # nedd to id modules and abstraction_modules with same counter
        id_scope = IdScope(remap={AbstractionModule.vtType: Module.vtType})
        
        p1 = self.create_pipeline2(id_scope)
        p2 = copy.copy(p1)
        self.assertEquals(p1, p2)
        self.assertEquals(p1.id, p2.id)
        p3 = p1.do_copy(True, id_scope, {})
        self.assertNotEquals(p1, p3)
        self.assertNotEquals(p1.id, p3.id)

    def test_serialization(self):
        import core.db.io
        p1 = self.create_default_pipeline()
        xml_str = core.db.io.serialize(p1)
        p2 = core.db.io.unserialize(xml_str, Pipeline)
        self.assertEquals(p1, p2)
        self.assertEquals(p1.id, p2.id)        

    def test_serialization2(self):
        import core.db.io
        p1 = self.create_pipeline2()
        xml_str = core.db.io.serialize(p1)
        p2 = core.db.io.unserialize(xml_str, Pipeline)
        self.assertEquals(p1, p2)
        self.assertEquals(p1.id, p2.id)        

    def test_aliases(self):
        """ Exercises aliases manipulation """
        import core.db.action
        from core.db.locator import XMLFileLocator
        import core.system
        v = XMLFileLocator( \
            core.system.vistrails_root_directory() +
            '/tests/resources/test_alias.xml').load()

        p1 = v.getPipeline('alias')
        p2 = v.getPipeline('alias')
        
        # testing removing an alias
        old_param = p1.modules[0].functions[0].params[0]
        func = p1.modules[0].functions[0]
        #old_id = p1.modules[0].functions[0].params[0].db_id
        #old_f_id = p1.modules[0].functions[0].db_id
        new_param = ModuleParam(id=-1,
                                pos=old_param.pos,
                                name=old_param.name,
                                alias="",
                                val=str(v),
                                type=old_param.type)
        action_spec = ('change', old_param, new_param,
                       func.vtType, func.real_id)
        action = core.db.action.create_action([action_spec])
        p1.perform_action(action)
        self.assertEquals(p1.has_alias('v1'),False)
        v1 = p2.aliases['v1']
        old_param2 = p2.modules[0].functions[0].params[0]
        new_param2 = ModuleParam(id=old_param.real_id,
                                pos=old_param.pos,
                                name=old_param.name,
                                alias="v1",
                                val=str(v),
                                type=old_param.type)
        func2 = p2.modules[0].functions[0]
        action2 = core.db.action.create_action([('change',
                                                 old_param2,
                                                 new_param2,
                                                 func2.vtType,
                                                 func2.real_id)
                                                ])
        p2.perform_action(action2)
        self.assertEquals(v1, p2.aliases['v1'])
        
    def test_module_signature(self):
        """Tests signatures for modules with similar (but not equal)
        parameter specs."""
        p1 = Pipeline()
        p1_functions = [ModuleFunction(name='value1',
                                       parameters=[ModuleParam(type='Float',
                                                               val='1.0',
                                                               )],
                                       ),
                        ModuleFunction(name='value2',
                                       parameters=[ModuleParam(type='Float',
                                                               val='2.0',
                                                               )],
                                       )]
        p1.add_module(Module(name='PythonCalc',
                             package='edu.utah.sci.vistrails.pythoncalc',
                             id=3,
                             functions=p1_functions))

        p2 = Pipeline()
        p2_functions = [ModuleFunction(name='value1',
                                       parameters=[ModuleParam(type='Float',
                                                               val='2.0',
                                                               )],
                                       ),
                        ModuleFunction(name='value2',
                                       parameters=[ModuleParam(type='Float',
                                                               val='1.0',
                                                               )],
                                       )]
        p2.add_module(Module(name='PythonCalc', 
                             package='edu.utah.sci.vistrails.pythoncalc',
                             id=3,
                             functions=p2_functions))

        self.assertNotEquals(p1.module_signature(3),
                             p2.module_signature(3))

    def test_find_method(self):
        p1 = Pipeline()
        p1_functions = [ModuleFunction(name='i1',
                                       parameters=[ModuleParam(type='Float',
                                                               val='1.0',
                                                               )],
                                       ),
                        ModuleFunction(name='i2',
                                       parameters=[ModuleParam(type='Float',
                                                               val='2.0',
                                                               )],
                                       )]
        p1.add_module(Module(name='CacheBug', 
                            package='edu.utah.sci.vistrails.console_mode_test',
                            id=3,
                            functions=p1_functions))

        self.assertEquals(p1.find_method(3, 'i1'), 0)
        self.assertEquals(p1.find_method(3, 'i2'), 1)
        self.assertEquals(p1.find_method(3, 'i3'), -1)
        self.assertRaises(KeyError, p1.find_method, 4, 'i1')

    def test_str(self):
        p1 = Pipeline()
        p1_functions = [ModuleFunction(name='i1',
                                       parameters=[ModuleParam(type='Float',
                                                               val='1.0',
                                                               )],
                                       ),
                        ModuleFunction(name='i2',
                                       parameters=[ModuleParam(type='Float',
                                                               val='2.0',
                                                               )],
                                       )]
        p1.add_module(Module(name='CacheBug', 
                            package='edu.utah.sci.vistrails.console_mode_test',
                            id=3,
                            functions=p1_functions))
        str(p1)

    def test_pipeline_equality_module_list_out_of_order(self):
        p1 = Pipeline()
        p1.add_module(Module(name='Foo',
                             package='bar',
                             id=0,
                             functions=[]))
        p1.add_module(Module(name='Foo2',
                             package='bar',
                             id=1,
                             functions=[]))
        p2 = Pipeline()
        p2.add_module(Module(name='Foo2',
                             package='bar',
                             id=1,
                             functions=[]))
        p2.add_module(Module(name='Foo',
                             package='bar',
                             id=0,
                             functions=[]))
        assert p1 == p2

#     def test_subpipeline(self):
#         p = self.create_default_pipeline()
#         p2 = p.get_subpipeline([0, 1])
#         for m in p2.modules:
#             print m
#         for c in p2.connections:
#             print c

if __name__ == '__main__':
    unittest.main()
