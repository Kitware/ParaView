
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
    
    def __copy__(self):
        cp = DBPortSpec()
        cp.db_id = self.db_id
        cp.is_dirty = self.is_dirty
        cp.db_name = self.db_name
        cp.is_dirty = self.is_dirty
        cp.db_type = self.db_type
        cp.is_dirty = self.is_dirty
        cp.db_spec = self.db_spec
        cp.is_dirty = self.is_dirty
        return cp

    def db_children(self, parent=(None,None), orphan=False):
        children = []
        children.append((self, parent[0], parent[1]))
        return children
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

    def __init__(self, id=None, cache=None, name=None, location=None, functions=None, annotations=None, portSpecs=None):
        self.__db_id = id
        self.__db_cache = cache
        self.__db_name = name
        self.__db_location = location
        if functions is None:
            self.__db_functions = []
        else:
            self.__db_functions = functions
        if annotations is None:
            self.__db_annotations = {}
        else:
            self.__db_annotations = annotations
        if portSpecs is None:
            self.__db_portSpecs = []
        else:
            self.__db_portSpecs = portSpecs
        self.is_dirty = True
    
    def __copy__(self):
        cp = DBModule()
        cp.db_id = self.db_id
        cp.is_dirty = self.is_dirty
        cp.db_cache = self.db_cache
        cp.is_dirty = self.is_dirty
        cp.db_name = self.db_name
        cp.is_dirty = self.is_dirty
        cp.db_location = self.db_location
        cp.is_dirty = self.is_dirty
        if self.db_functions is None:
            cp.db_functions = None
        else:
            cp.db_functions = [copy.copy(v) for v in self.db_functions]
        cp.is_dirty = self.is_dirty
        if self.db_annotations is None:
            cp.db_annotations = None
        else:
            cp.db_annotations = dict([(k,copy.copy(v)) for (k,v) in self.db_annotations.iteritems()])
        cp.is_dirty = self.is_dirty
        if self.db_portSpecs is None:
            cp.db_portSpecs = None
        else:
            cp.db_portSpecs = [copy.copy(v) for v in self.db_portSpecs]
        cp.is_dirty = self.is_dirty
        return cp

    def db_children(self, parent=(None,None), orphan=False):
        children = []
        children.extend(self.db_location.db_children((self.vtType, self.db_id), orphan))
        if orphan:
            self.db_location = None
        for child in self.db_functions:
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
        if orphan:
            self.db_functions = []
        for child in self.db_annotations.itervalues():
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
        if orphan:
            self.db_annotations = {}
        for child in self.db_portSpecs:
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
        if orphan:
            self.db_portSpecs = []
        children.append((self, parent[0], parent[1]))
        return children
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
    def db_change_annotation(self, annotation):
        self.is_dirty = True
        self.__db_annotations[annotation.db_id] = annotation
    def db_delete_annotation(self, annotation):
        self.is_dirty = True
        del self.__db_annotations[annotation.db_id]
    def db_get_annotation(self, key):
        if self.__db_annotations.has_key(key):
            return self.__db_annotations[key]
        return None
    
    def __get_db_portSpecs(self):
        return self.__db_portSpecs
    def __set_db_portSpecs(self, portSpecs):
        self.__db_portSpecs = portSpecs
        self.is_dirty = True
    db_portSpecs = property(__get_db_portSpecs, __set_db_portSpecs)
    def db_get_portSpecs(self):
        return self.__db_portSpecs
    def db_add_portSpec(self, portSpec):
        self.is_dirty = True
        self.__db_portSpecs.append(portSpec)
    def db_change_portSpec(self, portSpec):
        self.is_dirty = True
        found = False
        for i in xrange(len(self.__db_portSpecs)):
            if self.__db_portSpecs[i].db_id == portSpec.db_id:
                self.__db_portSpecs[i] = portSpec
                found = True
                break
        if not found:
            self.__db_portSpecs.append(portSpec)
    def db_delete_portSpec(self, portSpec):
        self.is_dirty = True
        for i in xrange(len(self.__db_portSpecs)):
            if self.__db_portSpecs[i].db_id == portSpec.db_id:
                del self.__db_portSpecs[i]
                break
    def db_get_portSpec(self, key):
        for i in xrange(len(self.__db_portSpecs)):
            if self.__db_portSpecs[i].db_id == key:
                return self.__db_portSpecs[i]
        return None
    
    def getPrimaryKey(self):
        return self.__db_id

class DBSession(object):

    vtType = 'session'

    def __init__(self, id=None, user=None, ip=None, visVersion=None, tsStart=None, tsEnd=None, machineId=None, wfExecs=None):
        self.__db_id = id
        self.__db_user = user
        self.__db_ip = ip
        self.__db_visVersion = visVersion
        self.__db_tsStart = tsStart
        self.__db_tsEnd = tsEnd
        self.__db_machineId = machineId
        if wfExecs is None:
            self.__db_wfExecs = {}
        else:
            self.__db_wfExecs = wfExecs
        self.is_dirty = True
    
    def __copy__(self):
        cp = DBSession()
        cp.db_id = self.db_id
        cp.is_dirty = self.is_dirty
        cp.db_user = self.db_user
        cp.is_dirty = self.is_dirty
        cp.db_ip = self.db_ip
        cp.is_dirty = self.is_dirty
        cp.db_visVersion = self.db_visVersion
        cp.is_dirty = self.is_dirty
        cp.db_tsStart = self.db_tsStart
        cp.is_dirty = self.is_dirty
        cp.db_tsEnd = self.db_tsEnd
        cp.is_dirty = self.is_dirty
        cp.db_machineId = self.db_machineId
        cp.is_dirty = self.is_dirty
        if self.db_wfExecs is None:
            cp.db_wfExecs = None
        else:
            cp.db_wfExecs = dict([(k,copy.copy(v)) for (k,v) in self.db_wfExecs.iteritems()])
        cp.is_dirty = self.is_dirty
        return cp

    def db_children(self, parent=(None,None), orphan=False):
        children = []
        for child in self.db_wfExecs.itervalues():
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
        if orphan:
            self.db_wfExecs = {}
        children.extend(self.db_log.db_children((self.vtType, self.db_id), orphan))
        if orphan:
            self.db_log = None
        children.append((self, parent[0], parent[1]))
        return children
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
    
    def __get_db_visVersion(self):
        return self.__db_visVersion
    def __set_db_visVersion(self, visVersion):
        self.__db_visVersion = visVersion
        self.is_dirty = True
    db_visVersion = property(__get_db_visVersion, __set_db_visVersion)
    def db_add_visVersion(self, visVersion):
        self.__db_visVersion = visVersion
    def db_change_visVersion(self, visVersion):
        self.__db_visVersion = visVersion
    def db_delete_visVersion(self, visVersion):
        self.__db_visVersion = None
    
    def __get_db_tsStart(self):
        return self.__db_tsStart
    def __set_db_tsStart(self, tsStart):
        self.__db_tsStart = tsStart
        self.is_dirty = True
    db_tsStart = property(__get_db_tsStart, __set_db_tsStart)
    def db_add_tsStart(self, tsStart):
        self.__db_tsStart = tsStart
    def db_change_tsStart(self, tsStart):
        self.__db_tsStart = tsStart
    def db_delete_tsStart(self, tsStart):
        self.__db_tsStart = None
    
    def __get_db_tsEnd(self):
        return self.__db_tsEnd
    def __set_db_tsEnd(self, tsEnd):
        self.__db_tsEnd = tsEnd
        self.is_dirty = True
    db_tsEnd = property(__get_db_tsEnd, __set_db_tsEnd)
    def db_add_tsEnd(self, tsEnd):
        self.__db_tsEnd = tsEnd
    def db_change_tsEnd(self, tsEnd):
        self.__db_tsEnd = tsEnd
    def db_delete_tsEnd(self, tsEnd):
        self.__db_tsEnd = None
    
    def __get_db_machineId(self):
        return self.__db_machineId
    def __set_db_machineId(self, machineId):
        self.__db_machineId = machineId
        self.is_dirty = True
    db_machineId = property(__get_db_machineId, __set_db_machineId)
    def db_add_machineId(self, machineId):
        self.__db_machineId = machineId
    def db_change_machineId(self, machineId):
        self.__db_machineId = machineId
    def db_delete_machineId(self, machineId):
        self.__db_machineId = None
    
    def __get_db_wfExecs(self):
        return self.__db_wfExecs
    def __set_db_wfExecs(self, wfExecs):
        self.__db_wfExecs = wfExecs
        self.is_dirty = True
    db_wfExecs = property(__get_db_wfExecs, __set_db_wfExecs)
    def db_get_wfExecs(self):
        return self.__db_wfExecs.values()
    def db_add_wfExec(self, wfExec):
        self.is_dirty = True
        self.__db_wfExecs[wfExec.db_id] = wfExec
    def db_change_wfExec(self, wfExec):
        self.is_dirty = True
        self.__db_wfExecs[wfExec.db_id] = wfExec
    def db_delete_wfExec(self, wfExec):
        self.is_dirty = True
        del self.__db_wfExecs[wfExec.db_id]
    def db_get_wfExec(self, key):
        if self.__db_wfExecs.has_key(key):
            return self.__db_wfExecs[key]
        return None
    
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
    
    def __copy__(self):
        cp = DBPort()
        cp.db_id = self.db_id
        cp.is_dirty = self.is_dirty
        cp.db_type = self.db_type
        cp.is_dirty = self.is_dirty
        cp.db_moduleId = self.db_moduleId
        cp.is_dirty = self.is_dirty
        cp.db_moduleName = self.db_moduleName
        cp.is_dirty = self.is_dirty
        cp.db_sig = self.db_sig
        cp.is_dirty = self.is_dirty
        return cp

    def db_children(self, parent=(None,None), orphan=False):
        children = []
        children.append((self, parent[0], parent[1]))
        return children
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

    def __init__(self, id=None, sessions=None, machines=None):
        self.__db_id = id
        if sessions is None:
            self.__db_sessions = {}
        else:
            self.__db_sessions = sessions
        if machines is None:
            self.__db_machines = {}
        else:
            self.__db_machines = machines
        self.is_dirty = True
    
    def __copy__(self):
        cp = DBLog()
        cp.db_id = self.db_id
        cp.is_dirty = self.is_dirty
        if self.db_sessions is None:
            cp.db_sessions = None
        else:
            cp.db_sessions = dict([(k,copy.copy(v)) for (k,v) in self.db_sessions.iteritems()])
        cp.is_dirty = self.is_dirty
        if self.db_machines is None:
            cp.db_machines = None
        else:
            cp.db_machines = dict([(k,copy.copy(v)) for (k,v) in self.db_machines.iteritems()])
        cp.is_dirty = self.is_dirty
        return cp

    def db_children(self, parent=(None,None), orphan=False):
        children = []
        for child in self.db_sessions.itervalues():
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
        if orphan:
            self.db_sessions = {}
        for child in self.db_machines.itervalues():
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
        if orphan:
            self.db_machines = {}
        children.append((self, parent[0], parent[1]))
        return children
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
    
    def __get_db_sessions(self):
        return self.__db_sessions
    def __set_db_sessions(self, sessions):
        self.__db_sessions = sessions
        self.is_dirty = True
    db_sessions = property(__get_db_sessions, __set_db_sessions)
    def db_get_sessions(self):
        return self.__db_sessions.values()
    def db_add_session(self, session):
        self.is_dirty = True
        self.__db_sessions[session.db_id] = session
    def db_change_session(self, session):
        self.is_dirty = True
        self.__db_sessions[session.db_id] = session
    def db_delete_session(self, session):
        self.is_dirty = True
        del self.__db_sessions[session.db_id]
    def db_get_session(self, key):
        if self.__db_sessions.has_key(key):
            return self.__db_sessions[key]
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

    def __init__(self, id=None, name=None, os=None, architecture=None, processor=None, ram=None):
        self.__db_id = id
        self.__db_name = name
        self.__db_os = os
        self.__db_architecture = architecture
        self.__db_processor = processor
        self.__db_ram = ram
        self.is_dirty = True
    
    def __copy__(self):
        cp = DBMachine()
        cp.db_id = self.db_id
        cp.is_dirty = self.is_dirty
        cp.db_name = self.db_name
        cp.is_dirty = self.is_dirty
        cp.db_os = self.db_os
        cp.is_dirty = self.is_dirty
        cp.db_architecture = self.db_architecture
        cp.is_dirty = self.is_dirty
        cp.db_processor = self.db_processor
        cp.is_dirty = self.is_dirty
        cp.db_ram = self.db_ram
        cp.is_dirty = self.is_dirty
        return cp

    def db_children(self, parent=(None,None), orphan=False):
        children = []
        children.extend(self.db_log.db_children((self.vtType, self.db_id), orphan))
        if orphan:
            self.db_log = None
        children.append((self, parent[0], parent[1]))
        return children
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
    
    def __copy__(self):
        cp = DBAdd()
        cp.db_id = self.db_id
        cp.is_dirty = self.is_dirty
        cp.db_what = self.db_what
        cp.is_dirty = self.is_dirty
        cp.db_objectId = self.db_objectId
        cp.is_dirty = self.is_dirty
        cp.db_parentObjId = self.db_parentObjId
        cp.is_dirty = self.is_dirty
        cp.db_parentObjType = self.db_parentObjType
        cp.is_dirty = self.is_dirty
        cp.db_data = self.db_data
        cp.is_dirty = self.is_dirty
        return cp

    def db_children(self, parent=(None,None), orphan=False):
        children = []
        children.extend(self.db_action.db_children((self.vtType, self.db_id), orphan))
        if orphan:
            self.db_action = None
        children.append((self, parent[0], parent[1]))
        return children
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
    
    def __copy__(self):
        cp = DBOther()
        cp.db_id = self.db_id
        cp.is_dirty = self.is_dirty
        cp.db_key = self.db_key
        cp.is_dirty = self.is_dirty
        cp.db_value = self.db_value
        cp.is_dirty = self.is_dirty
        return cp

    def db_children(self, parent=(None,None), orphan=False):
        children = []
        children.append((self, parent[0], parent[1]))
        return children
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
    
    def __copy__(self):
        cp = DBLocation()
        cp.db_id = self.db_id
        cp.is_dirty = self.is_dirty
        cp.db_x = self.db_x
        cp.is_dirty = self.is_dirty
        cp.db_y = self.db_y
        cp.is_dirty = self.is_dirty
        return cp

    def db_children(self, parent=(None,None), orphan=False):
        children = []
        children.append((self, parent[0], parent[1]))
        return children
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

class DBWfExec(object):

    vtType = 'wfExec'

    def __init__(self, id=None, tsStart=None, tsEnd=None, wfVersion=None, vistrailId=None, vistrailName=None, execRecs=None):
        self.__db_id = id
        self.__db_tsStart = tsStart
        self.__db_tsEnd = tsEnd
        self.__db_wfVersion = wfVersion
        self.__db_vistrailId = vistrailId
        self.__db_vistrailName = vistrailName
        if execRecs is None:
            self.__db_execRecs = {}
        else:
            self.__db_execRecs = execRecs
        self.is_dirty = True
    
    def __copy__(self):
        cp = DBWfExec()
        cp.db_id = self.db_id
        cp.is_dirty = self.is_dirty
        cp.db_tsStart = self.db_tsStart
        cp.is_dirty = self.is_dirty
        cp.db_tsEnd = self.db_tsEnd
        cp.is_dirty = self.is_dirty
        cp.db_wfVersion = self.db_wfVersion
        cp.is_dirty = self.is_dirty
        cp.db_vistrailId = self.db_vistrailId
        cp.is_dirty = self.is_dirty
        cp.db_vistrailName = self.db_vistrailName
        cp.is_dirty = self.is_dirty
        if self.db_execRecs is None:
            cp.db_execRecs = None
        else:
            cp.db_execRecs = dict([(k,copy.copy(v)) for (k,v) in self.db_execRecs.iteritems()])
        cp.is_dirty = self.is_dirty
        return cp

    def db_children(self, parent=(None,None), orphan=False):
        children = []
        children.extend(self.db_session.db_children((self.vtType, self.db_id), orphan))
        if orphan:
            self.db_session = None
        for child in self.db_execRecs.itervalues():
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
        if orphan:
            self.db_execRecs = {}
        children.append((self, parent[0], parent[1]))
        return children
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
    
    def __get_db_tsStart(self):
        return self.__db_tsStart
    def __set_db_tsStart(self, tsStart):
        self.__db_tsStart = tsStart
        self.is_dirty = True
    db_tsStart = property(__get_db_tsStart, __set_db_tsStart)
    def db_add_tsStart(self, tsStart):
        self.__db_tsStart = tsStart
    def db_change_tsStart(self, tsStart):
        self.__db_tsStart = tsStart
    def db_delete_tsStart(self, tsStart):
        self.__db_tsStart = None
    
    def __get_db_tsEnd(self):
        return self.__db_tsEnd
    def __set_db_tsEnd(self, tsEnd):
        self.__db_tsEnd = tsEnd
        self.is_dirty = True
    db_tsEnd = property(__get_db_tsEnd, __set_db_tsEnd)
    def db_add_tsEnd(self, tsEnd):
        self.__db_tsEnd = tsEnd
    def db_change_tsEnd(self, tsEnd):
        self.__db_tsEnd = tsEnd
    def db_delete_tsEnd(self, tsEnd):
        self.__db_tsEnd = None
    
    def __get_db_wfVersion(self):
        return self.__db_wfVersion
    def __set_db_wfVersion(self, wfVersion):
        self.__db_wfVersion = wfVersion
        self.is_dirty = True
    db_wfVersion = property(__get_db_wfVersion, __set_db_wfVersion)
    def db_add_wfVersion(self, wfVersion):
        self.__db_wfVersion = wfVersion
    def db_change_wfVersion(self, wfVersion):
        self.__db_wfVersion = wfVersion
    def db_delete_wfVersion(self, wfVersion):
        self.__db_wfVersion = None
    
    def __get_db_vistrailId(self):
        return self.__db_vistrailId
    def __set_db_vistrailId(self, vistrailId):
        self.__db_vistrailId = vistrailId
        self.is_dirty = True
    db_vistrailId = property(__get_db_vistrailId, __set_db_vistrailId)
    def db_add_vistrailId(self, vistrailId):
        self.__db_vistrailId = vistrailId
    def db_change_vistrailId(self, vistrailId):
        self.__db_vistrailId = vistrailId
    def db_delete_vistrailId(self, vistrailId):
        self.__db_vistrailId = None
    
    def __get_db_vistrailName(self):
        return self.__db_vistrailName
    def __set_db_vistrailName(self, vistrailName):
        self.__db_vistrailName = vistrailName
        self.is_dirty = True
    db_vistrailName = property(__get_db_vistrailName, __set_db_vistrailName)
    def db_add_vistrailName(self, vistrailName):
        self.__db_vistrailName = vistrailName
    def db_change_vistrailName(self, vistrailName):
        self.__db_vistrailName = vistrailName
    def db_delete_vistrailName(self, vistrailName):
        self.__db_vistrailName = None
    
    def __get_db_execRecs(self):
        return self.__db_execRecs
    def __set_db_execRecs(self, execRecs):
        self.__db_execRecs = execRecs
        self.is_dirty = True
    db_execRecs = property(__get_db_execRecs, __set_db_execRecs)
    def db_get_execRecs(self):
        return self.__db_execRecs.values()
    def db_add_execRec(self, execRec):
        self.is_dirty = True
        self.__db_execRecs[execRec.db_id] = execRec
    def db_change_execRec(self, execRec):
        self.is_dirty = True
        self.__db_execRecs[execRec.db_id] = execRec
    def db_delete_execRec(self, execRec):
        self.is_dirty = True
        del self.__db_execRecs[execRec.db_id]
    def db_get_execRec(self, key):
        if self.__db_execRecs.has_key(key):
            return self.__db_execRecs[key]
        return None
    
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
    
    def __copy__(self):
        cp = DBParameter()
        cp.db_id = self.db_id
        cp.is_dirty = self.is_dirty
        cp.db_pos = self.db_pos
        cp.is_dirty = self.is_dirty
        cp.db_name = self.db_name
        cp.is_dirty = self.is_dirty
        cp.db_type = self.db_type
        cp.is_dirty = self.is_dirty
        cp.db_val = self.db_val
        cp.is_dirty = self.is_dirty
        cp.db_alias = self.db_alias
        cp.is_dirty = self.is_dirty
        return cp

    def db_children(self, parent=(None,None), orphan=False):
        children = []
        children.append((self, parent[0], parent[1]))
        return children
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
    
    def __copy__(self):
        cp = DBFunction()
        cp.db_id = self.db_id
        cp.is_dirty = self.is_dirty
        cp.db_pos = self.db_pos
        cp.is_dirty = self.is_dirty
        cp.db_name = self.db_name
        cp.is_dirty = self.is_dirty
        if self.db_parameters is None:
            cp.db_parameters = None
        else:
            cp.db_parameters = [copy.copy(v) for v in self.db_parameters]
        cp.is_dirty = self.is_dirty
        return cp

    def db_children(self, parent=(None,None), orphan=False):
        children = []
        for child in self.db_parameters:
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
        if orphan:
            self.db_parameters = []
        children.append((self, parent[0], parent[1]))
        return children
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

class DBWorkflow(object):

    vtType = 'workflow'

    def __init__(self, id=None, name=None, modules=None, connections=None, annotations=None, others=None):
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
        self.is_dirty = True
    
    def __copy__(self):
        cp = DBWorkflow()
        cp.db_id = self.db_id
        cp.is_dirty = self.is_dirty
        cp.db_name = self.db_name
        cp.is_dirty = self.is_dirty
        if self.db_modules is None:
            cp.db_modules = None
        else:
            cp.db_modules = dict([(k,copy.copy(v)) for (k,v) in self.db_modules.iteritems()])
        cp.is_dirty = self.is_dirty
        if self.db_connections is None:
            cp.db_connections = None
        else:
            cp.db_connections = dict([(k,copy.copy(v)) for (k,v) in self.db_connections.iteritems()])
        cp.is_dirty = self.is_dirty
        if self.db_annotations is None:
            cp.db_annotations = None
        else:
            cp.db_annotations = [copy.copy(v) for v in self.db_annotations]
        cp.is_dirty = self.is_dirty
        if self.db_others is None:
            cp.db_others = None
        else:
            cp.db_others = [copy.copy(v) for v in self.db_others]
        cp.is_dirty = self.is_dirty
        return cp

    def db_children(self, parent=(None,None), orphan=False):
        children = []
        for child in self.db_modules.itervalues():
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
        if orphan:
            self.db_modules = {}
        for child in self.db_connections.itervalues():
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
        if orphan:
            self.db_connections = {}
        for child in self.db_annotations:
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
        if orphan:
            self.db_annotations = []
        for child in self.db_others:
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
        if orphan:
            self.db_others = []
        children.append((self, parent[0], parent[1]))
        return children
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
    
    def getPrimaryKey(self):
        return self.__db_id

class DBAction(object):

    vtType = 'action'

    def __init__(self, id=None, prevId=None, date=None, user=None, operations=None):
        self.__db_id = id
        self.__db_prevId = prevId
        self.__db_date = date
        self.__db_user = user
        if operations is None:
            self.__db_operations = []
        else:
            self.__db_operations = operations
        self.is_dirty = True
    
    def __copy__(self):
        cp = DBAction()
        cp.db_id = self.db_id
        cp.is_dirty = self.is_dirty
        cp.db_prevId = self.db_prevId
        cp.is_dirty = self.is_dirty
        cp.db_date = self.db_date
        cp.is_dirty = self.is_dirty
        cp.db_user = self.db_user
        cp.is_dirty = self.is_dirty
        if self.db_operations is None:
            cp.db_operations = None
        else:
            cp.db_operations = [copy.copy(v) for v in self.db_operations]
        cp.is_dirty = self.is_dirty
        return cp

    def db_children(self, parent=(None,None), orphan=False):
        children = []
        children.append((self, parent[0], parent[1]))
        return children
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

class DBAnnotation(object):

    vtType = 'annotation'

    def __init__(self, id=None, key=None, value=None):
        self.__db_id = id
        self.__db_key = key
        self.__db_value = value
        self.is_dirty = True
    
    def __copy__(self):
        cp = DBAnnotation()
        cp.db_id = self.db_id
        cp.is_dirty = self.is_dirty
        cp.db_key = self.db_key
        cp.is_dirty = self.is_dirty
        cp.db_value = self.db_value
        cp.is_dirty = self.is_dirty
        return cp

    def db_children(self, parent=(None,None), orphan=False):
        children = []
        children.append((self, parent[0], parent[1]))
        return children
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
    
    def __copy__(self):
        cp = DBChange()
        cp.db_id = self.db_id
        cp.is_dirty = self.is_dirty
        cp.db_what = self.db_what
        cp.is_dirty = self.is_dirty
        cp.db_oldObjId = self.db_oldObjId
        cp.is_dirty = self.is_dirty
        cp.db_newObjId = self.db_newObjId
        cp.is_dirty = self.is_dirty
        cp.db_parentObjId = self.db_parentObjId
        cp.is_dirty = self.is_dirty
        cp.db_parentObjType = self.db_parentObjType
        cp.is_dirty = self.is_dirty
        cp.db_data = self.db_data
        cp.is_dirty = self.is_dirty
        return cp

    def db_children(self, parent=(None,None), orphan=False):
        children = []
        children.extend(self.db_action.db_children((self.vtType, self.db_id), orphan))
        if orphan:
            self.db_action = None
        children.append((self, parent[0], parent[1]))
        return children
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

class DBMacro(object):

    vtType = 'macro'

    def __init__(self, id=None, name=None, descrptn=None, actions=None):
        self.__db_id = id
        self.__db_name = name
        self.__db_descrptn = descrptn
        if actions is None:
            self.__db_actions = {}
        else:
            self.__db_actions = actions
        self.is_dirty = True
    
    def __copy__(self):
        cp = DBMacro()
        cp.db_id = self.db_id
        cp.is_dirty = self.is_dirty
        cp.db_name = self.db_name
        cp.is_dirty = self.is_dirty
        cp.db_descrptn = self.db_descrptn
        cp.is_dirty = self.is_dirty
        if self.db_actions is None:
            cp.db_actions = None
        else:
            cp.db_actions = dict([(k,copy.copy(v)) for (k,v) in self.db_actions.iteritems()])
        cp.is_dirty = self.is_dirty
        return cp

    def db_children(self, parent=(None,None), orphan=False):
        children = []
        for child in self.db_actions.itervalues():
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
        if orphan:
            self.db_actions = {}
        children.extend(self.db_vistrail.db_children((self.vtType, self.db_id), orphan))
        if orphan:
            self.db_vistrail = None
        children.append((self, parent[0], parent[1]))
        return children
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
    
    def __get_db_descrptn(self):
        return self.__db_descrptn
    def __set_db_descrptn(self, descrptn):
        self.__db_descrptn = descrptn
        self.is_dirty = True
    db_descrptn = property(__get_db_descrptn, __set_db_descrptn)
    def db_add_descrptn(self, descrptn):
        self.__db_descrptn = descrptn
    def db_change_descrptn(self, descrptn):
        self.__db_descrptn = descrptn
    def db_delete_descrptn(self, descrptn):
        self.__db_descrptn = None
    
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
    
    def getPrimaryKey(self):
        return self.__db_id

class DBConnection(object):

    vtType = 'connection'

    def __init__(self, id=None, ports=None):
        self.__db_id = id
        self.db_type_index = {}
        if ports is None:
            self.__db_ports = []
        else:
            self.__db_ports = ports
            for v in self.__db_ports:
                self.db_type_index[v.db_type] = v
        self.is_dirty = True
    
    def __copy__(self):
        cp = DBConnection()
        cp.db_id = self.db_id
        cp.is_dirty = self.is_dirty
        if self.db_ports is None:
            cp.db_ports = None
        else:
            cp.db_ports = [copy.copy(v) for v in self.db_ports]
            for v in cp.__db_ports:
                cp.db_type_index[v.db_type] = v
        cp.is_dirty = self.is_dirty
        return cp

    def db_children(self, parent=(None,None), orphan=False):
        children = []
        for child in self.db_ports:
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
        if orphan:
            self.db_ports = []
        children.append((self, parent[0], parent[1]))
        return children
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
        self.db_type_index[port.db_type] = port
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
        self.db_type_index[port.db_type] = port
    def db_delete_port(self, port):
        self.is_dirty = True
        for i in xrange(len(self.__db_ports)):
            if self.__db_ports[i].db_id == port.db_id:
                del self.__db_ports[i]
                break
        del self.db_type_index[port.db_type]
    def db_get_port(self, key):
        for i in xrange(len(self.__db_ports)):
            if self.__db_ports[i].db_id == key:
                return self.__db_ports[i]
        return None
    def db_get_port_by_type(self, key):
        return self.db_type_index[key]
    
    def getPrimaryKey(self):
        return self.__db_id

class DBTag(object):

    vtType = 'tag'

    def __init__(self, name=None, time=None):
        self.__db_name = name
        self.__db_time = time
        self.is_dirty = True
    
    def __copy__(self):
        cp = DBTag()
        cp.db_name = self.db_name
        cp.is_dirty = self.is_dirty
        cp.db_time = self.db_time
        cp.is_dirty = self.is_dirty
        return cp

    def db_children(self, parent=(None,None), orphan=False):
        children = []
        children.extend(self.db_vistrail.db_children((self.vtType, self.db_id), orphan))
        if orphan:
            self.db_vistrail = None
        children.append((self, parent[0], parent[1]))
        return children
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
    
    def __get_db_time(self):
        return self.__db_time
    def __set_db_time(self, time):
        self.__db_time = time
        self.is_dirty = True
    db_time = property(__get_db_time, __set_db_time)
    def db_add_time(self, time):
        self.__db_time = time
    def db_change_time(self, time):
        self.__db_time = time
    def db_delete_time(self, time):
        self.__db_time = None
    
    def getPrimaryKey(self):
        return self.__db_name

class DBExecRec(object):

    vtType = 'execRec'

    def __init__(self, id=None, tsStart=None, tsEnd=None, moduleId=None, moduleName=None, annotations=None):
        self.__db_id = id
        self.__db_tsStart = tsStart
        self.__db_tsEnd = tsEnd
        self.__db_moduleId = moduleId
        self.__db_moduleName = moduleName
        if annotations is None:
            self.__db_annotations = []
        else:
            self.__db_annotations = annotations
        self.is_dirty = True
    
    def __copy__(self):
        cp = DBExecRec()
        cp.db_id = self.db_id
        cp.is_dirty = self.is_dirty
        cp.db_tsStart = self.db_tsStart
        cp.is_dirty = self.is_dirty
        cp.db_tsEnd = self.db_tsEnd
        cp.is_dirty = self.is_dirty
        cp.db_moduleId = self.db_moduleId
        cp.is_dirty = self.is_dirty
        cp.db_moduleName = self.db_moduleName
        cp.is_dirty = self.is_dirty
        if self.db_annotations is None:
            cp.db_annotations = None
        else:
            cp.db_annotations = [copy.copy(v) for v in self.db_annotations]
        cp.is_dirty = self.is_dirty
        return cp

    def db_children(self, parent=(None,None), orphan=False):
        children = []
        for child in self.db_annotations:
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
        if orphan:
            self.db_annotations = []
        children.extend(self.db_wfExec.db_children((self.vtType, self.db_id), orphan))
        if orphan:
            self.db_wfExec = None
        children.append((self, parent[0], parent[1]))
        return children
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
    
    def __get_db_tsStart(self):
        return self.__db_tsStart
    def __set_db_tsStart(self, tsStart):
        self.__db_tsStart = tsStart
        self.is_dirty = True
    db_tsStart = property(__get_db_tsStart, __set_db_tsStart)
    def db_add_tsStart(self, tsStart):
        self.__db_tsStart = tsStart
    def db_change_tsStart(self, tsStart):
        self.__db_tsStart = tsStart
    def db_delete_tsStart(self, tsStart):
        self.__db_tsStart = None
    
    def __get_db_tsEnd(self):
        return self.__db_tsEnd
    def __set_db_tsEnd(self, tsEnd):
        self.__db_tsEnd = tsEnd
        self.is_dirty = True
    db_tsEnd = property(__get_db_tsEnd, __set_db_tsEnd)
    def db_add_tsEnd(self, tsEnd):
        self.__db_tsEnd = tsEnd
    def db_change_tsEnd(self, tsEnd):
        self.__db_tsEnd = tsEnd
    def db_delete_tsEnd(self, tsEnd):
        self.__db_tsEnd = None
    
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

class DBVistrail(object):

    vtType = 'vistrail'

    def __init__(self, id=None, version=None, name=None, dbHost=None, dbPort=None, dbName=None, actions=None, tags=None, macros=None):
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
        if tags is None:
            self.__db_tags = {}
        else:
            self.__db_tags = tags
        if macros is None:
            self.__db_macros = {}
        else:
            self.__db_macros = macros
        self.is_dirty = True
    
    def __copy__(self):
        cp = DBVistrail()
        cp.db_id = self.db_id
        cp.is_dirty = self.is_dirty
        cp.db_version = self.db_version
        cp.is_dirty = self.is_dirty
        cp.db_name = self.db_name
        cp.is_dirty = self.is_dirty
        cp.db_dbHost = self.db_dbHost
        cp.is_dirty = self.is_dirty
        cp.db_dbPort = self.db_dbPort
        cp.is_dirty = self.is_dirty
        cp.db_dbName = self.db_dbName
        cp.is_dirty = self.is_dirty
        if self.db_actions is None:
            cp.db_actions = None
        else:
            cp.db_actions = dict([(k,copy.copy(v)) for (k,v) in self.db_actions.iteritems()])
        cp.is_dirty = self.is_dirty
        if self.db_tags is None:
            cp.db_tags = None
        else:
            cp.db_tags = dict([(k,copy.copy(v)) for (k,v) in self.db_tags.iteritems()])
        cp.is_dirty = self.is_dirty
        if self.db_macros is None:
            cp.db_macros = None
        else:
            cp.db_macros = dict([(k,copy.copy(v)) for (k,v) in self.db_macros.iteritems()])
        cp.is_dirty = self.is_dirty
        return cp

    def db_children(self, parent=(None,None), orphan=False):
        children = []
        for child in self.db_actions.itervalues():
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
        if orphan:
            self.db_actions = {}
        for child in self.db_tags.itervalues():
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
        if orphan:
            self.db_tags = {}
        for child in self.db_macros.itervalues():
            children.extend(child.db_children((self.vtType, self.db_id), orphan))
        if orphan:
            self.db_macros = {}
        children.append((self, parent[0], parent[1]))
        return children
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
        self.__db_tags[tag.db_name] = tag
    def db_change_tag(self, tag):
        self.is_dirty = True
        self.__db_tags[tag.db_name] = tag
    def db_delete_tag(self, tag):
        self.is_dirty = True
        del self.__db_tags[tag.db_name]
    def db_get_tag(self, key):
        if self.__db_tags.has_key(key):
            return self.__db_tags[key]
        return None
    
    def __get_db_macros(self):
        return self.__db_macros
    def __set_db_macros(self, macros):
        self.__db_macros = macros
        self.is_dirty = True
    db_macros = property(__get_db_macros, __set_db_macros)
    def db_get_macros(self):
        return self.__db_macros.values()
    def db_add_macro(self, macro):
        self.is_dirty = True
        self.__db_macros[macro.db_id] = macro
    def db_change_macro(self, macro):
        self.is_dirty = True
        self.__db_macros[macro.db_id] = macro
    def db_delete_macro(self, macro):
        self.is_dirty = True
        del self.__db_macros[macro.db_id]
    def db_get_macro(self, key):
        if self.__db_macros.has_key(key):
            return self.__db_macros[key]
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
    
    def __copy__(self):
        cp = DBDelete()
        cp.db_id = self.db_id
        cp.is_dirty = self.is_dirty
        cp.db_what = self.db_what
        cp.is_dirty = self.is_dirty
        cp.db_objectId = self.db_objectId
        cp.is_dirty = self.is_dirty
        cp.db_parentObjId = self.db_parentObjId
        cp.is_dirty = self.is_dirty
        cp.db_parentObjType = self.db_parentObjType
        cp.is_dirty = self.is_dirty
        return cp

    def db_children(self, parent=(None,None), orphan=False):
        children = []
        children.extend(self.db_action.db_children((self.vtType, self.db_id), orphan))
        if orphan:
            self.db_action = None
        children.append((self, parent[0], parent[1]))
        return children
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

