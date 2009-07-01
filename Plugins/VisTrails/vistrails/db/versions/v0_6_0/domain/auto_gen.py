
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

"""generated automatically by auto_dao.py"""

import copy

class DBPortSpec(object):

    vtType = 'portSpec'

    def __init__(self, id=None, name=None, type=None, spec=None):
        self.__db_id = id
        self.__db_name = name
        self.__db_type = type
        self.__db_spec = spec
        self.is_dirty = True
        self.is_new = True
    
    def __copy__(self):
        cp = DBPortSpec()
        cp.db_id = self.db_id
        cp.db_name = self.db_name
        cp.db_type = self.db_type
        cp.db_spec = self.db_spec
        cp.is_dirty = self.is_dirty
        cp.is_new = self.is_new
        return cp

    def copy_with_new_ids(self, parent, id_scope, id_remap):
        cp = copy.copy(self)
        children = cp.db_children(parent, False)
        for (obj, _, _) in children:
            new_id = id_scope.getNewId(obj.vtType)
            id_remap[(obj.vtType, obj.db_id)] = new_id
            obj.db_id = new_id
        for (obj, _, _) in children:
            if id_remap.has_key(('vistrail', obj.db_vistrailId)):
                obj.db_vistrailId = id_remap[('vistrail', obj.db_vistrailId)]
        
    def db_children(self, parent=(None,None), orphan=False):
        children = []
        children.append((self, parent[0], parent[1]))
        return children
    def has_changes(self):
        if self.is_dirty:
            return True
        return False
    def __get_db_id(self):
        return self.__db_id
    def __set_db_id(self, id):
        self.__db_id = id
        self.is_dirty = True
    db_id = property(__get_db_id, __set_db_id)
    def db_add_id(self, id):
        self.__db_id = id
    def db_change_id(self, id):
        self.__db_id = id
    def db_delete_id(self, id):
        self.__db_id = None
    
    def __get_db_name(self):
        return self.__db_name
    def __set_db_name(self, name):
        self.__db_name = name
        self.is_dirty = True
    db_name = property(__get_db_name, __set_db_name)
    def db_add_name(self, name):
        self.__db_name = name
    def db_change_name(self, name):
        self.__db_name = name
    def db_delete_name(self, name):
        self.__db_name = None
    
    def __get_db_type(self):
        return self.__db_type
    def __set_db_type(self, type):
        self.__db_type = type
        self.is_dirty = True
    db_type = property(__get_db_type, __set_db_type)
    def db_add_type(self, type):
        self.__db_type = type
    def db_change_type(self, type):
        self.__db_type = type
    def db_delete_type(self, type):
        self.__db_type = None
    
    def __get_db_spec(self):
        return self.__db_spec
    def __set_db_spec(self, spec):
        self.__db_spec = spec
        self.is_dirty = True
    db_spec = property(__get_db_spec, __set_db_spec)
    def db_add_spec(self, spec):
        self.__db_spec = spec
    def db_change_spec(self, spec):
        self.__db_spec = spec
    def db_delete_spec(self, spec):
        self.__db_spec = None
    
    def getPrimaryKey(self):
        return self.__db_id

class DBModule(object):

    vtType = 'module'

    def __init__(self, id=None, cache=None, name=None, package=None, version=None, location=None, functions=None, annotations=None, portSpecs=None):
        self.__db_id = id
        self.__db_cache = cache
        self.__db_name = name
        self.__db_package = package
        self.__db_version = version
        self.__db_location = location
        if functions is None:
            self.__db_functions = []
        else:
            self.__db_functions = functions
        self.db_annotations_key_index = {}
        if annotations is None:
            self.__db_annotations = {}
        else:
            self.__db_annotations = annotations
            for v in self.__db_annotations.itervalues():
                self.db_annotations_key_index[v.db_key] = v
        self.db_portSpecs_name_index = {}
        if portSpecs is None:
            self.__db_portSpecs = {}
        else:
            self.__db_portSpecs = portSpecs
            for v in self.__db_portSpecs.itervalues():
                self.db_portSpecs_name_index[v.db_name] = v
        self.is_dirty = True
        self.is_new = True
    
    def __copy__(self):
        cp = DBModule()
        cp.db_id = self.db_id
        cp.db_cache = self.db_cache
        cp.db_name = self.db_name
        cp.db_package = self.db_package
        cp.db_version = self.db_version
        cp.db_location = self.db_location
        if self.db_functions is None:
            cp.db_functions = None
        else:
            cp.db_functions = [copy.copy(v) for v in self.db_functions]
        if self.db_annotations is None:
            cp.db_annotations = None
        else:
            cp.db_annotations = dict([(k,copy.copy(v)) for (k,v) in self.db_annotations.iteritems()])
            for v in cp.__db_annotations.itervalues():
                cp.db_annotations_key_index[v.db_key] = v
        if self.db_portSpecs is None:
            cp.db_portSpecs = None
        else:
            cp.db_portSpecs = dict([(k,copy.copy(v)) for (k,v) in self.db_portSpecs.iteritems()])
            for v in cp.__db_portSpecs.itervalues():
                cp.db_portSpecs_name_index[v.db_name] = v
        cp.is_dirty = self.is_dirty
        cp.is_new = self.is_new
        return cp

    def copy_with_new_ids(self, parent, id_scope, id_remap):
        cp = copy.copy(self)
        children = cp.db_children(parent, False)
        for (obj, _, _) in children:
            new_id = id_scope.getNewId(obj.vtType)
            id_remap[(obj.vtType, obj.db_id)] = new_id
            obj.db_id = new_id
        for (obj, _, _) in children:
            if id_remap.has_key(('vistrail', obj.db_vistrailId)):
                obj.db_vistrailId = id_remap[('vistrail', obj.db_vistrailId)]
        
    def db_children(self, parent=(None,None), orphan=False):
        children = []
        if self.db_location is not None:
            children.extend(self.db_location.db_children((self.vtType, self.db_id), orphan))
            if orphan:
                self.db_location = None
        to_del = []
        for child in self.db_functions:
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
            if orphan:
                to_del.append(child)
        for child in to_del:
            self.db_delete_function(child)
        to_del = []
        for child in self.db_annotations.itervalues():
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
            if orphan:
                to_del.append(child)
        for child in to_del:
            self.db_delete_annotation(child)
        to_del = []
        for child in self.db_portSpecs.itervalues():
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
            if orphan:
                to_del.append(child)
        for child in to_del:
            self.db_delete_portSpec(child)
        children.append((self, parent[0], parent[1]))
        return children
    def has_changes(self):
        if self.is_dirty:
            return True
        if self.db_location is not None and self.db_location.has_changes():
            return True
        for child in self.db_functions:
            if child.has_changes():
                return True
        for child in self.db_annotations.itervalues():
            if child.has_changes():
                return True
        for child in self.db_portSpecs.itervalues():
            if child.has_changes():
                return True
        return False
    def __get_db_id(self):
        return self.__db_id
    def __set_db_id(self, id):
        self.__db_id = id
        self.is_dirty = True
    db_id = property(__get_db_id, __set_db_id)
    def db_add_id(self, id):
        self.__db_id = id
    def db_change_id(self, id):
        self.__db_id = id
    def db_delete_id(self, id):
        self.__db_id = None
    
    def __get_db_cache(self):
        return self.__db_cache
    def __set_db_cache(self, cache):
        self.__db_cache = cache
        self.is_dirty = True
    db_cache = property(__get_db_cache, __set_db_cache)
    def db_add_cache(self, cache):
        self.__db_cache = cache
    def db_change_cache(self, cache):
        self.__db_cache = cache
    def db_delete_cache(self, cache):
        self.__db_cache = None
    
    def __get_db_name(self):
        return self.__db_name
    def __set_db_name(self, name):
        self.__db_name = name
        self.is_dirty = True
    db_name = property(__get_db_name, __set_db_name)
    def db_add_name(self, name):
        self.__db_name = name
    def db_change_name(self, name):
        self.__db_name = name
    def db_delete_name(self, name):
        self.__db_name = None
    
    def __get_db_package(self):
        return self.__db_package
    def __set_db_package(self, package):
        self.__db_package = package
        self.is_dirty = True
    db_package = property(__get_db_package, __set_db_package)
    def db_add_package(self, package):
        self.__db_package = package
    def db_change_package(self, package):
        self.__db_package = package
    def db_delete_package(self, package):
        self.__db_package = None
    
    def __get_db_version(self):
        return self.__db_version
    def __set_db_version(self, version):
        self.__db_version = version
        self.is_dirty = True
    db_version = property(__get_db_version, __set_db_version)
    def db_add_version(self, version):
        self.__db_version = version
    def db_change_version(self, version):
        self.__db_version = version
    def db_delete_version(self, version):
        self.__db_version = None
    
    def __get_db_location(self):
        return self.__db_location
    def __set_db_location(self, location):
        self.__db_location = location
        self.is_dirty = True
    db_location = property(__get_db_location, __set_db_location)
    def db_add_location(self, location):
        self.__db_location = location
    def db_change_location(self, location):
        self.__db_location = location
    def db_delete_location(self, location):
        self.__db_location = None
    
    def __get_db_functions(self):
        return self.__db_functions
    def __set_db_functions(self, functions):
        self.__db_functions = functions
        self.is_dirty = True
    db_functions = property(__get_db_functions, __set_db_functions)
    def db_get_functions(self):
        return self.__db_functions
    def db_add_function(self, function):
        self.is_dirty = True
        self.__db_functions.append(function)
    def db_change_function(self, function):
        self.is_dirty = True
        found = False
        for i in xrange(len(self.__db_functions)):
            if self.__db_functions[i].db_id == function.db_id:
                self.__db_functions[i] = function
                found = True
                break
        if not found:
            self.__db_functions.append(function)
    def db_delete_function(self, function):
        self.is_dirty = True
        for i in xrange(len(self.__db_functions)):
            if self.__db_functions[i].db_id == function.db_id:
                del self.__db_functions[i]
                break
    def db_get_function(self, key):
        for i in xrange(len(self.__db_functions)):
            if self.__db_functions[i].db_id == key:
                return self.__db_functions[i]
        return None
    
    def __get_db_annotations(self):
        return self.__db_annotations
    def __set_db_annotations(self, annotations):
        self.__db_annotations = annotations
        self.is_dirty = True
    db_annotations = property(__get_db_annotations, __set_db_annotations)
    def db_get_annotations(self):
        return self.__db_annotations.values()
    def db_add_annotation(self, annotation):
        self.is_dirty = True
        self.__db_annotations[annotation.db_id] = annotation
        self.db_annotations_key_index[annotation.db_key] = annotation
    def db_change_annotation(self, annotation):
        self.is_dirty = True
        self.__db_annotations[annotation.db_id] = annotation
        self.db_annotations_key_index[annotation.db_key] = annotation
    def db_delete_annotation(self, annotation):
        self.is_dirty = True
        del self.__db_annotations[annotation.db_id]
        del self.db_annotations_key_index[annotation.db_key]
    def db_get_annotation(self, key):
        if self.__db_annotations.has_key(key):
            return self.__db_annotations[key]
        return None
    def db_get_annotation_by_key(self, key):
        return self.db_annotations_key_index[key]
    def db_has_annotation_with_key(self, key):
        return self.db_annotations_key_index.has_key(key)
    
    def __get_db_portSpecs(self):
        return self.__db_portSpecs
    def __set_db_portSpecs(self, portSpecs):
        self.__db_portSpecs = portSpecs
        self.is_dirty = True
    db_portSpecs = property(__get_db_portSpecs, __set_db_portSpecs)
    def db_get_portSpecs(self):
        return self.__db_portSpecs.values()
    def db_add_portSpec(self, portSpec):
        self.is_dirty = True
        self.__db_portSpecs[portSpec.db_id] = portSpec
        self.db_portSpecs_name_index[portSpec.db_name] = portSpec
    def db_change_portSpec(self, portSpec):
        self.is_dirty = True
        self.__db_portSpecs[portSpec.db_id] = portSpec
        self.db_portSpecs_name_index[portSpec.db_name] = portSpec
    def db_delete_portSpec(self, portSpec):
        self.is_dirty = True
        del self.__db_portSpecs[portSpec.db_id]
        del self.db_portSpecs_name_index[portSpec.db_name]
    def db_get_portSpec(self, key):
        if self.__db_portSpecs.has_key(key):
            return self.__db_portSpecs[key]
        return None
    def db_get_portSpec_by_name(self, key):
        return self.db_portSpecs_name_index[key]
    def db_has_portSpec_with_name(self, key):
        return self.db_portSpecs_name_index.has_key(key)
    
    def getPrimaryKey(self):
        return self.__db_id

class DBTag(object):

    vtType = 'tag'

    def __init__(self, id=None, name=None):
        self.__db_id = id
        self.__db_name = name
        self.is_dirty = True
        self.is_new = True
    
    def __copy__(self):
        cp = DBTag()
        cp.db_id = self.db_id
        cp.db_name = self.db_name
        cp.is_dirty = self.is_dirty
        cp.is_new = self.is_new
        return cp

    def copy_with_new_ids(self, parent, id_scope, id_remap):
        cp = copy.copy(self)
        children = cp.db_children(parent, False)
        for (obj, _, _) in children:
            new_id = id_scope.getNewId(obj.vtType)
            id_remap[(obj.vtType, obj.db_id)] = new_id
            obj.db_id = new_id
        for (obj, _, _) in children:
            if id_remap.has_key(('action', obj.db_id)):
                obj.db_id = id_remap[('action', obj.db_id)]
            if id_remap.has_key(('vistrail', obj.db_vistrailId)):
                obj.db_vistrailId = id_remap[('vistrail', obj.db_vistrailId)]
        
    def db_children(self, parent=(None,None), orphan=False):
        children = []
        children.append((self, parent[0], parent[1]))
        return children
    def has_changes(self):
        if self.is_dirty:
            return True
        return False
    def __get_db_id(self):
        return self.__db_id
    def __set_db_id(self, id):
        self.__db_id = id
        self.is_dirty = True
    db_id = property(__get_db_id, __set_db_id)
    def db_add_id(self, id):
        self.__db_id = id
    def db_change_id(self, id):
        self.__db_id = id
    def db_delete_id(self, id):
        self.__db_id = None
    
    def __get_db_name(self):
        return self.__db_name
    def __set_db_name(self, name):
        self.__db_name = name
        self.is_dirty = True
    db_name = property(__get_db_name, __set_db_name)
    def db_add_name(self, name):
        self.__db_name = name
    def db_change_name(self, name):
        self.__db_name = name
    def db_delete_name(self, name):
        self.__db_name = None
    
    def getPrimaryKey(self):
        return self.__db_id

class DBPort(object):

    vtType = 'port'

    def __init__(self, id=None, type=None, moduleId=None, moduleName=None, sig=None):
        self.__db_id = id
        self.__db_type = type
        self.__db_moduleId = moduleId
        self.__db_moduleName = moduleName
        self.__db_sig = sig
        self.is_dirty = True
        self.is_new = True
    
    def __copy__(self):
        cp = DBPort()
        cp.db_id = self.db_id
        cp.db_type = self.db_type
        cp.db_moduleId = self.db_moduleId
        cp.db_moduleName = self.db_moduleName
        cp.db_sig = self.db_sig
        cp.is_dirty = self.is_dirty
        cp.is_new = self.is_new
        return cp

    def copy_with_new_ids(self, parent, id_scope, id_remap):
        cp = copy.copy(self)
        children = cp.db_children(parent, False)
        for (obj, _, _) in children:
            new_id = id_scope.getNewId(obj.vtType)
            id_remap[(obj.vtType, obj.db_id)] = new_id
            obj.db_id = new_id
        for (obj, _, _) in children:
            if id_remap.has_key(('module', obj.db_moduleId)):
                obj.db_moduleId = id_remap[('module', obj.db_moduleId)]
            if id_remap.has_key(('vistrail', obj.db_vistrailId)):
                obj.db_vistrailId = id_remap[('vistrail', obj.db_vistrailId)]
        
    def db_children(self, parent=(None,None), orphan=False):
        children = []
        children.append((self, parent[0], parent[1]))
        return children
    def has_changes(self):
        if self.is_dirty:
            return True
        return False
    def __get_db_id(self):
        return self.__db_id
    def __set_db_id(self, id):
        self.__db_id = id
        self.is_dirty = True
    db_id = property(__get_db_id, __set_db_id)
    def db_add_id(self, id):
        self.__db_id = id
    def db_change_id(self, id):
        self.__db_id = id
    def db_delete_id(self, id):
        self.__db_id = None
    
    def __get_db_type(self):
        return self.__db_type
    def __set_db_type(self, type):
        self.__db_type = type
        self.is_dirty = True
    db_type = property(__get_db_type, __set_db_type)
    def db_add_type(self, type):
        self.__db_type = type
    def db_change_type(self, type):
        self.__db_type = type
    def db_delete_type(self, type):
        self.__db_type = None
    
    def __get_db_moduleId(self):
        return self.__db_moduleId
    def __set_db_moduleId(self, moduleId):
        self.__db_moduleId = moduleId
        self.is_dirty = True
    db_moduleId = property(__get_db_moduleId, __set_db_moduleId)
    def db_add_moduleId(self, moduleId):
        self.__db_moduleId = moduleId
    def db_change_moduleId(self, moduleId):
        self.__db_moduleId = moduleId
    def db_delete_moduleId(self, moduleId):
        self.__db_moduleId = None
    
    def __get_db_moduleName(self):
        return self.__db_moduleName
    def __set_db_moduleName(self, moduleName):
        self.__db_moduleName = moduleName
        self.is_dirty = True
    db_moduleName = property(__get_db_moduleName, __set_db_moduleName)
    def db_add_moduleName(self, moduleName):
        self.__db_moduleName = moduleName
    def db_change_moduleName(self, moduleName):
        self.__db_moduleName = moduleName
    def db_delete_moduleName(self, moduleName):
        self.__db_moduleName = None
    
    def __get_db_sig(self):
        return self.__db_sig
    def __set_db_sig(self, sig):
        self.__db_sig = sig
        self.is_dirty = True
    db_sig = property(__get_db_sig, __set_db_sig)
    def db_add_sig(self, sig):
        self.__db_sig = sig
    def db_change_sig(self, sig):
        self.__db_sig = sig
    def db_delete_sig(self, sig):
        self.__db_sig = None
    
    def getPrimaryKey(self):
        return self.__db_id

class DBLog(object):

    vtType = 'log'

    def __init__(self, id=None, workflow_execs=None, machines=None):
        self.__db_id = id
        if workflow_execs is None:
            self.__db_workflow_execs = {}
        else:
            self.__db_workflow_execs = workflow_execs
        if machines is None:
            self.__db_machines = {}
        else:
            self.__db_machines = machines
        self.is_dirty = True
        self.is_new = True
    
    def __copy__(self):
        cp = DBLog()
        cp.db_id = self.db_id
        if self.db_workflow_execs is None:
            cp.db_workflow_execs = None
        else:
            cp.db_workflow_execs = dict([(k,copy.copy(v)) for (k,v) in self.db_workflow_execs.iteritems()])
        if self.db_machines is None:
            cp.db_machines = None
        else:
            cp.db_machines = dict([(k,copy.copy(v)) for (k,v) in self.db_machines.iteritems()])
        cp.is_dirty = self.is_dirty
        cp.is_new = self.is_new
        return cp

    def copy_with_new_ids(self, parent, id_scope, id_remap):
        cp = copy.copy(self)
        children = cp.db_children(parent, False)
        for (obj, _, _) in children:
            new_id = id_scope.getNewId(obj.vtType)
            id_remap[(obj.vtType, obj.db_id)] = new_id
            obj.db_id = new_id
        for (obj, _, _) in children:
            if id_remap.has_key(('vistrail', obj.db_vistrailId)):
                obj.db_vistrailId = id_remap[('vistrail', obj.db_vistrailId)]
        
    def db_children(self, parent=(None,None), orphan=False):
        children = []
        to_del = []
        for child in self.db_workflow_execs.itervalues():
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
            if orphan:
                to_del.append(child)
        for child in to_del:
            self.db_delete_workflow_exec(child)
        to_del = []
        for child in self.db_machines.itervalues():
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
            if orphan:
                to_del.append(child)
        for child in to_del:
            self.db_delete_machine(child)
        children.append((self, parent[0], parent[1]))
        return children
    def has_changes(self):
        if self.is_dirty:
            return True
        for child in self.db_workflow_execs.itervalues():
            if child.has_changes():
                return True
        for child in self.db_machines.itervalues():
            if child.has_changes():
                return True
        return False
    def __get_db_id(self):
        return self.__db_id
    def __set_db_id(self, id):
        self.__db_id = id
        self.is_dirty = True
    db_id = property(__get_db_id, __set_db_id)
    def db_add_id(self, id):
        self.__db_id = id
    def db_change_id(self, id):
        self.__db_id = id
    def db_delete_id(self, id):
        self.__db_id = None
    
    def __get_db_workflow_execs(self):
        return self.__db_workflow_execs
    def __set_db_workflow_execs(self, workflow_execs):
        self.__db_workflow_execs = workflow_execs
        self.is_dirty = True
    db_workflow_execs = property(__get_db_workflow_execs, __set_db_workflow_execs)
    def db_get_workflow_execs(self):
        return self.__db_workflow_execs.values()
    def db_add_workflow_exec(self, workflow_exec):
        self.is_dirty = True
        self.__db_workflow_execs[workflow_exec.db_id] = workflow_exec
    def db_change_workflow_exec(self, workflow_exec):
        self.is_dirty = True
        self.__db_workflow_execs[workflow_exec.db_id] = workflow_exec
    def db_delete_workflow_exec(self, workflow_exec):
        self.is_dirty = True
        del self.__db_workflow_execs[workflow_exec.db_id]
    def db_get_workflow_exec(self, key):
        if self.__db_workflow_execs.has_key(key):
            return self.__db_workflow_execs[key]
        return None
    
    def __get_db_machines(self):
        return self.__db_machines
    def __set_db_machines(self, machines):
        self.__db_machines = machines
        self.is_dirty = True
    db_machines = property(__get_db_machines, __set_db_machines)
    def db_get_machines(self):
        return self.__db_machines.values()
    def db_add_machine(self, machine):
        self.is_dirty = True
        self.__db_machines[machine.db_id] = machine
    def db_change_machine(self, machine):
        self.is_dirty = True
        self.__db_machines[machine.db_id] = machine
    def db_delete_machine(self, machine):
        self.is_dirty = True
        del self.__db_machines[machine.db_id]
    def db_get_machine(self, key):
        if self.__db_machines.has_key(key):
            return self.__db_machines[key]
        return None
    
    def getPrimaryKey(self):
        return self.__db_id

class DBMachine(object):

    vtType = 'machine'

    def __init__(self, id=None, name=None, os=None, architecture=None, processor=None, ram=None, module_execs=None):
        self.__db_id = id
        self.__db_name = name
        self.__db_os = os
        self.__db_architecture = architecture
        self.__db_processor = processor
        self.__db_ram = ram
        if module_execs is None:
            self.__db_module_execs = []
        else:
            self.__db_module_execs = module_execs
        self.is_dirty = True
        self.is_new = True
    
    def __copy__(self):
        cp = DBMachine()
        cp.db_id = self.db_id
        cp.db_name = self.db_name
        cp.db_os = self.db_os
        cp.db_architecture = self.db_architecture
        cp.db_processor = self.db_processor
        cp.db_ram = self.db_ram
        if self.db_module_execs is None:
            cp.db_module_execs = None
        else:
            cp.db_module_execs = [copy.copy(v) for v in self.db_module_execs]
        cp.is_dirty = self.is_dirty
        cp.is_new = self.is_new
        return cp

    def copy_with_new_ids(self, parent, id_scope, id_remap):
        cp = copy.copy(self)
        children = cp.db_children(parent, False)
        for (obj, _, _) in children:
            new_id = id_scope.getNewId(obj.vtType)
            id_remap[(obj.vtType, obj.db_id)] = new_id
            obj.db_id = new_id
        for (obj, _, _) in children:
            if id_remap.has_key(('vistrail', obj.db_vistrailId)):
                obj.db_vistrailId = id_remap[('vistrail', obj.db_vistrailId)]
        
    def db_children(self, parent=(None,None), orphan=False):
        children = []
        to_del = []
        for child in self.db_module_execs:
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
            if orphan:
                to_del.append(child)
        for child in to_del:
            self.db_delete_module_exec(child)
        children.append((self, parent[0], parent[1]))
        return children
    def has_changes(self):
        if self.is_dirty:
            return True
        for child in self.db_module_execs:
            if child.has_changes():
                return True
        return False
    def __get_db_id(self):
        return self.__db_id
    def __set_db_id(self, id):
        self.__db_id = id
        self.is_dirty = True
    db_id = property(__get_db_id, __set_db_id)
    def db_add_id(self, id):
        self.__db_id = id
    def db_change_id(self, id):
        self.__db_id = id
    def db_delete_id(self, id):
        self.__db_id = None
    
    def __get_db_name(self):
        return self.__db_name
    def __set_db_name(self, name):
        self.__db_name = name
        self.is_dirty = True
    db_name = property(__get_db_name, __set_db_name)
    def db_add_name(self, name):
        self.__db_name = name
    def db_change_name(self, name):
        self.__db_name = name
    def db_delete_name(self, name):
        self.__db_name = None
    
    def __get_db_os(self):
        return self.__db_os
    def __set_db_os(self, os):
        self.__db_os = os
        self.is_dirty = True
    db_os = property(__get_db_os, __set_db_os)
    def db_add_os(self, os):
        self.__db_os = os
    def db_change_os(self, os):
        self.__db_os = os
    def db_delete_os(self, os):
        self.__db_os = None
    
    def __get_db_architecture(self):
        return self.__db_architecture
    def __set_db_architecture(self, architecture):
        self.__db_architecture = architecture
        self.is_dirty = True
    db_architecture = property(__get_db_architecture, __set_db_architecture)
    def db_add_architecture(self, architecture):
        self.__db_architecture = architecture
    def db_change_architecture(self, architecture):
        self.__db_architecture = architecture
    def db_delete_architecture(self, architecture):
        self.__db_architecture = None
    
    def __get_db_processor(self):
        return self.__db_processor
    def __set_db_processor(self, processor):
        self.__db_processor = processor
        self.is_dirty = True
    db_processor = property(__get_db_processor, __set_db_processor)
    def db_add_processor(self, processor):
        self.__db_processor = processor
    def db_change_processor(self, processor):
        self.__db_processor = processor
    def db_delete_processor(self, processor):
        self.__db_processor = None
    
    def __get_db_ram(self):
        return self.__db_ram
    def __set_db_ram(self, ram):
        self.__db_ram = ram
        self.is_dirty = True
    db_ram = property(__get_db_ram, __set_db_ram)
    def db_add_ram(self, ram):
        self.__db_ram = ram
    def db_change_ram(self, ram):
        self.__db_ram = ram
    def db_delete_ram(self, ram):
        self.__db_ram = None
    
    def __get_db_module_execs(self):
        return self.__db_module_execs
    def __set_db_module_execs(self, module_execs):
        self.__db_module_execs = module_execs
        self.is_dirty = True
    db_module_execs = property(__get_db_module_execs, __set_db_module_execs)
    def db_get_module_execs(self):
        return self.__db_module_execs
    def db_add_module_exec(self, module_exec):
        self.is_dirty = True
        self.__db_module_execs.append(module_exec)
    def db_change_module_exec(self, module_exec):
        self.is_dirty = True
        found = False
        for i in xrange(len(self.__db_module_execs)):
            if self.__db_module_execs[i].db_id == module_exec.db_id:
                self.__db_module_execs[i] = module_exec
                found = True
                break
        if not found:
            self.__db_module_execs.append(module_exec)
    def db_delete_module_exec(self, module_exec):
        self.is_dirty = True
        for i in xrange(len(self.__db_module_execs)):
            if self.__db_module_execs[i].db_id == module_exec.db_id:
                del self.__db_module_execs[i]
                break
    def db_get_module_exec(self, key):
        for i in xrange(len(self.__db_module_execs)):
            if self.__db_module_execs[i].db_id == key:
                return self.__db_module_execs[i]
        return None
    
    def getPrimaryKey(self):
        return self.__db_id

class DBAdd(object):

    vtType = 'add'

    def __init__(self, id=None, what=None, objectId=None, parentObjId=None, parentObjType=None, data=None):
        self.__db_id = id
        self.__db_what = what
        self.__db_objectId = objectId
        self.__db_parentObjId = parentObjId
        self.__db_parentObjType = parentObjType
        self.__db_data = data
        self.is_dirty = True
        self.is_new = True
    
    def __copy__(self):
        cp = DBAdd()
        cp.db_id = self.db_id
        cp.db_what = self.db_what
        cp.db_objectId = self.db_objectId
        cp.db_parentObjId = self.db_parentObjId
        cp.db_parentObjType = self.db_parentObjType
        cp.db_data = self.db_data
        cp.is_dirty = self.is_dirty
        cp.is_new = self.is_new
        return cp

    def copy_with_new_ids(self, parent, id_scope, id_remap):
        cp = copy.copy(self)
        children = cp.db_children(parent, False)
        for (obj, _, _) in children:
            new_id = id_scope.getNewId(obj.vtType)
            id_remap[(obj.vtType, obj.db_id)] = new_id
            obj.db_id = new_id
        for (obj, _, _) in children:
            if id_remap.has_key(('vistrail', obj.db_vistrailId)):
                obj.db_vistrailId = id_remap[('vistrail', obj.db_vistrailId)]
        
    def db_children(self, parent=(None,None), orphan=False):
        children = []
        if self.db_data is not None:
            children.extend(self.db_data.db_children((self.vtType, self.db_id), orphan))
            if orphan:
                self.db_data = None
        children.append((self, parent[0], parent[1]))
        return children
    def has_changes(self):
        if self.is_dirty:
            return True
        if self.db_data is not None and self.db_data.has_changes():
            return True
        return False
    def __get_db_id(self):
        return self.__db_id
    def __set_db_id(self, id):
        self.__db_id = id
        self.is_dirty = True
    db_id = property(__get_db_id, __set_db_id)
    def db_add_id(self, id):
        self.__db_id = id
    def db_change_id(self, id):
        self.__db_id = id
    def db_delete_id(self, id):
        self.__db_id = None
    
    def __get_db_what(self):
        return self.__db_what
    def __set_db_what(self, what):
        self.__db_what = what
        self.is_dirty = True
    db_what = property(__get_db_what, __set_db_what)
    def db_add_what(self, what):
        self.__db_what = what
    def db_change_what(self, what):
        self.__db_what = what
    def db_delete_what(self, what):
        self.__db_what = None
    
    def __get_db_objectId(self):
        return self.__db_objectId
    def __set_db_objectId(self, objectId):
        self.__db_objectId = objectId
        self.is_dirty = True
    db_objectId = property(__get_db_objectId, __set_db_objectId)
    def db_add_objectId(self, objectId):
        self.__db_objectId = objectId
    def db_change_objectId(self, objectId):
        self.__db_objectId = objectId
    def db_delete_objectId(self, objectId):
        self.__db_objectId = None
    
    def __get_db_parentObjId(self):
        return self.__db_parentObjId
    def __set_db_parentObjId(self, parentObjId):
        self.__db_parentObjId = parentObjId
        self.is_dirty = True
    db_parentObjId = property(__get_db_parentObjId, __set_db_parentObjId)
    def db_add_parentObjId(self, parentObjId):
        self.__db_parentObjId = parentObjId
    def db_change_parentObjId(self, parentObjId):
        self.__db_parentObjId = parentObjId
    def db_delete_parentObjId(self, parentObjId):
        self.__db_parentObjId = None
    
    def __get_db_parentObjType(self):
        return self.__db_parentObjType
    def __set_db_parentObjType(self, parentObjType):
        self.__db_parentObjType = parentObjType
        self.is_dirty = True
    db_parentObjType = property(__get_db_parentObjType, __set_db_parentObjType)
    def db_add_parentObjType(self, parentObjType):
        self.__db_parentObjType = parentObjType
    def db_change_parentObjType(self, parentObjType):
        self.__db_parentObjType = parentObjType
    def db_delete_parentObjType(self, parentObjType):
        self.__db_parentObjType = None
    
    def __get_db_data(self):
        return self.__db_data
    def __set_db_data(self, data):
        self.__db_data = data
        self.is_dirty = True
    db_data = property(__get_db_data, __set_db_data)
    def db_add_data(self, data):
        self.__db_data = data
    def db_change_data(self, data):
        self.__db_data = data
    def db_delete_data(self, data):
        self.__db_data = None
    
    def getPrimaryKey(self):
        return self.__db_id

class DBOther(object):

    vtType = 'other'

    def __init__(self, id=None, key=None, value=None):
        self.__db_id = id
        self.__db_key = key
        self.__db_value = value
        self.is_dirty = True
        self.is_new = True
    
    def __copy__(self):
        cp = DBOther()
        cp.db_id = self.db_id
        cp.db_key = self.db_key
        cp.db_value = self.db_value
        cp.is_dirty = self.is_dirty
        cp.is_new = self.is_new
        return cp

    def copy_with_new_ids(self, parent, id_scope, id_remap):
        cp = copy.copy(self)
        children = cp.db_children(parent, False)
        for (obj, _, _) in children:
            new_id = id_scope.getNewId(obj.vtType)
            id_remap[(obj.vtType, obj.db_id)] = new_id
            obj.db_id = new_id
        for (obj, _, _) in children:
            if id_remap.has_key(('vistrail', obj.db_vistrailId)):
                obj.db_vistrailId = id_remap[('vistrail', obj.db_vistrailId)]
        
    def db_children(self, parent=(None,None), orphan=False):
        children = []
        children.append((self, parent[0], parent[1]))
        return children
    def has_changes(self):
        if self.is_dirty:
            return True
        return False
    def __get_db_id(self):
        return self.__db_id
    def __set_db_id(self, id):
        self.__db_id = id
        self.is_dirty = True
    db_id = property(__get_db_id, __set_db_id)
    def db_add_id(self, id):
        self.__db_id = id
    def db_change_id(self, id):
        self.__db_id = id
    def db_delete_id(self, id):
        self.__db_id = None
    
    def __get_db_key(self):
        return self.__db_key
    def __set_db_key(self, key):
        self.__db_key = key
        self.is_dirty = True
    db_key = property(__get_db_key, __set_db_key)
    def db_add_key(self, key):
        self.__db_key = key
    def db_change_key(self, key):
        self.__db_key = key
    def db_delete_key(self, key):
        self.__db_key = None
    
    def __get_db_value(self):
        return self.__db_value
    def __set_db_value(self, value):
        self.__db_value = value
        self.is_dirty = True
    db_value = property(__get_db_value, __set_db_value)
    def db_add_value(self, value):
        self.__db_value = value
    def db_change_value(self, value):
        self.__db_value = value
    def db_delete_value(self, value):
        self.__db_value = None
    
    def getPrimaryKey(self):
        return self.__db_id

class DBLocation(object):

    vtType = 'location'

    def __init__(self, id=None, x=None, y=None):
        self.__db_id = id
        self.__db_x = x
        self.__db_y = y
        self.is_dirty = True
        self.is_new = True
    
    def __copy__(self):
        cp = DBLocation()
        cp.db_id = self.db_id
        cp.db_x = self.db_x
        cp.db_y = self.db_y
        cp.is_dirty = self.is_dirty
        cp.is_new = self.is_new
        return cp

    def copy_with_new_ids(self, parent, id_scope, id_remap):
        cp = copy.copy(self)
        children = cp.db_children(parent, False)
        for (obj, _, _) in children:
            new_id = id_scope.getNewId(obj.vtType)
            id_remap[(obj.vtType, obj.db_id)] = new_id
            obj.db_id = new_id
        for (obj, _, _) in children:
            if id_remap.has_key(('vistrail', obj.db_vistrailId)):
                obj.db_vistrailId = id_remap[('vistrail', obj.db_vistrailId)]
        
    def db_children(self, parent=(None,None), orphan=False):
        children = []
        children.append((self, parent[0], parent[1]))
        return children
    def has_changes(self):
        if self.is_dirty:
            return True
        return False
    def __get_db_id(self):
        return self.__db_id
    def __set_db_id(self, id):
        self.__db_id = id
        self.is_dirty = True
    db_id = property(__get_db_id, __set_db_id)
    def db_add_id(self, id):
        self.__db_id = id
    def db_change_id(self, id):
        self.__db_id = id
    def db_delete_id(self, id):
        self.__db_id = None
    
    def __get_db_x(self):
        return self.__db_x
    def __set_db_x(self, x):
        self.__db_x = x
        self.is_dirty = True
    db_x = property(__get_db_x, __set_db_x)
    def db_add_x(self, x):
        self.__db_x = x
    def db_change_x(self, x):
        self.__db_x = x
    def db_delete_x(self, x):
        self.__db_x = None
    
    def __get_db_y(self):
        return self.__db_y
    def __set_db_y(self, y):
        self.__db_y = y
        self.is_dirty = True
    db_y = property(__get_db_y, __set_db_y)
    def db_add_y(self, y):
        self.__db_y = y
    def db_change_y(self, y):
        self.__db_y = y
    def db_delete_y(self, y):
        self.__db_y = None
    
    def getPrimaryKey(self):
        return self.__db_id

class DBWorkflowExec(object):

    vtType = 'workflow_exec'

    def __init__(self, id=None, user=None, ip=None, vt_version=None, ts_start=None, ts_end=None, parent_id=None, parent_type=None, parent_version=None, name=None, module_execs=None):
        self.__db_id = id
        self.__db_user = user
        self.__db_ip = ip
        self.__db_vt_version = vt_version
        self.__db_ts_start = ts_start
        self.__db_ts_end = ts_end
        self.__db_parent_id = parent_id
        self.__db_parent_type = parent_type
        self.__db_parent_version = parent_version
        self.__db_name = name
        if module_execs is None:
            self.__db_module_execs = {}
        else:
            self.__db_module_execs = module_execs
        self.is_dirty = True
        self.is_new = True
    
    def __copy__(self):
        cp = DBWorkflowExec()
        cp.db_id = self.db_id
        cp.db_user = self.db_user
        cp.db_ip = self.db_ip
        cp.db_vt_version = self.db_vt_version
        cp.db_ts_start = self.db_ts_start
        cp.db_ts_end = self.db_ts_end
        cp.db_parent_id = self.db_parent_id
        cp.db_parent_type = self.db_parent_type
        cp.db_parent_version = self.db_parent_version
        cp.db_name = self.db_name
        if self.db_module_execs is None:
            cp.db_module_execs = None
        else:
            cp.db_module_execs = dict([(k,copy.copy(v)) for (k,v) in self.db_module_execs.iteritems()])
        cp.is_dirty = self.is_dirty
        cp.is_new = self.is_new
        return cp

    def copy_with_new_ids(self, parent, id_scope, id_remap):
        cp = copy.copy(self)
        children = cp.db_children(parent, False)
        for (obj, _, _) in children:
            new_id = id_scope.getNewId(obj.vtType)
            id_remap[(obj.vtType, obj.db_id)] = new_id
            obj.db_id = new_id
        for (obj, _, _) in children:
            if id_remap.has_key(('vistrail', obj.db_vistrailId)):
                obj.db_vistrailId = id_remap[('vistrail', obj.db_vistrailId)]
        
    def db_children(self, parent=(None,None), orphan=False):
        children = []
        to_del = []
        for child in self.db_module_execs.itervalues():
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
            if orphan:
                to_del.append(child)
        for child in to_del:
            self.db_delete_module_exec(child)
        children.append((self, parent[0], parent[1]))
        return children
    def has_changes(self):
        if self.is_dirty:
            return True
        for child in self.db_module_execs.itervalues():
            if child.has_changes():
                return True
        return False
    def __get_db_id(self):
        return self.__db_id
    def __set_db_id(self, id):
        self.__db_id = id
        self.is_dirty = True
    db_id = property(__get_db_id, __set_db_id)
    def db_add_id(self, id):
        self.__db_id = id
    def db_change_id(self, id):
        self.__db_id = id
    def db_delete_id(self, id):
        self.__db_id = None
    
    def __get_db_user(self):
        return self.__db_user
    def __set_db_user(self, user):
        self.__db_user = user
        self.is_dirty = True
    db_user = property(__get_db_user, __set_db_user)
    def db_add_user(self, user):
        self.__db_user = user
    def db_change_user(self, user):
        self.__db_user = user
    def db_delete_user(self, user):
        self.__db_user = None
    
    def __get_db_ip(self):
        return self.__db_ip
    def __set_db_ip(self, ip):
        self.__db_ip = ip
        self.is_dirty = True
    db_ip = property(__get_db_ip, __set_db_ip)
    def db_add_ip(self, ip):
        self.__db_ip = ip
    def db_change_ip(self, ip):
        self.__db_ip = ip
    def db_delete_ip(self, ip):
        self.__db_ip = None
    
    def __get_db_vt_version(self):
        return self.__db_vt_version
    def __set_db_vt_version(self, vt_version):
        self.__db_vt_version = vt_version
        self.is_dirty = True
    db_vt_version = property(__get_db_vt_version, __set_db_vt_version)
    def db_add_vt_version(self, vt_version):
        self.__db_vt_version = vt_version
    def db_change_vt_version(self, vt_version):
        self.__db_vt_version = vt_version
    def db_delete_vt_version(self, vt_version):
        self.__db_vt_version = None
    
    def __get_db_ts_start(self):
        return self.__db_ts_start
    def __set_db_ts_start(self, ts_start):
        self.__db_ts_start = ts_start
        self.is_dirty = True
    db_ts_start = property(__get_db_ts_start, __set_db_ts_start)
    def db_add_ts_start(self, ts_start):
        self.__db_ts_start = ts_start
    def db_change_ts_start(self, ts_start):
        self.__db_ts_start = ts_start
    def db_delete_ts_start(self, ts_start):
        self.__db_ts_start = None
    
    def __get_db_ts_end(self):
        return self.__db_ts_end
    def __set_db_ts_end(self, ts_end):
        self.__db_ts_end = ts_end
        self.is_dirty = True
    db_ts_end = property(__get_db_ts_end, __set_db_ts_end)
    def db_add_ts_end(self, ts_end):
        self.__db_ts_end = ts_end
    def db_change_ts_end(self, ts_end):
        self.__db_ts_end = ts_end
    def db_delete_ts_end(self, ts_end):
        self.__db_ts_end = None
    
    def __get_db_parent_id(self):
        return self.__db_parent_id
    def __set_db_parent_id(self, parent_id):
        self.__db_parent_id = parent_id
        self.is_dirty = True
    db_parent_id = property(__get_db_parent_id, __set_db_parent_id)
    def db_add_parent_id(self, parent_id):
        self.__db_parent_id = parent_id
    def db_change_parent_id(self, parent_id):
        self.__db_parent_id = parent_id
    def db_delete_parent_id(self, parent_id):
        self.__db_parent_id = None
    
    def __get_db_parent_type(self):
        return self.__db_parent_type
    def __set_db_parent_type(self, parent_type):
        self.__db_parent_type = parent_type
        self.is_dirty = True
    db_parent_type = property(__get_db_parent_type, __set_db_parent_type)
    def db_add_parent_type(self, parent_type):
        self.__db_parent_type = parent_type
    def db_change_parent_type(self, parent_type):
        self.__db_parent_type = parent_type
    def db_delete_parent_type(self, parent_type):
        self.__db_parent_type = None
    
    def __get_db_parent_version(self):
        return self.__db_parent_version
    def __set_db_parent_version(self, parent_version):
        self.__db_parent_version = parent_version
        self.is_dirty = True
    db_parent_version = property(__get_db_parent_version, __set_db_parent_version)
    def db_add_parent_version(self, parent_version):
        self.__db_parent_version = parent_version
    def db_change_parent_version(self, parent_version):
        self.__db_parent_version = parent_version
    def db_delete_parent_version(self, parent_version):
        self.__db_parent_version = None
    
    def __get_db_name(self):
        return self.__db_name
    def __set_db_name(self, name):
        self.__db_name = name
        self.is_dirty = True
    db_name = property(__get_db_name, __set_db_name)
    def db_add_name(self, name):
        self.__db_name = name
    def db_change_name(self, name):
        self.__db_name = name
    def db_delete_name(self, name):
        self.__db_name = None
    
    def __get_db_module_execs(self):
        return self.__db_module_execs
    def __set_db_module_execs(self, module_execs):
        self.__db_module_execs = module_execs
        self.is_dirty = True
    db_module_execs = property(__get_db_module_execs, __set_db_module_execs)
    def db_get_module_execs(self):
        return self.__db_module_execs.values()
    def db_add_module_exec(self, module_exec):
        self.is_dirty = True
        self.__db_module_execs[module_exec.db_id] = module_exec
    def db_change_module_exec(self, module_exec):
        self.is_dirty = True
        self.__db_module_execs[module_exec.db_id] = module_exec
    def db_delete_module_exec(self, module_exec):
        self.is_dirty = True
        del self.__db_module_execs[module_exec.db_id]
    def db_get_module_exec(self, key):
        if self.__db_module_execs.has_key(key):
            return self.__db_module_execs[key]
        return None
    
    def getPrimaryKey(self):
        return self.__db_id

class DBFunction(object):

    vtType = 'function'

    def __init__(self, id=None, pos=None, name=None, parameters=None):
        self.__db_id = id
        self.__db_pos = pos
        self.__db_name = name
        if parameters is None:
            self.__db_parameters = []
        else:
            self.__db_parameters = parameters
        self.is_dirty = True
        self.is_new = True
    
    def __copy__(self):
        cp = DBFunction()
        cp.db_id = self.db_id
        cp.db_pos = self.db_pos
        cp.db_name = self.db_name
        if self.db_parameters is None:
            cp.db_parameters = None
        else:
            cp.db_parameters = [copy.copy(v) for v in self.db_parameters]
        cp.is_dirty = self.is_dirty
        cp.is_new = self.is_new
        return cp

    def copy_with_new_ids(self, parent, id_scope, id_remap):
        cp = copy.copy(self)
        children = cp.db_children(parent, False)
        for (obj, _, _) in children:
            new_id = id_scope.getNewId(obj.vtType)
            id_remap[(obj.vtType, obj.db_id)] = new_id
            obj.db_id = new_id
        for (obj, _, _) in children:
            if id_remap.has_key(('vistrail', obj.db_vistrailId)):
                obj.db_vistrailId = id_remap[('vistrail', obj.db_vistrailId)]
        
    def db_children(self, parent=(None,None), orphan=False):
        children = []
        to_del = []
        for child in self.db_parameters:
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
            if orphan:
                to_del.append(child)
        for child in to_del:
            self.db_delete_parameter(child)
        children.append((self, parent[0], parent[1]))
        return children
    def has_changes(self):
        if self.is_dirty:
            return True
        for child in self.db_parameters:
            if child.has_changes():
                return True
        return False
    def __get_db_id(self):
        return self.__db_id
    def __set_db_id(self, id):
        self.__db_id = id
        self.is_dirty = True
    db_id = property(__get_db_id, __set_db_id)
    def db_add_id(self, id):
        self.__db_id = id
    def db_change_id(self, id):
        self.__db_id = id
    def db_delete_id(self, id):
        self.__db_id = None
    
    def __get_db_pos(self):
        return self.__db_pos
    def __set_db_pos(self, pos):
        self.__db_pos = pos
        self.is_dirty = True
    db_pos = property(__get_db_pos, __set_db_pos)
    def db_add_pos(self, pos):
        self.__db_pos = pos
    def db_change_pos(self, pos):
        self.__db_pos = pos
    def db_delete_pos(self, pos):
        self.__db_pos = None
    
    def __get_db_name(self):
        return self.__db_name
    def __set_db_name(self, name):
        self.__db_name = name
        self.is_dirty = True
    db_name = property(__get_db_name, __set_db_name)
    def db_add_name(self, name):
        self.__db_name = name
    def db_change_name(self, name):
        self.__db_name = name
    def db_delete_name(self, name):
        self.__db_name = None
    
    def __get_db_parameters(self):
        return self.__db_parameters
    def __set_db_parameters(self, parameters):
        self.__db_parameters = parameters
        self.is_dirty = True
    db_parameters = property(__get_db_parameters, __set_db_parameters)
    def db_get_parameters(self):
        return self.__db_parameters
    def db_add_parameter(self, parameter):
        self.is_dirty = True
        self.__db_parameters.append(parameter)
    def db_change_parameter(self, parameter):
        self.is_dirty = True
        found = False
        for i in xrange(len(self.__db_parameters)):
            if self.__db_parameters[i].db_id == parameter.db_id:
                self.__db_parameters[i] = parameter
                found = True
                break
        if not found:
            self.__db_parameters.append(parameter)
    def db_delete_parameter(self, parameter):
        self.is_dirty = True
        for i in xrange(len(self.__db_parameters)):
            if self.__db_parameters[i].db_id == parameter.db_id:
                del self.__db_parameters[i]
                break
    def db_get_parameter(self, key):
        for i in xrange(len(self.__db_parameters)):
            if self.__db_parameters[i].db_id == key:
                return self.__db_parameters[i]
        return None
    
    def getPrimaryKey(self):
        return self.__db_id

class DBAbstraction(object):

    vtType = 'abstraction'

    def __init__(self, id=None, name=None, actions=None, tags=None):
        self.__db_id = id
        self.__db_name = name
        if actions is None:
            self.__db_actions = {}
        else:
            self.__db_actions = actions
        self.db_tags_name_index = {}
        if tags is None:
            self.__db_tags = {}
        else:
            self.__db_tags = tags
            for v in self.__db_tags.itervalues():
                self.db_tags_name_index[v.db_name] = v
        self.is_dirty = True
        self.is_new = True
    
    def __copy__(self):
        cp = DBAbstraction()
        cp.db_id = self.db_id
        cp.db_name = self.db_name
        if self.db_actions is None:
            cp.db_actions = None
        else:
            cp.db_actions = dict([(k,copy.copy(v)) for (k,v) in self.db_actions.iteritems()])
        if self.db_tags is None:
            cp.db_tags = None
        else:
            cp.db_tags = dict([(k,copy.copy(v)) for (k,v) in self.db_tags.iteritems()])
            for v in cp.__db_tags.itervalues():
                cp.db_tags_name_index[v.db_name] = v
        cp.is_dirty = self.is_dirty
        cp.is_new = self.is_new
        return cp

    def copy_with_new_ids(self, parent, id_scope, id_remap):
        cp = copy.copy(self)
        children = cp.db_children(parent, False)
        for (obj, _, _) in children:
            new_id = id_scope.getNewId(obj.vtType)
            id_remap[(obj.vtType, obj.db_id)] = new_id
            obj.db_id = new_id
        
    def db_children(self, parent=(None,None), orphan=False):
        children = []
        to_del = []
        for child in self.db_actions.itervalues():
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
            if orphan:
                to_del.append(child)
        for child in to_del:
            self.db_delete_action(child)
        to_del = []
        for child in self.db_tags.itervalues():
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
            if orphan:
                to_del.append(child)
        for child in to_del:
            self.db_delete_tag(child)
        children.append((self, parent[0], parent[1]))
        return children
    def has_changes(self):
        if self.is_dirty:
            return True
        for child in self.db_actions.itervalues():
            if child.has_changes():
                return True
        for child in self.db_tags.itervalues():
            if child.has_changes():
                return True
        return False
    def __get_db_id(self):
        return self.__db_id
    def __set_db_id(self, id):
        self.__db_id = id
        self.is_dirty = True
    db_id = property(__get_db_id, __set_db_id)
    def db_add_id(self, id):
        self.__db_id = id
    def db_change_id(self, id):
        self.__db_id = id
    def db_delete_id(self, id):
        self.__db_id = None
    
    def __get_db_name(self):
        return self.__db_name
    def __set_db_name(self, name):
        self.__db_name = name
        self.is_dirty = True
    db_name = property(__get_db_name, __set_db_name)
    def db_add_name(self, name):
        self.__db_name = name
    def db_change_name(self, name):
        self.__db_name = name
    def db_delete_name(self, name):
        self.__db_name = None
    
    def __get_db_actions(self):
        return self.__db_actions
    def __set_db_actions(self, actions):
        self.__db_actions = actions
        self.is_dirty = True
    db_actions = property(__get_db_actions, __set_db_actions)
    def db_get_actions(self):
        return self.__db_actions.values()
    def db_add_action(self, action):
        self.is_dirty = True
        self.__db_actions[action.db_id] = action
    def db_change_action(self, action):
        self.is_dirty = True
        self.__db_actions[action.db_id] = action
    def db_delete_action(self, action):
        self.is_dirty = True
        del self.__db_actions[action.db_id]
    def db_get_action(self, key):
        if self.__db_actions.has_key(key):
            return self.__db_actions[key]
        return None
    
    def __get_db_tags(self):
        return self.__db_tags
    def __set_db_tags(self, tags):
        self.__db_tags = tags
        self.is_dirty = True
    db_tags = property(__get_db_tags, __set_db_tags)
    def db_get_tags(self):
        return self.__db_tags.values()
    def db_add_tag(self, tag):
        self.is_dirty = True
        self.__db_tags[tag.db_id] = tag
        self.db_tags_name_index[tag.db_name] = tag
    def db_change_tag(self, tag):
        self.is_dirty = True
        self.__db_tags[tag.db_id] = tag
        self.db_tags_name_index[tag.db_name] = tag
    def db_delete_tag(self, tag):
        self.is_dirty = True
        del self.__db_tags[tag.db_id]
        del self.db_tags_name_index[tag.db_name]
    def db_get_tag(self, key):
        if self.__db_tags.has_key(key):
            return self.__db_tags[key]
        return None
    def db_get_tag_by_name(self, key):
        return self.db_tags_name_index[key]
    def db_has_tag_with_name(self, key):
        return self.db_tags_name_index.has_key(key)
    
    def getPrimaryKey(self):
        return self.__db_id

class DBWorkflow(object):

    vtType = 'workflow'

    def __init__(self, id=None, name=None, modules=None, connections=None, annotations=None, others=None, abstractionRefs=None):
        self.__db_id = id
        self.__db_name = name
        if modules is None:
            self.__db_modules = {}
        else:
            self.__db_modules = modules
        if connections is None:
            self.__db_connections = {}
        else:
            self.__db_connections = connections
        if annotations is None:
            self.__db_annotations = []
        else:
            self.__db_annotations = annotations
        if others is None:
            self.__db_others = []
        else:
            self.__db_others = others
        if abstractionRefs is None:
            self.__db_abstractionRefs = {}
        else:
            self.__db_abstractionRefs = abstractionRefs
        self.is_dirty = True
        self.is_new = True
    
    def __copy__(self):
        cp = DBWorkflow()
        cp.db_id = self.db_id
        cp.db_name = self.db_name
        if self.db_modules is None:
            cp.db_modules = None
        else:
            cp.db_modules = dict([(k,copy.copy(v)) for (k,v) in self.db_modules.iteritems()])
        if self.db_connections is None:
            cp.db_connections = None
        else:
            cp.db_connections = dict([(k,copy.copy(v)) for (k,v) in self.db_connections.iteritems()])
        if self.db_annotations is None:
            cp.db_annotations = None
        else:
            cp.db_annotations = [copy.copy(v) for v in self.db_annotations]
        if self.db_others is None:
            cp.db_others = None
        else:
            cp.db_others = [copy.copy(v) for v in self.db_others]
        if self.db_abstractionRefs is None:
            cp.db_abstractionRefs = None
        else:
            cp.db_abstractionRefs = dict([(k,copy.copy(v)) for (k,v) in self.db_abstractionRefs.iteritems()])
        cp.is_dirty = self.is_dirty
        cp.is_new = self.is_new
        return cp

    def copy_with_new_ids(self, parent, id_scope, id_remap):
        cp = copy.copy(self)
        children = cp.db_children(parent, False)
        for (obj, _, _) in children:
            new_id = id_scope.getNewId(obj.vtType)
            id_remap[(obj.vtType, obj.db_id)] = new_id
            obj.db_id = new_id
        for (obj, _, _) in children:
            if id_remap.has_key(('vistrail', obj.db_vistrailId)):
                obj.db_vistrailId = id_remap[('vistrail', obj.db_vistrailId)]
        
    def db_children(self, parent=(None,None), orphan=False):
        children = []
        to_del = []
        for child in self.db_modules.itervalues():
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
            if orphan:
                to_del.append(child)
        for child in to_del:
            self.db_delete_module(child)
        to_del = []
        for child in self.db_connections.itervalues():
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
            if orphan:
                to_del.append(child)
        for child in to_del:
            self.db_delete_connection(child)
        to_del = []
        for child in self.db_annotations:
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
            if orphan:
                to_del.append(child)
        for child in to_del:
            self.db_delete_annotation(child)
        to_del = []
        for child in self.db_others:
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
            if orphan:
                to_del.append(child)
        for child in to_del:
            self.db_delete_other(child)
        to_del = []
        for child in self.db_abstractionRefs.itervalues():
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
            if orphan:
                to_del.append(child)
        for child in to_del:
            self.db_delete_abstractionRef(child)
        children.append((self, parent[0], parent[1]))
        return children
    def has_changes(self):
        if self.is_dirty:
            return True
        for child in self.db_modules.itervalues():
            if child.has_changes():
                return True
        for child in self.db_connections.itervalues():
            if child.has_changes():
                return True
        for child in self.db_annotations:
            if child.has_changes():
                return True
        for child in self.db_others:
            if child.has_changes():
                return True
        for child in self.db_abstractionRefs.itervalues():
            if child.has_changes():
                return True
        return False
    def __get_db_id(self):
        return self.__db_id
    def __set_db_id(self, id):
        self.__db_id = id
        self.is_dirty = True
    db_id = property(__get_db_id, __set_db_id)
    def db_add_id(self, id):
        self.__db_id = id
    def db_change_id(self, id):
        self.__db_id = id
    def db_delete_id(self, id):
        self.__db_id = None
    
    def __get_db_name(self):
        return self.__db_name
    def __set_db_name(self, name):
        self.__db_name = name
        self.is_dirty = True
    db_name = property(__get_db_name, __set_db_name)
    def db_add_name(self, name):
        self.__db_name = name
    def db_change_name(self, name):
        self.__db_name = name
    def db_delete_name(self, name):
        self.__db_name = None
    
    def __get_db_modules(self):
        return self.__db_modules
    def __set_db_modules(self, modules):
        self.__db_modules = modules
        self.is_dirty = True
    db_modules = property(__get_db_modules, __set_db_modules)
    def db_get_modules(self):
        return self.__db_modules.values()
    def db_add_module(self, module):
        self.is_dirty = True
        self.__db_modules[module.db_id] = module
    def db_change_module(self, module):
        self.is_dirty = True
        self.__db_modules[module.db_id] = module
    def db_delete_module(self, module):
        self.is_dirty = True
        del self.__db_modules[module.db_id]
    def db_get_module(self, key):
        if self.__db_modules.has_key(key):
            return self.__db_modules[key]
        return None
    
    def __get_db_connections(self):
        return self.__db_connections
    def __set_db_connections(self, connections):
        self.__db_connections = connections
        self.is_dirty = True
    db_connections = property(__get_db_connections, __set_db_connections)
    def db_get_connections(self):
        return self.__db_connections.values()
    def db_add_connection(self, connection):
        self.is_dirty = True
        self.__db_connections[connection.db_id] = connection
    def db_change_connection(self, connection):
        self.is_dirty = True
        self.__db_connections[connection.db_id] = connection
    def db_delete_connection(self, connection):
        self.is_dirty = True
        del self.__db_connections[connection.db_id]
    def db_get_connection(self, key):
        if self.__db_connections.has_key(key):
            return self.__db_connections[key]
        return None
    
    def __get_db_annotations(self):
        return self.__db_annotations
    def __set_db_annotations(self, annotations):
        self.__db_annotations = annotations
        self.is_dirty = True
    db_annotations = property(__get_db_annotations, __set_db_annotations)
    def db_get_annotations(self):
        return self.__db_annotations
    def db_add_annotation(self, annotation):
        self.is_dirty = True
        self.__db_annotations.append(annotation)
    def db_change_annotation(self, annotation):
        self.is_dirty = True
        found = False
        for i in xrange(len(self.__db_annotations)):
            if self.__db_annotations[i].db_id == annotation.db_id:
                self.__db_annotations[i] = annotation
                found = True
                break
        if not found:
            self.__db_annotations.append(annotation)
    def db_delete_annotation(self, annotation):
        self.is_dirty = True
        for i in xrange(len(self.__db_annotations)):
            if self.__db_annotations[i].db_id == annotation.db_id:
                del self.__db_annotations[i]
                break
    def db_get_annotation(self, key):
        for i in xrange(len(self.__db_annotations)):
            if self.__db_annotations[i].db_id == key:
                return self.__db_annotations[i]
        return None
    
    def __get_db_others(self):
        return self.__db_others
    def __set_db_others(self, others):
        self.__db_others = others
        self.is_dirty = True
    db_others = property(__get_db_others, __set_db_others)
    def db_get_others(self):
        return self.__db_others
    def db_add_other(self, other):
        self.is_dirty = True
        self.__db_others.append(other)
    def db_change_other(self, other):
        self.is_dirty = True
        found = False
        for i in xrange(len(self.__db_others)):
            if self.__db_others[i].db_id == other.db_id:
                self.__db_others[i] = other
                found = True
                break
        if not found:
            self.__db_others.append(other)
    def db_delete_other(self, other):
        self.is_dirty = True
        for i in xrange(len(self.__db_others)):
            if self.__db_others[i].db_id == other.db_id:
                del self.__db_others[i]
                break
    def db_get_other(self, key):
        for i in xrange(len(self.__db_others)):
            if self.__db_others[i].db_id == key:
                return self.__db_others[i]
        return None
    
    def __get_db_abstractionRefs(self):
        return self.__db_abstractionRefs
    def __set_db_abstractionRefs(self, abstractionRefs):
        self.__db_abstractionRefs = abstractionRefs
        self.is_dirty = True
    db_abstractionRefs = property(__get_db_abstractionRefs, __set_db_abstractionRefs)
    def db_get_abstractionRefs(self):
        return self.__db_abstractionRefs.values()
    def db_add_abstractionRef(self, abstractionRef):
        self.is_dirty = True
        self.__db_abstractionRefs[abstractionRef.db_id] = abstractionRef
    def db_change_abstractionRef(self, abstractionRef):
        self.is_dirty = True
        self.__db_abstractionRefs[abstractionRef.db_id] = abstractionRef
    def db_delete_abstractionRef(self, abstractionRef):
        self.is_dirty = True
        del self.__db_abstractionRefs[abstractionRef.db_id]
    def db_get_abstractionRef(self, key):
        if self.__db_abstractionRefs.has_key(key):
            return self.__db_abstractionRefs[key]
        return None
    
    def getPrimaryKey(self):
        return self.__db_id

class DBAbstractionRef(object):

    vtType = 'abstractionRef'

    def __init__(self, id=None, abstraction_id=None, version=None):
        self.__db_id = id
        self.__db_abstraction_id = abstraction_id
        self.__db_version = version
        self.is_dirty = True
        self.is_new = True
    
    def __copy__(self):
        cp = DBAbstractionRef()
        cp.db_id = self.db_id
        cp.db_abstraction_id = self.db_abstraction_id
        cp.db_version = self.db_version
        cp.is_dirty = self.is_dirty
        cp.is_new = self.is_new
        return cp

    def copy_with_new_ids(self, parent, id_scope, id_remap):
        cp = copy.copy(self)
        children = cp.db_children(parent, False)
        for (obj, _, _) in children:
            new_id = id_scope.getNewId(obj.vtType)
            id_remap[(obj.vtType, obj.db_id)] = new_id
            obj.db_id = new_id
        for (obj, _, _) in children:
            if id_remap.has_key(('vistrail', obj.db_vistrailId)):
                obj.db_vistrailId = id_remap[('vistrail', obj.db_vistrailId)]
        
    def db_children(self, parent=(None,None), orphan=False):
        children = []
        children.append((self, parent[0], parent[1]))
        return children
    def has_changes(self):
        if self.is_dirty:
            return True
        return False
    def __get_db_id(self):
        return self.__db_id
    def __set_db_id(self, id):
        self.__db_id = id
        self.is_dirty = True
    db_id = property(__get_db_id, __set_db_id)
    def db_add_id(self, id):
        self.__db_id = id
    def db_change_id(self, id):
        self.__db_id = id
    def db_delete_id(self, id):
        self.__db_id = None
    
    def __get_db_abstraction_id(self):
        return self.__db_abstraction_id
    def __set_db_abstraction_id(self, abstraction_id):
        self.__db_abstraction_id = abstraction_id
        self.is_dirty = True
    db_abstraction_id = property(__get_db_abstraction_id, __set_db_abstraction_id)
    def db_add_abstraction_id(self, abstraction_id):
        self.__db_abstraction_id = abstraction_id
    def db_change_abstraction_id(self, abstraction_id):
        self.__db_abstraction_id = abstraction_id
    def db_delete_abstraction_id(self, abstraction_id):
        self.__db_abstraction_id = None
    
    def __get_db_version(self):
        return self.__db_version
    def __set_db_version(self, version):
        self.__db_version = version
        self.is_dirty = True
    db_version = property(__get_db_version, __set_db_version)
    def db_add_version(self, version):
        self.__db_version = version
    def db_change_version(self, version):
        self.__db_version = version
    def db_delete_version(self, version):
        self.__db_version = None
    
    def getPrimaryKey(self):
        return self.__db_id

class DBAnnotation(object):

    vtType = 'annotation'

    def __init__(self, id=None, key=None, value=None):
        self.__db_id = id
        self.__db_key = key
        self.__db_value = value
        self.is_dirty = True
        self.is_new = True
    
    def __copy__(self):
        cp = DBAnnotation()
        cp.db_id = self.db_id
        cp.db_key = self.db_key
        cp.db_value = self.db_value
        cp.is_dirty = self.is_dirty
        cp.is_new = self.is_new
        return cp

    def copy_with_new_ids(self, parent, id_scope, id_remap):
        cp = copy.copy(self)
        children = cp.db_children(parent, False)
        for (obj, _, _) in children:
            new_id = id_scope.getNewId(obj.vtType)
            id_remap[(obj.vtType, obj.db_id)] = new_id
            obj.db_id = new_id
        for (obj, _, _) in children:
            if id_remap.has_key(('vistrail', obj.db_vistrailId)):
                obj.db_vistrailId = id_remap[('vistrail', obj.db_vistrailId)]
        
    def db_children(self, parent=(None,None), orphan=False):
        children = []
        children.append((self, parent[0], parent[1]))
        return children
    def has_changes(self):
        if self.is_dirty:
            return True
        return False
    def __get_db_id(self):
        return self.__db_id
    def __set_db_id(self, id):
        self.__db_id = id
        self.is_dirty = True
    db_id = property(__get_db_id, __set_db_id)
    def db_add_id(self, id):
        self.__db_id = id
    def db_change_id(self, id):
        self.__db_id = id
    def db_delete_id(self, id):
        self.__db_id = None
    
    def __get_db_key(self):
        return self.__db_key
    def __set_db_key(self, key):
        self.__db_key = key
        self.is_dirty = True
    db_key = property(__get_db_key, __set_db_key)
    def db_add_key(self, key):
        self.__db_key = key
    def db_change_key(self, key):
        self.__db_key = key
    def db_delete_key(self, key):
        self.__db_key = None
    
    def __get_db_value(self):
        return self.__db_value
    def __set_db_value(self, value):
        self.__db_value = value
        self.is_dirty = True
    db_value = property(__get_db_value, __set_db_value)
    def db_add_value(self, value):
        self.__db_value = value
    def db_change_value(self, value):
        self.__db_value = value
    def db_delete_value(self, value):
        self.__db_value = None
    
    def getPrimaryKey(self):
        return self.__db_id

class DBChange(object):

    vtType = 'change'

    def __init__(self, id=None, what=None, oldObjId=None, newObjId=None, parentObjId=None, parentObjType=None, data=None):
        self.__db_id = id
        self.__db_what = what
        self.__db_oldObjId = oldObjId
        self.__db_newObjId = newObjId
        self.__db_parentObjId = parentObjId
        self.__db_parentObjType = parentObjType
        self.__db_data = data
        self.is_dirty = True
        self.is_new = True
    
    def __copy__(self):
        cp = DBChange()
        cp.db_id = self.db_id
        cp.db_what = self.db_what
        cp.db_oldObjId = self.db_oldObjId
        cp.db_newObjId = self.db_newObjId
        cp.db_parentObjId = self.db_parentObjId
        cp.db_parentObjType = self.db_parentObjType
        cp.db_data = self.db_data
        cp.is_dirty = self.is_dirty
        cp.is_new = self.is_new
        return cp

    def copy_with_new_ids(self, parent, id_scope, id_remap):
        cp = copy.copy(self)
        children = cp.db_children(parent, False)
        for (obj, _, _) in children:
            new_id = id_scope.getNewId(obj.vtType)
            id_remap[(obj.vtType, obj.db_id)] = new_id
            obj.db_id = new_id
        for (obj, _, _) in children:
            if id_remap.has_key(('vistrail', obj.db_vistrailId)):
                obj.db_vistrailId = id_remap[('vistrail', obj.db_vistrailId)]
        
    def db_children(self, parent=(None,None), orphan=False):
        children = []
        if self.db_data is not None:
            children.extend(self.db_data.db_children((self.vtType, self.db_id), orphan))
            if orphan:
                self.db_data = None
        children.append((self, parent[0], parent[1]))
        return children
    def has_changes(self):
        if self.is_dirty:
            return True
        if self.db_data is not None and self.db_data.has_changes():
            return True
        return False
    def __get_db_id(self):
        return self.__db_id
    def __set_db_id(self, id):
        self.__db_id = id
        self.is_dirty = True
    db_id = property(__get_db_id, __set_db_id)
    def db_add_id(self, id):
        self.__db_id = id
    def db_change_id(self, id):
        self.__db_id = id
    def db_delete_id(self, id):
        self.__db_id = None
    
    def __get_db_what(self):
        return self.__db_what
    def __set_db_what(self, what):
        self.__db_what = what
        self.is_dirty = True
    db_what = property(__get_db_what, __set_db_what)
    def db_add_what(self, what):
        self.__db_what = what
    def db_change_what(self, what):
        self.__db_what = what
    def db_delete_what(self, what):
        self.__db_what = None
    
    def __get_db_oldObjId(self):
        return self.__db_oldObjId
    def __set_db_oldObjId(self, oldObjId):
        self.__db_oldObjId = oldObjId
        self.is_dirty = True
    db_oldObjId = property(__get_db_oldObjId, __set_db_oldObjId)
    def db_add_oldObjId(self, oldObjId):
        self.__db_oldObjId = oldObjId
    def db_change_oldObjId(self, oldObjId):
        self.__db_oldObjId = oldObjId
    def db_delete_oldObjId(self, oldObjId):
        self.__db_oldObjId = None
    
    def __get_db_newObjId(self):
        return self.__db_newObjId
    def __set_db_newObjId(self, newObjId):
        self.__db_newObjId = newObjId
        self.is_dirty = True
    db_newObjId = property(__get_db_newObjId, __set_db_newObjId)
    def db_add_newObjId(self, newObjId):
        self.__db_newObjId = newObjId
    def db_change_newObjId(self, newObjId):
        self.__db_newObjId = newObjId
    def db_delete_newObjId(self, newObjId):
        self.__db_newObjId = None
    
    def __get_db_parentObjId(self):
        return self.__db_parentObjId
    def __set_db_parentObjId(self, parentObjId):
        self.__db_parentObjId = parentObjId
        self.is_dirty = True
    db_parentObjId = property(__get_db_parentObjId, __set_db_parentObjId)
    def db_add_parentObjId(self, parentObjId):
        self.__db_parentObjId = parentObjId
    def db_change_parentObjId(self, parentObjId):
        self.__db_parentObjId = parentObjId
    def db_delete_parentObjId(self, parentObjId):
        self.__db_parentObjId = None
    
    def __get_db_parentObjType(self):
        return self.__db_parentObjType
    def __set_db_parentObjType(self, parentObjType):
        self.__db_parentObjType = parentObjType
        self.is_dirty = True
    db_parentObjType = property(__get_db_parentObjType, __set_db_parentObjType)
    def db_add_parentObjType(self, parentObjType):
        self.__db_parentObjType = parentObjType
    def db_change_parentObjType(self, parentObjType):
        self.__db_parentObjType = parentObjType
    def db_delete_parentObjType(self, parentObjType):
        self.__db_parentObjType = None
    
    def __get_db_data(self):
        return self.__db_data
    def __set_db_data(self, data):
        self.__db_data = data
        self.is_dirty = True
    db_data = property(__get_db_data, __set_db_data)
    def db_add_data(self, data):
        self.__db_data = data
    def db_change_data(self, data):
        self.__db_data = data
    def db_delete_data(self, data):
        self.__db_data = None
    
    def getPrimaryKey(self):
        return self.__db_id

class DBParameter(object):

    vtType = 'parameter'

    def __init__(self, id=None, pos=None, name=None, type=None, val=None, alias=None):
        self.__db_id = id
        self.__db_pos = pos
        self.__db_name = name
        self.__db_type = type
        self.__db_val = val
        self.__db_alias = alias
        self.is_dirty = True
        self.is_new = True
    
    def __copy__(self):
        cp = DBParameter()
        cp.db_id = self.db_id
        cp.db_pos = self.db_pos
        cp.db_name = self.db_name
        cp.db_type = self.db_type
        cp.db_val = self.db_val
        cp.db_alias = self.db_alias
        cp.is_dirty = self.is_dirty
        cp.is_new = self.is_new
        return cp

    def copy_with_new_ids(self, parent, id_scope, id_remap):
        cp = copy.copy(self)
        children = cp.db_children(parent, False)
        for (obj, _, _) in children:
            new_id = id_scope.getNewId(obj.vtType)
            id_remap[(obj.vtType, obj.db_id)] = new_id
            obj.db_id = new_id
        for (obj, _, _) in children:
            if id_remap.has_key(('vistrail', obj.db_vistrailId)):
                obj.db_vistrailId = id_remap[('vistrail', obj.db_vistrailId)]
        
    def db_children(self, parent=(None,None), orphan=False):
        children = []
        children.append((self, parent[0], parent[1]))
        return children
    def has_changes(self):
        if self.is_dirty:
            return True
        return False
    def __get_db_id(self):
        return self.__db_id
    def __set_db_id(self, id):
        self.__db_id = id
        self.is_dirty = True
    db_id = property(__get_db_id, __set_db_id)
    def db_add_id(self, id):
        self.__db_id = id
    def db_change_id(self, id):
        self.__db_id = id
    def db_delete_id(self, id):
        self.__db_id = None
    
    def __get_db_pos(self):
        return self.__db_pos
    def __set_db_pos(self, pos):
        self.__db_pos = pos
        self.is_dirty = True
    db_pos = property(__get_db_pos, __set_db_pos)
    def db_add_pos(self, pos):
        self.__db_pos = pos
    def db_change_pos(self, pos):
        self.__db_pos = pos
    def db_delete_pos(self, pos):
        self.__db_pos = None
    
    def __get_db_name(self):
        return self.__db_name
    def __set_db_name(self, name):
        self.__db_name = name
        self.is_dirty = True
    db_name = property(__get_db_name, __set_db_name)
    def db_add_name(self, name):
        self.__db_name = name
    def db_change_name(self, name):
        self.__db_name = name
    def db_delete_name(self, name):
        self.__db_name = None
    
    def __get_db_type(self):
        return self.__db_type
    def __set_db_type(self, type):
        self.__db_type = type
        self.is_dirty = True
    db_type = property(__get_db_type, __set_db_type)
    def db_add_type(self, type):
        self.__db_type = type
    def db_change_type(self, type):
        self.__db_type = type
    def db_delete_type(self, type):
        self.__db_type = None
    
    def __get_db_val(self):
        return self.__db_val
    def __set_db_val(self, val):
        self.__db_val = val
        self.is_dirty = True
    db_val = property(__get_db_val, __set_db_val)
    def db_add_val(self, val):
        self.__db_val = val
    def db_change_val(self, val):
        self.__db_val = val
    def db_delete_val(self, val):
        self.__db_val = None
    
    def __get_db_alias(self):
        return self.__db_alias
    def __set_db_alias(self, alias):
        self.__db_alias = alias
        self.is_dirty = True
    db_alias = property(__get_db_alias, __set_db_alias)
    def db_add_alias(self, alias):
        self.__db_alias = alias
    def db_change_alias(self, alias):
        self.__db_alias = alias
    def db_delete_alias(self, alias):
        self.__db_alias = None
    
    def getPrimaryKey(self):
        return self.__db_id

class DBConnection(object):

    vtType = 'connection'

    def __init__(self, id=None, ports=None):
        self.__db_id = id
        self.db_ports_type_index = {}
        if ports is None:
            self.__db_ports = []
        else:
            self.__db_ports = ports
            for v in self.__db_ports:
                self.db_ports_type_index[v.db_type] = v
        self.is_dirty = True
        self.is_new = True
    
    def __copy__(self):
        cp = DBConnection()
        cp.db_id = self.db_id
        if self.db_ports is None:
            cp.db_ports = None
        else:
            cp.db_ports = [copy.copy(v) for v in self.db_ports]
            for v in cp.__db_ports:
                cp.db_ports_type_index[v.db_type] = v
        cp.is_dirty = self.is_dirty
        cp.is_new = self.is_new
        return cp

    def copy_with_new_ids(self, parent, id_scope, id_remap):
        cp = copy.copy(self)
        children = cp.db_children(parent, False)
        for (obj, _, _) in children:
            new_id = id_scope.getNewId(obj.vtType)
            id_remap[(obj.vtType, obj.db_id)] = new_id
            obj.db_id = new_id
        for (obj, _, _) in children:
            if id_remap.has_key(('vistrail', obj.db_vistrailId)):
                obj.db_vistrailId = id_remap[('vistrail', obj.db_vistrailId)]
        
    def db_children(self, parent=(None,None), orphan=False):
        children = []
        to_del = []
        for child in self.db_ports:
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
            if orphan:
                to_del.append(child)
        for child in to_del:
            self.db_delete_port(child)
        children.append((self, parent[0], parent[1]))
        return children
    def has_changes(self):
        if self.is_dirty:
            return True
        for child in self.db_ports:
            if child.has_changes():
                return True
        return False
    def __get_db_id(self):
        return self.__db_id
    def __set_db_id(self, id):
        self.__db_id = id
        self.is_dirty = True
    db_id = property(__get_db_id, __set_db_id)
    def db_add_id(self, id):
        self.__db_id = id
    def db_change_id(self, id):
        self.__db_id = id
    def db_delete_id(self, id):
        self.__db_id = None
    
    def __get_db_ports(self):
        return self.__db_ports
    def __set_db_ports(self, ports):
        self.__db_ports = ports
        self.is_dirty = True
    db_ports = property(__get_db_ports, __set_db_ports)
    def db_get_ports(self):
        return self.__db_ports
    def db_add_port(self, port):
        self.is_dirty = True
        self.__db_ports.append(port)
        self.db_ports_type_index[port.db_type] = port
    def db_change_port(self, port):
        self.is_dirty = True
        found = False
        for i in xrange(len(self.__db_ports)):
            if self.__db_ports[i].db_id == port.db_id:
                self.__db_ports[i] = port
                found = True
                break
        if not found:
            self.__db_ports.append(port)
        self.db_ports_type_index[port.db_type] = port
    def db_delete_port(self, port):
        self.is_dirty = True
        for i in xrange(len(self.__db_ports)):
            if self.__db_ports[i].db_id == port.db_id:
                del self.__db_ports[i]
                break
        del self.db_ports_type_index[port.db_type]
    def db_get_port(self, key):
        for i in xrange(len(self.__db_ports)):
            if self.__db_ports[i].db_id == key:
                return self.__db_ports[i]
        return None
    def db_get_port_by_type(self, key):
        return self.db_ports_type_index[key]
    def db_has_port_with_type(self, key):
        return self.db_ports_type_index.has_key(key)
    
    def getPrimaryKey(self):
        return self.__db_id

class DBAction(object):

    vtType = 'action'

    def __init__(self, id=None, prevId=None, date=None, user=None, annotations=None, operations=None):
        self.__db_id = id
        self.__db_prevId = prevId
        self.__db_date = date
        self.__db_user = user
        self.db_annotations_key_index = {}
        if annotations is None:
            self.__db_annotations = {}
        else:
            self.__db_annotations = annotations
            for v in self.__db_annotations.itervalues():
                self.db_annotations_key_index[v.db_key] = v
        if operations is None:
            self.__db_operations = []
        else:
            self.__db_operations = operations
        self.is_dirty = True
        self.is_new = True
    
    def __copy__(self):
        cp = DBAction()
        cp.db_id = self.db_id
        cp.db_prevId = self.db_prevId
        cp.db_date = self.db_date
        cp.db_user = self.db_user
        if self.db_annotations is None:
            cp.db_annotations = None
        else:
            cp.db_annotations = dict([(k,copy.copy(v)) for (k,v) in self.db_annotations.iteritems()])
            for v in cp.__db_annotations.itervalues():
                cp.db_annotations_key_index[v.db_key] = v
        if self.db_operations is None:
            cp.db_operations = None
        else:
            cp.db_operations = [copy.copy(v) for v in self.db_operations]
        cp.is_dirty = self.is_dirty
        cp.is_new = self.is_new
        return cp

    def copy_with_new_ids(self, parent, id_scope, id_remap):
        cp = copy.copy(self)
        children = cp.db_children(parent, False)
        for (obj, _, _) in children:
            new_id = id_scope.getNewId(obj.vtType)
            id_remap[(obj.vtType, obj.db_id)] = new_id
            obj.db_id = new_id
        for (obj, _, _) in children:
            if id_remap.has_key(('vistrail', obj.db_vistrailId)):
                obj.db_vistrailId = id_remap[('vistrail', obj.db_vistrailId)]
        
    def db_children(self, parent=(None,None), orphan=False):
        children = []
        to_del = []
        for child in self.db_annotations.itervalues():
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
            if orphan:
                to_del.append(child)
        for child in to_del:
            self.db_delete_annotation(child)
        to_del = []
        for child in self.db_operations:
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
            if orphan:
                to_del.append(child)
        for child in to_del:
            self.db_delete_operation(child)
        children.append((self, parent[0], parent[1]))
        return children
    def has_changes(self):
        if self.is_dirty:
            return True
        for child in self.db_annotations.itervalues():
            if child.has_changes():
                return True
        for child in self.db_operations:
            if child.has_changes():
                return True
        return False
    def __get_db_id(self):
        return self.__db_id
    def __set_db_id(self, id):
        self.__db_id = id
        self.is_dirty = True
    db_id = property(__get_db_id, __set_db_id)
    def db_add_id(self, id):
        self.__db_id = id
    def db_change_id(self, id):
        self.__db_id = id
    def db_delete_id(self, id):
        self.__db_id = None
    
    def __get_db_prevId(self):
        return self.__db_prevId
    def __set_db_prevId(self, prevId):
        self.__db_prevId = prevId
        self.is_dirty = True
    db_prevId = property(__get_db_prevId, __set_db_prevId)
    def db_add_prevId(self, prevId):
        self.__db_prevId = prevId
    def db_change_prevId(self, prevId):
        self.__db_prevId = prevId
    def db_delete_prevId(self, prevId):
        self.__db_prevId = None
    
    def __get_db_date(self):
        return self.__db_date
    def __set_db_date(self, date):
        self.__db_date = date
        self.is_dirty = True
    db_date = property(__get_db_date, __set_db_date)
    def db_add_date(self, date):
        self.__db_date = date
    def db_change_date(self, date):
        self.__db_date = date
    def db_delete_date(self, date):
        self.__db_date = None
    
    def __get_db_user(self):
        return self.__db_user
    def __set_db_user(self, user):
        self.__db_user = user
        self.is_dirty = True
    db_user = property(__get_db_user, __set_db_user)
    def db_add_user(self, user):
        self.__db_user = user
    def db_change_user(self, user):
        self.__db_user = user
    def db_delete_user(self, user):
        self.__db_user = None
    
    def __get_db_annotations(self):
        return self.__db_annotations
    def __set_db_annotations(self, annotations):
        self.__db_annotations = annotations
        self.is_dirty = True
    db_annotations = property(__get_db_annotations, __set_db_annotations)
    def db_get_annotations(self):
        return self.__db_annotations.values()
    def db_add_annotation(self, annotation):
        self.is_dirty = True
        self.__db_annotations[annotation.db_id] = annotation
        self.db_annotations_key_index[annotation.db_key] = annotation
    def db_change_annotation(self, annotation):
        self.is_dirty = True
        self.__db_annotations[annotation.db_id] = annotation
        self.db_annotations_key_index[annotation.db_key] = annotation
    def db_delete_annotation(self, annotation):
        self.is_dirty = True
        del self.__db_annotations[annotation.db_id]
        del self.db_annotations_key_index[annotation.db_key]
    def db_get_annotation(self, key):
        if self.__db_annotations.has_key(key):
            return self.__db_annotations[key]
        return None
    def db_get_annotation_by_key(self, key):
        return self.db_annotations_key_index[key]
    def db_has_annotation_with_key(self, key):
        return self.db_annotations_key_index.has_key(key)
    
    def __get_db_operations(self):
        return self.__db_operations
    def __set_db_operations(self, operations):
        self.__db_operations = operations
        self.is_dirty = True
    db_operations = property(__get_db_operations, __set_db_operations)
    def db_get_operations(self):
        return self.__db_operations
    def db_add_operation(self, operation):
        self.is_dirty = True
        self.__db_operations.append(operation)
    def db_change_operation(self, operation):
        self.is_dirty = True
        found = False
        for i in xrange(len(self.__db_operations)):
            if self.__db_operations[i].db_id == operation.db_id:
                self.__db_operations[i] = operation
                found = True
                break
        if not found:
            self.__db_operations.append(operation)
    def db_delete_operation(self, operation):
        self.is_dirty = True
        for i in xrange(len(self.__db_operations)):
            if self.__db_operations[i].db_id == operation.db_id:
                del self.__db_operations[i]
                break
    def db_get_operation(self, key):
        for i in xrange(len(self.__db_operations)):
            if self.__db_operations[i].db_id == key:
                return self.__db_operations[i]
        return None
    
    def getPrimaryKey(self):
        return self.__db_id

class DBDelete(object):

    vtType = 'delete'

    def __init__(self, id=None, what=None, objectId=None, parentObjId=None, parentObjType=None):
        self.__db_id = id
        self.__db_what = what
        self.__db_objectId = objectId
        self.__db_parentObjId = parentObjId
        self.__db_parentObjType = parentObjType
        self.is_dirty = True
        self.is_new = True
    
    def __copy__(self):
        cp = DBDelete()
        cp.db_id = self.db_id
        cp.db_what = self.db_what
        cp.db_objectId = self.db_objectId
        cp.db_parentObjId = self.db_parentObjId
        cp.db_parentObjType = self.db_parentObjType
        cp.is_dirty = self.is_dirty
        cp.is_new = self.is_new
        return cp

    def copy_with_new_ids(self, parent, id_scope, id_remap):
        cp = copy.copy(self)
        children = cp.db_children(parent, False)
        for (obj, _, _) in children:
            new_id = id_scope.getNewId(obj.vtType)
            id_remap[(obj.vtType, obj.db_id)] = new_id
            obj.db_id = new_id
        for (obj, _, _) in children:
            if id_remap.has_key(('vistrail', obj.db_vistrailId)):
                obj.db_vistrailId = id_remap[('vistrail', obj.db_vistrailId)]
        
    def db_children(self, parent=(None,None), orphan=False):
        children = []
        children.append((self, parent[0], parent[1]))
        return children
    def has_changes(self):
        if self.is_dirty:
            return True
        return False
    def __get_db_id(self):
        return self.__db_id
    def __set_db_id(self, id):
        self.__db_id = id
        self.is_dirty = True
    db_id = property(__get_db_id, __set_db_id)
    def db_add_id(self, id):
        self.__db_id = id
    def db_change_id(self, id):
        self.__db_id = id
    def db_delete_id(self, id):
        self.__db_id = None
    
    def __get_db_what(self):
        return self.__db_what
    def __set_db_what(self, what):
        self.__db_what = what
        self.is_dirty = True
    db_what = property(__get_db_what, __set_db_what)
    def db_add_what(self, what):
        self.__db_what = what
    def db_change_what(self, what):
        self.__db_what = what
    def db_delete_what(self, what):
        self.__db_what = None
    
    def __get_db_objectId(self):
        return self.__db_objectId
    def __set_db_objectId(self, objectId):
        self.__db_objectId = objectId
        self.is_dirty = True
    db_objectId = property(__get_db_objectId, __set_db_objectId)
    def db_add_objectId(self, objectId):
        self.__db_objectId = objectId
    def db_change_objectId(self, objectId):
        self.__db_objectId = objectId
    def db_delete_objectId(self, objectId):
        self.__db_objectId = None
    
    def __get_db_parentObjId(self):
        return self.__db_parentObjId
    def __set_db_parentObjId(self, parentObjId):
        self.__db_parentObjId = parentObjId
        self.is_dirty = True
    db_parentObjId = property(__get_db_parentObjId, __set_db_parentObjId)
    def db_add_parentObjId(self, parentObjId):
        self.__db_parentObjId = parentObjId
    def db_change_parentObjId(self, parentObjId):
        self.__db_parentObjId = parentObjId
    def db_delete_parentObjId(self, parentObjId):
        self.__db_parentObjId = None
    
    def __get_db_parentObjType(self):
        return self.__db_parentObjType
    def __set_db_parentObjType(self, parentObjType):
        self.__db_parentObjType = parentObjType
        self.is_dirty = True
    db_parentObjType = property(__get_db_parentObjType, __set_db_parentObjType)
    def db_add_parentObjType(self, parentObjType):
        self.__db_parentObjType = parentObjType
    def db_change_parentObjType(self, parentObjType):
        self.__db_parentObjType = parentObjType
    def db_delete_parentObjType(self, parentObjType):
        self.__db_parentObjType = None
    
    def getPrimaryKey(self):
        return self.__db_id

class DBVistrail(object):

    vtType = 'vistrail'

    def __init__(self, id=None, version=None, name=None, dbHost=None, dbPort=None, dbName=None, actions=None, tags=None, abstractions=None):
        self.__db_id = id
        self.__db_version = version
        self.__db_name = name
        self.__db_dbHost = dbHost
        self.__db_dbPort = dbPort
        self.__db_dbName = dbName
        if actions is None:
            self.__db_actions = {}
        else:
            self.__db_actions = actions
        self.db_tags_name_index = {}
        if tags is None:
            self.__db_tags = {}
        else:
            self.__db_tags = tags
            for v in self.__db_tags.itervalues():
                self.db_tags_name_index[v.db_name] = v
        if abstractions is None:
            self.__db_abstractions = {}
        else:
            self.__db_abstractions = abstractions
        self.is_dirty = True
        self.is_new = True
    
    def __copy__(self):
        cp = DBVistrail()
        cp.db_id = self.db_id
        cp.db_version = self.db_version
        cp.db_name = self.db_name
        cp.db_dbHost = self.db_dbHost
        cp.db_dbPort = self.db_dbPort
        cp.db_dbName = self.db_dbName
        if self.db_actions is None:
            cp.db_actions = None
        else:
            cp.db_actions = dict([(k,copy.copy(v)) for (k,v) in self.db_actions.iteritems()])
        if self.db_tags is None:
            cp.db_tags = None
        else:
            cp.db_tags = dict([(k,copy.copy(v)) for (k,v) in self.db_tags.iteritems()])
            for v in cp.__db_tags.itervalues():
                cp.db_tags_name_index[v.db_name] = v
        if self.db_abstractions is None:
            cp.db_abstractions = None
        else:
            cp.db_abstractions = dict([(k,copy.copy(v)) for (k,v) in self.db_abstractions.iteritems()])
        cp.is_dirty = self.is_dirty
        cp.is_new = self.is_new
        return cp

    def copy_with_new_ids(self, parent, id_scope, id_remap):
        cp = copy.copy(self)
        children = cp.db_children(parent, False)
        for (obj, _, _) in children:
            new_id = id_scope.getNewId(obj.vtType)
            id_remap[(obj.vtType, obj.db_id)] = new_id
            obj.db_id = new_id
        
    def db_children(self, parent=(None,None), orphan=False):
        children = []
        to_del = []
        for child in self.db_actions.itervalues():
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
            if orphan:
                to_del.append(child)
        for child in to_del:
            self.db_delete_action(child)
        to_del = []
        for child in self.db_tags.itervalues():
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
            if orphan:
                to_del.append(child)
        for child in to_del:
            self.db_delete_tag(child)
        to_del = []
        for child in self.db_abstractions.itervalues():
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
            if orphan:
                to_del.append(child)
        for child in to_del:
            self.db_delete_abstraction(child)
        children.append((self, parent[0], parent[1]))
        return children
    def has_changes(self):
        if self.is_dirty:
            return True
        for child in self.db_actions.itervalues():
            if child.has_changes():
                return True
        for child in self.db_tags.itervalues():
            if child.has_changes():
                return True
        for child in self.db_abstractions.itervalues():
            if child.has_changes():
                return True
        return False
    def __get_db_id(self):
        return self.__db_id
    def __set_db_id(self, id):
        self.__db_id = id
        self.is_dirty = True
    db_id = property(__get_db_id, __set_db_id)
    def db_add_id(self, id):
        self.__db_id = id
    def db_change_id(self, id):
        self.__db_id = id
    def db_delete_id(self, id):
        self.__db_id = None
    
    def __get_db_version(self):
        return self.__db_version
    def __set_db_version(self, version):
        self.__db_version = version
        self.is_dirty = True
    db_version = property(__get_db_version, __set_db_version)
    def db_add_version(self, version):
        self.__db_version = version
    def db_change_version(self, version):
        self.__db_version = version
    def db_delete_version(self, version):
        self.__db_version = None
    
    def __get_db_name(self):
        return self.__db_name
    def __set_db_name(self, name):
        self.__db_name = name
        self.is_dirty = True
    db_name = property(__get_db_name, __set_db_name)
    def db_add_name(self, name):
        self.__db_name = name
    def db_change_name(self, name):
        self.__db_name = name
    def db_delete_name(self, name):
        self.__db_name = None
    
    def __get_db_dbHost(self):
        return self.__db_dbHost
    def __set_db_dbHost(self, dbHost):
        self.__db_dbHost = dbHost
        self.is_dirty = True
    db_dbHost = property(__get_db_dbHost, __set_db_dbHost)
    def db_add_dbHost(self, dbHost):
        self.__db_dbHost = dbHost
    def db_change_dbHost(self, dbHost):
        self.__db_dbHost = dbHost
    def db_delete_dbHost(self, dbHost):
        self.__db_dbHost = None
    
    def __get_db_dbPort(self):
        return self.__db_dbPort
    def __set_db_dbPort(self, dbPort):
        self.__db_dbPort = dbPort
        self.is_dirty = True
    db_dbPort = property(__get_db_dbPort, __set_db_dbPort)
    def db_add_dbPort(self, dbPort):
        self.__db_dbPort = dbPort
    def db_change_dbPort(self, dbPort):
        self.__db_dbPort = dbPort
    def db_delete_dbPort(self, dbPort):
        self.__db_dbPort = None
    
    def __get_db_dbName(self):
        return self.__db_dbName
    def __set_db_dbName(self, dbName):
        self.__db_dbName = dbName
        self.is_dirty = True
    db_dbName = property(__get_db_dbName, __set_db_dbName)
    def db_add_dbName(self, dbName):
        self.__db_dbName = dbName
    def db_change_dbName(self, dbName):
        self.__db_dbName = dbName
    def db_delete_dbName(self, dbName):
        self.__db_dbName = None
    
    def __get_db_actions(self):
        return self.__db_actions
    def __set_db_actions(self, actions):
        self.__db_actions = actions
        self.is_dirty = True
    db_actions = property(__get_db_actions, __set_db_actions)
    def db_get_actions(self):
        return self.__db_actions.values()
    def db_add_action(self, action):
        self.is_dirty = True
        self.__db_actions[action.db_id] = action
    def db_change_action(self, action):
        self.is_dirty = True
        self.__db_actions[action.db_id] = action
    def db_delete_action(self, action):
        self.is_dirty = True
        del self.__db_actions[action.db_id]
    def db_get_action(self, key):
        if self.__db_actions.has_key(key):
            return self.__db_actions[key]
        return None
    
    def __get_db_tags(self):
        return self.__db_tags
    def __set_db_tags(self, tags):
        self.__db_tags = tags
        self.is_dirty = True
    db_tags = property(__get_db_tags, __set_db_tags)
    def db_get_tags(self):
        return self.__db_tags.values()
    def db_add_tag(self, tag):
        self.is_dirty = True
        self.__db_tags[tag.db_id] = tag
        self.db_tags_name_index[tag.db_name] = tag
    def db_change_tag(self, tag):
        self.is_dirty = True
        self.__db_tags[tag.db_id] = tag
        self.db_tags_name_index[tag.db_name] = tag
    def db_delete_tag(self, tag):
        self.is_dirty = True
        del self.__db_tags[tag.db_id]
        del self.db_tags_name_index[tag.db_name]
    def db_get_tag(self, key):
        if self.__db_tags.has_key(key):
            return self.__db_tags[key]
        return None
    def db_get_tag_by_name(self, key):
        return self.db_tags_name_index[key]
    def db_has_tag_with_name(self, key):
        return self.db_tags_name_index.has_key(key)
    
    def __get_db_abstractions(self):
        return self.__db_abstractions
    def __set_db_abstractions(self, abstractions):
        self.__db_abstractions = abstractions
        self.is_dirty = True
    db_abstractions = property(__get_db_abstractions, __set_db_abstractions)
    def db_get_abstractions(self):
        return self.__db_abstractions.values()
    def db_add_abstraction(self, abstraction):
        self.is_dirty = True
        self.__db_abstractions[abstraction.db_id] = abstraction
    def db_change_abstraction(self, abstraction):
        self.is_dirty = True
        self.__db_abstractions[abstraction.db_id] = abstraction
    def db_delete_abstraction(self, abstraction):
        self.is_dirty = True
        del self.__db_abstractions[abstraction.db_id]
    def db_get_abstraction(self, key):
        if self.__db_abstractions.has_key(key):
            return self.__db_abstractions[key]
        return None
    
    def getPrimaryKey(self):
        return self.__db_id

class DBModuleExec(object):

    vtType = 'module_exec'

    def __init__(self, id=None, ts_start=None, ts_end=None, module_id=None, module_name=None, annotations=None):
        self.__db_id = id
        self.__db_ts_start = ts_start
        self.__db_ts_end = ts_end
        self.__db_module_id = module_id
        self.__db_module_name = module_name
        if annotations is None:
            self.__db_annotations = []
        else:
            self.__db_annotations = annotations
        self.is_dirty = True
        self.is_new = True
    
    def __copy__(self):
        cp = DBModuleExec()
        cp.db_id = self.db_id
        cp.db_ts_start = self.db_ts_start
        cp.db_ts_end = self.db_ts_end
        cp.db_module_id = self.db_module_id
        cp.db_module_name = self.db_module_name
        if self.db_annotations is None:
            cp.db_annotations = None
        else:
            cp.db_annotations = [copy.copy(v) for v in self.db_annotations]
        cp.is_dirty = self.is_dirty
        cp.is_new = self.is_new
        return cp

    def copy_with_new_ids(self, parent, id_scope, id_remap):
        cp = copy.copy(self)
        children = cp.db_children(parent, False)
        for (obj, _, _) in children:
            new_id = id_scope.getNewId(obj.vtType)
            id_remap[(obj.vtType, obj.db_id)] = new_id
            obj.db_id = new_id
        for (obj, _, _) in children:
            if id_remap.has_key(('module', obj.db_module_id)):
                obj.db_module_id = id_remap[('module', obj.db_module_id)]
            if id_remap.has_key(('vistrail', obj.db_vistrailId)):
                obj.db_vistrailId = id_remap[('vistrail', obj.db_vistrailId)]
        
    def db_children(self, parent=(None,None), orphan=False):
        children = []
        to_del = []
        for child in self.db_annotations:
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
            if orphan:
                to_del.append(child)
        for child in to_del:
            self.db_delete_annotation(child)
        children.append((self, parent[0], parent[1]))
        return children
    def has_changes(self):
        if self.is_dirty:
            return True
        for child in self.db_annotations:
            if child.has_changes():
                return True
        return False
    def __get_db_id(self):
        return self.__db_id
    def __set_db_id(self, id):
        self.__db_id = id
        self.is_dirty = True
    db_id = property(__get_db_id, __set_db_id)
    def db_add_id(self, id):
        self.__db_id = id
    def db_change_id(self, id):
        self.__db_id = id
    def db_delete_id(self, id):
        self.__db_id = None
    
    def __get_db_ts_start(self):
        return self.__db_ts_start
    def __set_db_ts_start(self, ts_start):
        self.__db_ts_start = ts_start
        self.is_dirty = True
    db_ts_start = property(__get_db_ts_start, __set_db_ts_start)
    def db_add_ts_start(self, ts_start):
        self.__db_ts_start = ts_start
    def db_change_ts_start(self, ts_start):
        self.__db_ts_start = ts_start
    def db_delete_ts_start(self, ts_start):
        self.__db_ts_start = None
    
    def __get_db_ts_end(self):
        return self.__db_ts_end
    def __set_db_ts_end(self, ts_end):
        self.__db_ts_end = ts_end
        self.is_dirty = True
    db_ts_end = property(__get_db_ts_end, __set_db_ts_end)
    def db_add_ts_end(self, ts_end):
        self.__db_ts_end = ts_end
    def db_change_ts_end(self, ts_end):
        self.__db_ts_end = ts_end
    def db_delete_ts_end(self, ts_end):
        self.__db_ts_end = None
    
    def __get_db_module_id(self):
        return self.__db_module_id
    def __set_db_module_id(self, module_id):
        self.__db_module_id = module_id
        self.is_dirty = True
    db_module_id = property(__get_db_module_id, __set_db_module_id)
    def db_add_module_id(self, module_id):
        self.__db_module_id = module_id
    def db_change_module_id(self, module_id):
        self.__db_module_id = module_id
    def db_delete_module_id(self, module_id):
        self.__db_module_id = None
    
    def __get_db_module_name(self):
        return self.__db_module_name
    def __set_db_module_name(self, module_name):
        self.__db_module_name = module_name
        self.is_dirty = True
    db_module_name = property(__get_db_module_name, __set_db_module_name)
    def db_add_module_name(self, module_name):
        self.__db_module_name = module_name
    def db_change_module_name(self, module_name):
        self.__db_module_name = module_name
    def db_delete_module_name(self, module_name):
        self.__db_module_name = None
    
    def __get_db_annotations(self):
        return self.__db_annotations
    def __set_db_annotations(self, annotations):
        self.__db_annotations = annotations
        self.is_dirty = True
    db_annotations = property(__get_db_annotations, __set_db_annotations)
    def db_get_annotations(self):
        return self.__db_annotations
    def db_add_annotation(self, annotation):
        self.is_dirty = True
        self.__db_annotations.append(annotation)
    def db_change_annotation(self, annotation):
        self.is_dirty = True
        found = False
        for i in xrange(len(self.__db_annotations)):
            if self.__db_annotations[i].db_id == annotation.db_id:
                self.__db_annotations[i] = annotation
                found = True
                break
        if not found:
            self.__db_annotations.append(annotation)
    def db_delete_annotation(self, annotation):
        self.is_dirty = True
        for i in xrange(len(self.__db_annotations)):
            if self.__db_annotations[i].db_id == annotation.db_id:
                del self.__db_annotations[i]
                break
    def db_get_annotation(self, key):
        for i in xrange(len(self.__db_annotations)):
            if self.__db_annotations[i].db_id == key:
                return self.__db_annotations[i]
        return None
    
    def getPrimaryKey(self):
        return self.__db_id

