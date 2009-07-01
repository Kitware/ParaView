
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
""" This file contains the definition of the class Vistrail """

import copy
import datetime
import getpass
import itertools
import string
import traceback
import xml.dom.minidom

from db.domain import DBVistrail
from db import VistrailsDBException
from core.data_structures.graph import Graph
from core.data_structures.bijectivedict import Bidict
from core.debug import DebugPrint
import core.db.io
from core.utils import enum, VistrailsInternalError, InstanceObject
from core.vistrail.action import Action
from core.vistrail.abstraction import Abstraction
from core.vistrail.annotation import Annotation
from core.vistrail.connection import Connection
from core.vistrail.location import Location
from core.vistrail.module import Module
from core.vistrail.module_function import ModuleFunction
from core.vistrail.module_param import ModuleParam
from core.vistrail.operation import AddOp, ChangeOp, DeleteOp
from core.vistrail.pipeline import Pipeline
from core.vistrail.plugin_data import PluginData
from core.vistrail.port_spec import PortSpec
from core.vistrail.tag import Tag
################################################################################

class Vistrail(DBVistrail):
    """Vistrail is the base class for storing versioned pipelines.

    Because of the automatic loading from the db layer, the fields in
    the class will seem mysterious.

    self.currentVersion: version of the schema being used for this vistrail
    (Do not confuse with the currently selected version on the controller)

    self.actions: list of core/vistrail/action/Action objects

    self.actionMap: dictionary from version number to action object.

    self.tagMap: dictionary from version number to tag object.


    Simple use cases:

    To get a version number given a tag name, use
    get_tag_by_name(tag_name).id

    """
  
    def __init__(self, locator=None):
  DBVistrail.__init__(self)

        self.changed = False
        self.currentVersion = -1
        self.currentGraph=None
        # self.prunedVersions = set()
        self.savedQueries = []
        self.locator = locator

        # object to keep explicit expanded 
        # version tree always updated
        self.tree = ExplicitExpandedVersionTree(self)

    ##########################################################################
    # Properties

    id = DBVistrail.db_id
    actions = DBVistrail.db_actions # This is now read-write
    tags = DBVistrail.db_tags # This is now read-write
    abstractions = DBVistrail.db_abstractions # This is now read-write
    annotations = DBVistrail.db_annotations
    
    def _get_actionMap(self):
        return self.db_actions_id_index
    actionMap = property(_get_actionMap)

    def _get_tagMap(self):
        return self.db_tags_id_index
    tagMap = property(_get_tagMap)

    def get_tag_by_name(self, name):
        return self.db_get_tag_by_name(name)
    def has_tag_with_name(self, name):
        return self.db_has_tag_with_name(name)
    
    def _get_abstractionMap(self):
        return self.db_abstractions_id_index
    abstractionMap = property(_get_abstractionMap)

    @staticmethod
    def convert(_vistrail):
  _vistrail.__class__ = Vistrail
        _vistrail.changed = False
        _vistrail.currentVersion = -1
        _vistrail.currentGraph=None
        # _vistrail.prunedVersions = set()
        _vistrail.savedQueries = []

  for action in _vistrail.actions:
      Action.convert(action)
  for tag in _vistrail.tags:
            Tag.convert(tag)
        for abstraction in _vistrail.abstractions:
            Abstraction.convert(abstraction)
        for annotation in _vistrail.annotations:
            Annotation.convert(annotation)
  _vistrail.changed = False

        # brute force creation of Vistrail object
        # needs a "tree" field also!
        _vistrail.tree = ExplicitExpandedVersionTree(_vistrail)

        # add all versions to the trees
  for action in _vistrail.actions:
      _vistrail.tree.addVersion(action.id, action.prevId)

    def get_annotation(self, key):
        if self.db_has_annotation_with_key(key):
            return self.db_get_annotation_by_key(key)
        return None
    
    def set_annotation(self, key, value):
        if self.db_has_annotation_with_key(key):
            old_annotation = self.db_get_annotation_by_key(key)
            if old_annotation.value == value:
                return False
            self.db_delete_annotation(old_annotation)
        annotation = Annotation(id=self.idScope.getNewId(Annotation.vtType),
                                key=key,
                                value=value,
                                )
        self.db_add_annotation(annotation)
        return True

    def _get_plugin_info(self):
        annotation = self.get_annotation("__plugin_info__")
        return annotation.value if annotation is not None else ""
    def _set_plugin_info(self, value):
        return self.set_annotation("__plugin_info__", value)
    plugin_info = property(_get_plugin_info, _set_plugin_info)

    def _get_database_info(self):
        annotation = self.get_annotation("__database_info__")
        return annotation.value if annotation is not None else ""
    def _set_database_info(self, value):
        return self.set_annotation("__database_info__", value)
    database_info = property(_get_database_info, _set_database_info)

    def getVersionName(self, version):
        """ getVersionName(version) -> str 
        Returns the name of a version, if it exists. Returns an empty string
        if it doesn't. 
        
        """
        try:
            return self.tagMap[version].name
        except KeyError:
            return ""

    def get_version_count(self):
        """get_version_count() -> Integer
        Returns the total number of versions in this vistrail.

        """
        return len(self.actionMap)

    def get_version_number(self, version):
        """get_version_number(version) -> Integer
        Returns the version number given a tag.

        """
        return self.get_tag_by_name(version).time
    
    def get_latest_version(self):
        """get_latest_version() -> Integer
        Returns the latest version id for the vistrail.

        FIXME: Running time O(|versions|)

        FIXME: Check if pruning is handled correctly here.

        """
        try:
            return max(v.id for v in self.actions if not v.prune)
        except:
            return 0
                   
    def getPipeline(self, version):
        """getPipeline(number or tagname) -> Pipeline
        Return a pipeline object given a version number or a version name. 

        """
        return Vistrail.getPipelineDispatcher[type(version)](self, version)
    
    def getPipelineVersionName(self, version):
        """getPipelineVersionName(version:str) -> Pipeline
        Returns a pipeline given a version name. If version name doesn't exist
        it will return None.

        """
        if self.has_tag_with_name(version):
#             number = self.tagMap[version]
      number = self.get_tag_by_name(version).time
            return self.getPipelineVersionNumber(number)
        else:
            return None

    def getPipelineVersionTag(self, version):
        """getPipelineVersionTag(version:Tag) -> Pipeline
        Returns a pipeline given a version tag. If version tag doesn't exist
        it will return None.

        """
  return self.getPipelineVersionNumber(version.time)
    
    def getPipelineVersionNumber(self, version):
        """getPipelineVersionNumber(version:int) -> Pipeline
        Returns a pipeline given a version number.

        """
        workflow = core.db.io.get_workflow(self, version)
        workflow.set_abstraction_map(self.abstractionMap)
        return workflow


    def getPipelineDiffByAction(self, v1, v2):
        """ getPipelineDiffByAction(v1: int, v2: int) -> tuple(list,list,list)
        Compute the diff between v1 and v2 just by looking at the
        action chains. The value returned is a tuple containing lists
        of shared, v1 not v2, and v2 not v1 modules
        
        """
        # Get first common ancestor
        p = self.getFirstCommonVersion(v1,v2)
        
        # Get the modules present in v1 and v2
        v1andv2 = []
        v2Only = []
        v1Only = []
        sharedCreations = []
        parent = self.getPipeline(p)
        for m in parent.modules.keys():
            v1andv2.append(m)
            sharedCreations.append(m)
        l1 = self.actionChain(v1,p)
        l2 = self.actionChain(v2,p)
        l = l1 + l2

        # Take deleted modules out of shared modules
        for a in l:
            if a.type == "DeleteModule":
                for id in a.ids:
                    if id in v1andv2:
                        v1andv2.remove(id)

        # Add deleted "shared modules" of v2 to v1
        for a in l2:
            if a.type == "DeleteModule":
                for id in a.ids:
                    if id in sharedCreations:
                        v1Only.append(id)
                        
        # Add deleted "shared modules" of v1 to v2
        for a in l1:
            if a.type == "DeleteModule":
                for id in a.ids:
                    if id in sharedCreations:
                        v2Only.append(id)

        # Add module created by v1 only
        for a in l1:
            if a.type == "AddModule":
                if a.module.id not in v1Only:
                    v1Only.append(a.module.id)
            if a.type == "DeleteModule":
                for id in a.ids:
                    if id in v1Only:
                        v1Only.remove(id)
                    
        # Add module created by v2 only
        for a in l2:
            if a.type == "AddModule":
                if a.module.id not in v2Only:
                    v2Only.append(a.module.id)
            if a.type == "DeleteModule":
                for id in a.ids:
                    if id in v2Only:
                        v2Only.remove(id)
                    
        return (v1andv2,v1Only,v2Only)

    def make_actions_from_diff(self, diff):
        """ make_actions_from_diff(diff) -> [action]
        Returns a sequence of actions that performs the diff.

        (The point is that this might be smaller than the
        algebra-based one).
        """
        (p1,
         p2,
         m_shared,
         m_to_be_deleted,
         m_to_be_added,
         parameter_changes,
         c_shared,
         c_to_be_deleted,
         c_to_be_added) = (diff.p1,
                           diff.p2,
                           diff.v1andv2,
                           diff.v1only,
                           diff.v2only,
                           diff.paramchanged,
                           diff.c1andc2,
                           diff.c1only,
                           diff.c2only)

        p1_c = copy.copy(p1)
        result = []

        module_id_remap = Bidict()
        module_id_remap.update(m_shared)

        connection_id_remap = Bidict()
        connection_id_remap.update(c_shared)
        
        for ((m_id_from, m_id_to), _) in parameter_changes:
            module_id_remap[m_id_from] = m_id_to

        # First all the modules to get the remap
        for p2_m_id in m_to_be_added:
            add_module = AddModuleAction()
            add_module.module = copy.copy(p2.modules[p2_m_id])
            add_module.module.id = p1_c.fresh_module_id()
            module_id_remap[add_module.module.id] = p2_m_id
            result.append(add_module)
            add_module.perform(p1_c)


        # Then all the connections using the remap
        for p2_c_id in c_to_be_added:
            c2 = p2.connections[p2_c_id]
            add_connection = AddConnectionAction()
            new_c = copy.copy(c2)
            add_connection.connection = new_c
            new_c.id = p1_c.fresh_connection_id()
            new_c.sourceId = module_id_remap.inverse[c2.sourceId]
            new_c.destinationId = module_id_remap.inverse[c2.destinationId]
            connection_id_remap[c2.id] = new_c.id
            result.append(add_connection)
            add_connection.perform(p1_c)


        # Now delete all connections:
        delete_conns = DeleteConnectionAction()
        delete_conns.ids = copy.copy(c_to_be_deleted)
        if len(delete_conns.ids) > 0:
            delete_conns.perform(p1_c)
            result.append(delete_conns)

        # And then all the modules
        delete_modules = DeleteModuleAction()
        delete_modules.ids = copy.copy(m_to_be_deleted)
        if len(delete_modules.ids) > 0:
            delete_modules.perform(p1_c)
            result.append(delete_modules)

        # From now on, module_id_remap is not necessary, we can act
        # on p1 ids without worry. (they still exist)

        # Now move everyone
        move_action = MoveModuleAction()
        for (p1_m_id, p2_m_id) in m_shared.iteritems():
            delta = p2.modules[p2_m_id].location - p1.modules[p1_m_id].location
            move_action.addMove(p1_m_id, delta.x, delta.y)
        move_action.perform(p1_c)
        result.append(move_action)

        # Now change parameters
        def make_param_change(fto_name, fto_params,
                              m_id, f_id, m):
            action = ChangeParameterAction()
            for (p_id, param) in enumerate(fto_params):
                p_name = m.functions[f_id].params[p_id].name
                p_alias = m.functions[f_id].params[p_id].alias
                (p_type, p_value) = param
                action.addParameter(m_id, f_id, p_id, fto_name,
                                    p_name, p_value, p_type, p_alias)
            return action
        
        if len(parameter_changes):
            # print parameter_changes
            for ((m_from_id, m_to_id), plist) in parameter_changes:
                m_from = p1.modules[m_to_id]
                for ((ffrom_name, ffrom_params),
                     (fto_name, fto_params)) in plist:
                    for (f_id, f) in enumerate(m_from.functions):
                        if f.name != fto_name: continue
                        new_action = make_param_change(fto_name,
                                                       fto_params,
                                                       m_from_id,
                                                       f_id,
                                                       m_from)
                        new_action.perform(p1_c)
                        result.append(new_action)

        return (result,
                module_id_remap,
                connection_id_remap)

    def get_pipeline_diff_with_connections(self, v1, v2):
        """like get_pipeline_diff but returns connection info"""
        (p1, p2, v1andv2, v1only,
         v2only, paramchanged) = self.getPipelineDiff(v1, v2)

        v1andv2 = Bidict(v1andv2)

        # Now, do the connections between shared modules

        c1andc2 = []
        c1only = []
        c2only = []

        used = set()
        for (edge_from_1, edge_to_1,
             edge_id_1) in p1.graph.iter_all_edges():
            try:
                edge_from_2 = v1andv2[edge_from_1]
                edge_to_2 = v1andv2[edge_to_1]
            except KeyError:
                # edge is clearly in c1, so must not be in c2
                c1only.append(edge_id_1)
                continue
            c1 = p1.connections[edge_id_1]
            found = False
            for (_, _, edge_id_2) in [x for x
                                      in p2.graph.iter_edges_from(edge_from_2)
                                      if x[1] == edge_to_2]:
                c2 = p2.connections[edge_id_2]
                if c1.equals_no_id(c2) and not edge_id_2 in used:
                    # Found edge in both
                    c1andc2.append((edge_id_1, edge_id_2))
                    used.add(edge_id_2)
                    found = True
                    continue
            if not found:
                c1only.append(edge_id_1)

        used = set()
        for (edge_from_2, edge_to_2,
             edge_id_2) in p2.graph.iter_all_edges():
            try:
                edge_from_1 = v1andv2.inverse[edge_from_2]
                edge_to_1 = v1andv2.inverse[edge_to_2]
            except KeyError:
                # edge is clearly in c2, so must not be in c1
                c2only.append(edge_id_2)
                continue
            c2 = p2.connections[edge_id_2]
            found = False
            for (_, _, edge_id_1) in [x for x
                                      in p1.graph.iter_edges_from(edge_from_1)
                                      if x[1] == edge_to_1]:
                c1 = p1.connections[edge_id_1]
                if c2.equals_no_id(c1) and not edge_id_1 in used:
                    # Found edge in both, but it was already added. just mark
                    # and continue
                    found = True
                    used.add(edge_id_1)
                    continue
            if not found:
                c2only.append(edge_id_2)

        return InstanceObject(p1=p1, p2=p2, v1andv2=v1andv2, v1only=v1only,
                              v2only=v2only, paramchanged=paramchanged,
                              c1andc2=Bidict(c1andc2),
                              c1only=c1only, c2only=c2only)

    def getPipelineDiff(self, v1, v2):
        """ getPipelineDiff(v1: int, v2: int) -> tuple        
        Perform a diff between 2 versions, this will obtain the shared
        modules by getting shared nodes on the version tree. After,
        that, it will perform a heuristic algorithm to match
        signatures of modules to get more shared/diff modules. The
        heuristic is O(N^2), where N = the number of modules

        Keyword arguments:
        v1     --- the first version number
        v2     --- the second version number
        return --- (p1, p2: VistrailPipeline,
                    [shared modules (id in v1, id in v2) ...],
                    [v1 not v2 modules],
                    [v2 not v1 modules],
                    [parameter-changed modules (see-below)])

        parameter-changed modules = [((module id in v1, module id in v2),
                                      [(function in v1, function in v2)...]),
                                      ...]
        
        """
        return core.db.io.get_workflow_diff(self, v1, v2)

        # Instantiate pipelines associated with v1 and v2
        p1 = self.getPipelineVersionNumber(v1)
        p2 = self.getPipelineVersionNumber(v2)

        # Find the shared modules deriving from the version tree
        # common ancestor
        (v1Andv2, v1Only, v2Only) = self.getPipelineDiffByAction(v1, v2)

        # Convert v1Andv2 to a list of tuple
        v1Andv2 = [(i,i) for i in v1Andv2]

        # Looking for more shared modules by looking at all modules of
        # v1 and determine if there is an corresponding one in v2.
        # Only look by name for now
        for m1id in copy.copy(v1Only):
            m1 = p1.modules[m1id]
            for m2id in v2Only:
                m2 = p2.modules[m2id]
                if m1.name==m2.name:
                    v1Andv2.append((m1id, m2id))
                    v1Only.remove(m1id)
                    v2Only.remove(m2id)
                    break

        # Capture parameter changes
        paramChanged = []
        for (m1id,m2id) in copy.copy(v1Andv2):
            m1 = p1.modules[m1id]
            m2 = p2.modules[m2id]
            # Get signatures of all functions in m1 and m2
            signature1 = []
            signature2 = []
            for f1 in m1.functions:
                signature1.append((f1.name,
                                   [(p.type, str(p.strValue))
                                    for p in f1.params]))
            for f2 in m2.functions:
                signature2.append((f2.name,
                                   [(p.type, str(p.strValue))
                                    for p in f2.params]))

            if signature1!=signature2:
                v1Andv2.remove((m1id,m2id))
                paramMatching = []
                id2 = 0
                for s1 in signature1:
                    # Looking for a match and perform a panel-to-panel
                    # comparison
                    i = id2
                    match = None
                    while i<len(signature2):
                        s2 = signature2[i]
                        if s1==s2:
                            match = i
                            break
                        if s1[0]==s2[0] and match==None:
                            match = i
                        i += 1
                    if match!=None:
                        paramMatching.append((s1, signature2[match]))
                        while id2<match:
                            paramMatching.append(((None, None), signature2[id2]))
                            id2 += 1
                        id2 += 1
                    else:
                        paramMatching.append((s1, (None, None)))
                while id2<len(signature2):
                    paramMatching.append(((None, None), signature2[id2]))
                    id2 += 1
                paramChanged.append(((m1id,m2id),paramMatching))
        return (p1, p2, v1Andv2, v1Only, v2Only, paramChanged)                    
                        
    def getFirstCommonVersion(self, v1, v2):
        """ Returns the first version that it is common to both v1 and v2 
        Parameters
        ----------
        - v1 : 'int'
         version number 1

        - v2 : 'int'
         version number 2

        """
        if (v1<=0 or v2<=0):
            return 0
        
        t1 = set()
        t1.add(v1)
        t = self.actionMap[v1].parent
        while  t != 0:
            t1.add(t)
            t = self.actionMap[t].parent
        
        t = v2
        while t != 0:
            if t in t1:
                return t
            t = self.actionMap[t].parent
        return 0
    
    def getLastCommonVersion(self, v):
        """getLastCommonVersion(v: Vistrail) -> int
        Returns the last version that is common to this vistrail and v
  
        """
        # TODO:  There HAS to be a better way to do this...
        common = []
        for action in self.actionMap:
            if(v.hasVersion(action.timestep)):
                common.append(action.timestep)
                
        timestep = 0
        for time in common:
            if time > timestep:
                timestep = time

        return timestep  

    def general_action_chain(self, v1, v2):
        """general_action_chain(v1, v2): Returns an action that turns
        pipeline v1 into v2."""

        return core.db.io.getPathAsAction(self, v1, v2)
        
    def actionChain(self, t, start=0):
        """ actionChain(t:int, start=0) -> [Action]  
        Returns the action chain (list of Action)  necessary to recreate a 
        pipeline from a  certain time
                      
        """
        assert t >= start
        if t == start:
            return []
        result = []
        action = self.actionMap[t]
        
        while 1:
            result.append(action)
            if action.timestep == start:
                break
            if action.parent == start:
                if start != 0:
                    action = self.actionMap[action.parent]
                break
            action = self.actionMap[action.parent]
        result.reverse()
        return result
    
    def update_object(self, obj, **kwargs):
        self.db_update_object(obj, **kwargs)

    def add_action(self, action, parent, session=None):
        # FIXME: this should go to core.db.io
        Action.convert(action)
        if action.id < 0:
            action.id = self.idScope.getNewId(action.vtType)
        action.prevId = parent
        action.date = self.getDate()
        action.user = self.getUser()
        if session is not None:
            action.session = session
        for op in action.operations:
            if op.id < 0:
                op.id = self.idScope.getNewId('operation')
                if op.vtType == 'add' or op.vtType == 'change':
                    self.db_add_object(op.db_data)
        self.addVersion(action)                

    def add_abstraction(self, abstraction):
        Abstraction.convert(abstraction)
        if abstraction.id < 0:
            abstraction.id = self.idScope.getNewId(abstraction.vtType)
            action_remap = {}
            for action in abstraction.actions.itervalues():
                if action.id < 0:
                    new_id = abstraction.idScope.getNewId(action.vtType)
                    action_remap[action.id] = new_id
                    action.id = new_id
                action.date = self.getDate()
                action.user = self.getUser()
                for op in action.operations:
                    if op.id < 0:
                        op.id = self.idScope.getNewId('operation')
            for action in abstraction.actions.itervalues():
                if action.prevId < 0:
                    action.prevId = action_remap[action.prevId]

        self.db_add_abstraction(abstraction)

    def getLastActions(self, n):
        """ getLastActions(n: int) -> list of ids
        Returns the last n actions performed
        """
        last_n = []
        num_actions = len(self.actionMap)
        if num_actions < n:
            n = num_actions
        if n > 0:
            sorted_keys = sorted(self.actionMap.keys())
            last_n = sorted_keys[num_actions-n:num_actions-1]
        return last_n

    def hasVersion(self, version):
        """hasVersion(version:int) -> boolean
        Returns True if version with given timestamp exists

        """
        return version in self.actionMap
    
    def addVersion(self, action):
        """ addVersion(action: Action) -> None 
        Adds new version to vistrail
          
        """
        if action.timestep in self.actionMap:
            raise VistrailsInternalError("existing timestep")
        self.db_add_action(action)
        self.changed = True

        # signal to update explicit tree
        self.tree.addVersion(action.id, action.prevId)

    def hasTag(self, tag):
        """ hasTag(tag) -> boolean 
        Returns True if a tag with given name or number exists
       
        """
        if type(tag) == type(0) or type(tag) == type(0L):
            return tag in self.tagMap
        elif type(tag) == type('str'):
            return self.has_tag_with_name(tag)
        
    def addTag(self, version_name, version_number):
        """addTag(version_name, version_number) -> None
        Adds new tag to vistrail
          
        """
        if version_name == '':
            return None
        if version_number in self.tagMap:
            DebugPrint.log("Version is already tagged")
            raise VersionAlreadyTagged()
        if self.has_tag_with_name(version_name):
            DebugPrint.log("Tag already exists")
            raise TagExists()
        tag = Tag(id=long(version_number),
                  name=version_name,
                  )
        self.db_add_tag(tag)
        self.changed = True
        
    def changeTag(self, version_name, version_number):
        """changeTag(version_name, version_number) -> None        
        Changes the old tag of version_number to version_name in the
        vistrail.  If version_name is empty, this version will be
        untagged.
                  
        """
        if not version_number in self.tagMap:
            DebugPrint.log("Version is not tagged")
            raise VersionNotTagged()
        if self.tagMap[version_number].name == version_name:
            return None
        if self.has_tag_with_name(version_name):
            DebugPrint.log("Tag already exists")
            raise TagExists()
        self.db_delete_tag(self.tagMap[version_number])
        if version_name != '':
            tag = Tag(id=long(version_number),
                      name=version_name,
                      )
            self.db_add_tag(tag)
        self.changed = True

    def change_annotation(self, key, value, version_number):
        """ change_annotation(key:str, value:str, version_number:long) -> None 
        Changes the annotation of (key, value) for version version_number
                  
        """
        
        if version_number in self.actionMap:
            action = self.actionMap[version_number]
            if action.has_annotation_with_key(key):
                old_annotation = action.get_annotation_by_key(key)
                if old_annotation.value == value:
                    return False
                action.delete_annotation(old_annotation)
            if value.strip() != '':
                annotation = \
                    Annotation(id=self.idScope.getNewId(Annotation.vtType),
                               key=key,
                               value=value,
                               )
                action.add_annotation(annotation)
            self.changed = True
            return True
        return False

    def change_notes(self, notes, version_number):
        """ change_notes(notes:str, version_number:int) -> None 
        Changes the notes of a version
                  
        """
    
        return self.change_annotation(Action.ANNOTATION_NOTES, 
                                 notes, version_number)
        
    def change_description(self, description, version_number): 
        """ change_description(description:str, version_number:int) -> None 
        Changes the description of a version
                  
        """
       
        return self.change_annotation(Action.ANNOTATION_DESCRIPTION, 
                                 description, version_number)

    def get_description(self, version_number):
        """ get_description(version_number: int) -> str
        Compute the description of a version
        
        """
        description = ""
        if version_number in self.actionMap:
            action = self.actionMap[version_number]
            # if a description has been manually set, return that value
            if action.description is not None:
                return action.description
            ops = action.operations
            added_modules = 0
            added_functions = 0
            added_connections = 0
            moved_modules = 0
            changed_parameters = 0
            deleted_modules = 0
            deleted_connections = 0
            deleted_parameters = 0
            for op in ops:
                if op.vtType == 'add':
                    if op.what == 'module':
                        added_modules+=1
                    elif op.what == 'connection':
                        added_connections+=1
                    elif op.what == 'function':
                        added_functions+=1
                elif op.vtType == 'change':
                    if op.what == 'parameter':
                        changed_parameters+=1
                    elif op.what == 'location':
                        moved_modules+=1
                elif op.vtType == 'delete':
                    if op.what == 'module':
                        deleted_modules+=1
                    elif op.what == 'connection':
                        deleted_connections+=1
                    elif op.what == 'parameter':
                        deleted_parameters+=1
                else:
                    raise Exception("Unknown operation type '%s'" % op.vtType)

            if added_modules:
                description = "Added module"
                if added_modules > 1:
                    description += "s"
            elif added_connections:
                description = "Added connection"
            elif added_functions:
                description = "Added function"
            elif moved_modules:
                description = "Moved module"
                if moved_modules > 1:
                    description += "s"
            elif changed_parameters:
                description = "Changed parameter"
                if changed_parameters > 1:
                    description += "s"
            elif deleted_modules:
                description = "Deleted module"
                if deleted_modules > 1:
                    description += "s"
            elif deleted_connections:
                description = "Deleted connection"
                if deleted_connections > 1:
                    description += "s"
            elif deleted_parameters:
                description = "Deleted parameter"
                if deleted_parameters > 1:
                    description += "s"
                
        return description

    # FIXME: remove this function (left here only for transition)
    def getVersionGraph(self):
        """getVersionGraph() -> Graph 
        Returns the version graph
        
        """
        result = Graph()
        result.add_vertex(0)

        # the sorting is for the display using graphviz
        # we want to always add nodes from left to right
        for action in sorted(self.actionMap.itervalues(), 
                             key=lambda x: x.timestep):
            # We need to check the presence of the parent's timestep
            # on the graph because it might have been previously
            # pruned. Remember that pruning is only marked for the
            # topmost invisible action.
            if (action.parent in result.vertices and
                action.prune != 1):
                result.add_edge(action.parent,
                                action.timestep,
                               0)
        return result

    # FIXME: remove this function (left here only for transition)
    # the idea of terse graph does not need 
    # to be so intrinsic to be in the Vistrail 
    # class. It can be treated in a higher layer.
    def getTerseGraph(self):
        """ getTerseGraph() -> Graph 
        Returns the version graph skiping the non-tagged internal nodes. 
        Branches are kept.
        
        """
        complete = self.getVersionGraph()
        x = []
        x.append(0)
        while len(x):
            current = x.pop()
            efrom = complete.edges_from(current)
            eto = complete.edges_to(current)

            for (e1,e2) in efrom:
                x.append(e1)
            if len(efrom) == 1 and len(eto) == 1 and not self.hasTag(current):
                to_me = eto[0][0]
                from_me = efrom[0][0]
                complete.delete_edge(current, from_me, None)
                complete.change_edge(to_me, current, from_me, None, -1)
                # complete.delete_edge(to_me, current, None)
                # complete.add_edge(to_me, from_me, -1)
                complete.delete_vertex(current)
        return complete

    def getCurrentGraph(self):
        """getCurrentGraph() -> Graph
        returns the current version graph. if there is not one, returns the
        terse graph instead 

        """
        if not self.currentGraph:
            self.currentGraph=copy.copy(self.getTerseGraph())
        return self.currentGraph

    def setCurrentGraph(self, newGraph):
        """setCurrentGraph(newGraph: Graph) -> None
        Sets a copy of newGraph as the currentGraph. 

        """
        self.currentGraph=copy.copy(newGraph)

    def getDate(self):
  """ getDate() -> str - Returns the current date and time. """
    #  return time.strftime("%d %b %Y %H:%M:%S", time.localtime())
        return datetime.datetime.now()
    
    def getUser(self):
  """ getUser() -> str - Returns the username. """
  return getpass.getuser()

    def serialize(self, filename):
        pass

    def pruneVersion(self, version):
        """ pruneVersion(version: int) -> None
        Add a version into the prunedVersion set
        
        """
        if version!=0: # not root
            def delete_tag(version):
                if version in self.tagMap:
                    self.db_delete_tag(self.tagMap[version])
            current_graph = self.getVersionGraph()
            current_graph.dfs(vertex_set=[version], enter_vertex=delete_tag)
            self.actionMap[version].prune = 1

            # self.prunedVersions.add(version)
    def hideVersion(self, version):
        """ hideVersion(version: int) -> None
        Set the prune flag for the version

        """
        if version != 0:
            self.actionMap[version].prune = 1

    def showVersion(self, version):
        """ showVersion(version: int) -> None
        Set the prune flag for the version

        """
        if version != 0:
            self.actionMap[version].prune = 0

    def expandVersion(self, version):
        """ expandVersion(version: int) -> None
        Set the expand flag for the version
        
        """
        if version!=0: # not root
            self.actionMap[version].expand = 1

    def collapseVersion(self, version):
        """ collapseVersion(version: int) -> None
        Reset the expand flag for the version
        
        """
        if version!=0:
            self.actionMap[version].expand = 0

    def setSavedQueries(self, savedQueries):
        """ setSavedQueries(savedQueries: list of (str, str, str)) -> None
        Set the saved queries of this vistrail
        
        """
        self.savedQueries = savedQueries

    # Dispatch in runtime according to type
    getPipelineDispatcher = {}
    getPipelineDispatcher[type(0)] = getPipelineVersionNumber
    getPipelineDispatcher[type(0L)] = getPipelineVersionNumber
    getPipelineDispatcher[type('0')] = getPipelineVersionName
    getPipelineDispatcher[Tag] = getPipelineVersionTag

    class InvalidAbstraction(Exception):
        pass

    def create_abstraction(self,
                           pipeline_version,
                           subgraph,
                           abstraction_name):
        pipeline = self.getPipeline(pipeline_version)
        current_graph = pipeline.graph
        if not current_graph.topologically_contractible(subgraph):
            msg = "Abstraction violates DAG constraints."
            raise self.InvalidAbstraction(msg)
        input_ports = current_graph.connections_to_subgraph(subgraph)
        output_ports = current_graph.connections_from_subgraph(subgraph)

        # Recreate pipeline from empty version
        sub_pipeline = pipeline.get_subpipeline(subgraph)
        actions = sub_pipeline.dump_actions()

        for (frm, to, conn_id) in input_ports:
            fresh_id = sub_pipeline.fresh_module_id()
            m = Module()
            m.id = fresh_id
            m.location = copy.copy(pipeline.modules[frm].location)
            m.name = "InputPort"
            actions.append(m)

            c = core.vistrail.connection.Connection()
            fresh_id = sub_pipeline.fresh_connection_id()
            c.id = fresh_id

        raise Exception("not finished")
        
##############################################################################

class ExplicitExpandedVersionTree(object):
    """
    Keep explicit expanded and tersed version 
    trees.
    """
    def __init__(self, vistrail):
        self.vistrail = vistrail
        self.expandedVersionTree = Graph()
        self.expandedVersionTree.add_vertex(0)
        self.tersedVersionTree = Graph()

    def addVersion(self, id, prevId):
        # print "add version %d child of %d" % (id, prevId)
        self.expandedVersionTree.add_vertex(id)
        self.expandedVersionTree.add_edge(prevId,id,0)
    
    def getVersionTree(self):
        return self.expandedVersionTree
        
##############################################################################

class VersionAlreadyTagged(Exception):
    def __str__(self):
        return "Version is already tagged"
    pass

class TagExists(Exception):
    def __str__(self):
        return "Tag already exists"
    pass

class VersionNotTagged(Exception):
    def __str__(self):
        return "Version is not tagged"
    pass

##############################################################################
# Testing

import unittest
import copy
import random

class TestVistrail(unittest.TestCase):

    def create_vistrail(self):
        vistrail = Vistrail()

        m = Module(id=vistrail.idScope.getNewId(Module.vtType),
                   name='Float',
                   package='edu.utah.sci.vistrails.basic')
        add_op = AddOp(id=vistrail.idScope.getNewId(AddOp.vtType),
                       what=Module.vtType,
                       objectId=m.id,
                       data=m)
        function_id = vistrail.idScope.getNewId(ModuleFunction.vtType)
        function = ModuleFunction(id=function_id,
                                  name='value')
        change_op = ChangeOp(id=vistrail.idScope.getNewId(ChangeOp.vtType),
                             what=ModuleFunction.vtType,
                             oldObjId=2,
                             newObjId=function.real_id,
                             parentObjId=m.id,
                             parentObjType=Module.vtType,
                             data=function)
        param = ModuleParam(id=vistrail.idScope.getNewId(ModuleParam.vtType),
                            type='Integer',
                            val='1')
        delete_op = DeleteOp(id=vistrail.idScope.getNewId(DeleteOp.vtType),
                             what=ModuleParam.vtType,
                             objectId=param.real_id,
                             parentObjId=function.real_id,
                             parentObjType=ModuleFunction.vtType)

        action1 = Action(id=vistrail.idScope.getNewId(Action.vtType),
                         operations=[add_op])
        action2 = Action(id=vistrail.idScope.getNewId(Action.vtType),
                         operations=[change_op, delete_op])

        vistrail.add_action(action1, 0)
        vistrail.add_action(action2, action1.id)
        vistrail.addTag('first action', action1.id)
        vistrail.addTag('second action', action2.id)
        return vistrail

    def test_get_tag_by_name(self):
        v = self.create_vistrail()
        self.failUnlessRaises(KeyError, lambda: v.get_tag_by_name('not here'))
        v.get_tag_by_name('first action')
        v.get_tag_by_name('second action')

    def test_copy(self):
        v1 = self.create_vistrail()
        v2 = copy.copy(v1)
        v3 = v1.do_copy(True, v1.idScope, {})
        # FIXME add checks for equality

    def test_serialization(self):
        import core.db.io
        v1 = self.create_vistrail()
        xml_str = core.db.io.serialize(v1)
        v2 = core.db.io.unserialize(xml_str, Vistrail)
        # FIXME add checks for equality

    def test1(self):
        import core.vistrail
        from core.db.locator import XMLFileLocator
        import core.system
        v = XMLFileLocator(core.system.vistrails_root_directory() +
                           '/tests/resources/dummy.xml').load()
        #testing nodes in different branches
        v1 = 36
        v2 = 41
        p1 = v.getFirstCommonVersion(v1,v2)
        p2 = v.getFirstCommonVersion(v2,v1)
        self.assertEquals(p1,p2)
        
        #testing nodes in the same branch
        v1 = 15
        v2 = 36
        p1 = v.getFirstCommonVersion(v1,v2)
        p2 = v.getFirstCommonVersion(v2,v1)
        self.assertEquals(p1,p2)

        if p1 == 0 or p2 == 0:
            self.fail("vistrails tree is not single rooted.")

    def test2(self):
        import core.vistrail
        from core.db.locator import XMLFileLocator
        import core.system
        v = XMLFileLocator(core.system.vistrails_root_directory() +
                           '/tests/resources/dummy.xml').load()
        #testing diff
        v1 = 17
        v2 = 27
        v3 = 22
        v.getPipelineDiff(v1,v2)
        v.getPipelineDiff(v1,v3)

    def test_empty_action_chain(self):
        """Tests calling action chain on empty version."""
        v = Vistrail()
        p = v.getPipeline(0)

    def test_empty_action_chain_2(self):
        from core.db.locator import XMLFileLocator
        import core.system
        v = XMLFileLocator(core.system.vistrails_root_directory() +
                           '/tests/resources/dummy.xml').load()
        assert v.actionChain(17, 17) == []

    def test_get_version_negative_one(self):
        """Tests getting the 'no version' vistrail. This should raise
        VistrailsDBException.

        """
        v = Vistrail()
        self.assertRaises(VistrailsDBException, lambda: v.getPipeline(-1))

    def test_version_graph(self):
        from core.db.locator import XMLFileLocator
        import core.system
        v = XMLFileLocator(core.system.vistrails_root_directory() +
                           '/tests/resources/dummy.xml').load()
        v.getVersionGraph()

    def test_plugin_info(self):
        import core.db.io
        plugin_info_str = "this is a test of plugin_info"
        v1 = self.create_vistrail()
        v1.plugin_info = plugin_info_str
        xml_str = core.db.io.serialize(v1)
        v2 = core.db.io.unserialize(xml_str, Vistrail)
        assert plugin_info_str == v2.plugin_info

    def test_database_info(self):
        import core.db.io
        database_info_str = "db.hostname.edu:3306:TABLE_NAME"
        v1 = self.create_vistrail()
        v1.database_info = database_info_str
        xml_str = core.db.io.serialize(v1)
        v2 = core.db.io.unserialize(xml_str, Vistrail)
        assert database_info_str == v2.database_info

    def test_plugin_data(self):
        import core.db.io
        v1 = self.create_vistrail()
        plugin_data_str = "testing plugin_data"
        p = PluginData(id=v1.idScope.getNewId(PluginData.vtType),
                       data=plugin_data_str)
        add_op = AddOp(id=v1.idScope.getNewId(AddOp.vtType),
                       what=PluginData.vtType,
                       objectId=p.id,
                       data=p)
        action = Action(id=v1.idScope.getNewId(Action.vtType),
                        operations=[add_op])
        v1.add_action(action, 0)
        workflow = v1.getPipeline(action.id)
        p2 = workflow.plugin_datas[0]
        assert plugin_data_str == p2.data

    def test_inverse(self):
        """Test if inverses and general_action_chain are working by
        doing a lot of action-based transformations on a pipeline and
        checking against another way of getting the same one."""
        def check_pipelines(p, p2):
            if p != p2:
                p.show_comparison(p2)
                return False
            return True
        from core.db.locator import XMLFileLocator
        from core.db.locator import FileLocator
        import core.system
        import sys

        def do_test(filename, locator_class):
            v = locator_class(core.system.vistrails_root_directory() +
                               filename).load()
            version_ids = v.actionMap.keys()
            old_v = random.choice(version_ids)
            p = v.getPipeline(old_v)
            for i in xrange(10):
                new_v = random.choice(version_ids)
                p2 = v.getPipeline(new_v)
                a = v.general_action_chain(old_v, new_v)
                p.perform_action(a)
                if not check_pipelines(p, p2):
                    print i
                    
                assert p == p2
                old_v = new_v
                sys.stderr.flush()

        do_test('/tests/resources/dummy.xml', XMLFileLocator)
        do_test('/tests/resources/terminator.vt', FileLocator)

#     def test_abstraction(self):
#         import core.vistrail
#         import core.xml_parser
#         parser = core.xml_parser.XMLParser()
#         parser.openVistrail(core.system.vistrails_root_directory() +
#                             '/tests/resources/ect.xml')
#         v = parser.getVistrail()
#         parser.closeVistrail()
#         #testing diff
#         p = v.getPipeline('WindowedSync (lambda-mu) Error')
#         version = v.get_version_number('WindowedSync (lambda-mu) Error')
#         sub = p.graph.subgraph([43, 45])
#         v.create_abstraction(version, sub, "FOOBAR")

if __name__ == '__main__':
    unittest.main()
