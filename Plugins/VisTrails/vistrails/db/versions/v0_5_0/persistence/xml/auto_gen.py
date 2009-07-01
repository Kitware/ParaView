
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

from xml_dao import XMLDAO
from db.versions.v0_5_0.domain import *

class DBPortSpecXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'portSpec':
            return None
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        name = self.convertFromStr(self.getAttribute(node, 'name'), 'str')
        type = self.convertFromStr(self.getAttribute(node, 'type'), 'str')
        spec = self.convertFromStr(self.getAttribute(node, 'spec'), 'str')
        
        obj = DBPortSpec(id=id,
                         name=name,
                         type=type,
                         spec=spec)
        obj.is_dirty = False
        return obj
    
    def toXML(self, portSpec, doc):
        node = doc.createElement('portSpec')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(portSpec.db_id, 'long'))
        node.setAttribute('name',self.convertToStr(portSpec.db_name, 'str'))
        node.setAttribute('type',self.convertToStr(portSpec.db_type, 'str'))
        node.setAttribute('spec',self.convertToStr(portSpec.db_spec, 'str'))
        
        return node

class DBModuleXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'module':
            return None
        
        functions = []
        portSpecs = []
        
        annotations = {}
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        cache = self.convertFromStr(self.getAttribute(node, 'cache'), 'int')
        name = self.convertFromStr(self.getAttribute(node, 'name'), 'str')
        
        location = None
        
        # read children
        for child in node.childNodes:
            if child.nodeName == 'location':
                location = self.getDao('location').fromXML(child)
            elif child.nodeName == 'function':
                function = self.getDao('function').fromXML(child)
                functions.append(function)
            elif child.nodeName == 'annotation':
                annotation = self.getDao('annotation').fromXML(child)
                annotations[annotation.db_id] = annotation
            elif child.nodeName == 'portSpec':
                portSpec = self.getDao('portSpec').fromXML(child)
                portSpecs.append(portSpec)
            elif child.nodeType != child.TEXT_NODE:
                print '*** ERROR *** nodeName = %s' % child.nodeName
        
        obj = DBModule(id=id,
                       cache=cache,
                       name=name,
                       location=location,
                       functions=functions,
                       annotations=annotations,
                       portSpecs=portSpecs)
        obj.is_dirty = False
        return obj
    
    def toXML(self, module, doc):
        node = doc.createElement('module')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(module.db_id, 'long'))
        node.setAttribute('cache',self.convertToStr(module.db_cache, 'int'))
        node.setAttribute('name',self.convertToStr(module.db_name, 'str'))
        
        # set elements
        location = module.db_location
        if location is not None:
            node.appendChild(self.getDao('location').toXML(location, doc))
        functions = module.db_functions
        for function in functions:
            node.appendChild(self.getDao('function').toXML(function, doc))
        annotations = module.db_annotations
        for annotation in annotations.itervalues():
            node.appendChild(self.getDao('annotation').toXML(annotation, doc))
        portSpecs = module.db_portSpecs
        for portSpec in portSpecs:
            node.appendChild(self.getDao('portSpec').toXML(portSpec, doc))
        
        return node

class DBSessionXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'session':
            return None
        
        wfExecs = {}
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        user = self.convertFromStr(self.getAttribute(node, 'user'), 'str')
        ip = self.convertFromStr(self.getAttribute(node, 'ip'), 'str')
        visVersion = self.convertFromStr(self.getAttribute(node, 'visVersion'), 'str')
        tsStart = self.convertFromStr(self.getAttribute(node, 'tsStart'), 'datetime')
        tsEnd = self.convertFromStr(self.getAttribute(node, 'tsEnd'), 'datetime')
        machineId = self.convertFromStr(self.getAttribute(node, 'machineId'), 'long')
        
        
        # read children
        for child in node.childNodes:
            if child.nodeName == 'wfExec':
                wfExec = self.getDao('wfExec').fromXML(child)
                wfExecs[wfExec.db_id] = wfExec
            elif child.nodeType != child.TEXT_NODE:
                print '*** ERROR *** nodeName = %s' % child.nodeName
        
        obj = DBSession(id=id,
                        user=user,
                        ip=ip,
                        visVersion=visVersion,
                        tsStart=tsStart,
                        tsEnd=tsEnd,
                        machineId=machineId,
                        wfExecs=wfExecs)
        obj.is_dirty = False
        return obj
    
    def toXML(self, session, doc):
        node = doc.createElement('session')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(session.db_id, 'long'))
        node.setAttribute('user',self.convertToStr(session.db_user, 'str'))
        node.setAttribute('ip',self.convertToStr(session.db_ip, 'str'))
        node.setAttribute('visVersion',self.convertToStr(session.db_visVersion, 'str'))
        node.setAttribute('tsStart',self.convertToStr(session.db_tsStart, 'datetime'))
        node.setAttribute('tsEnd',self.convertToStr(session.db_tsEnd, 'datetime'))
        node.setAttribute('machineId',self.convertToStr(session.db_machineId, 'long'))
        
        # set elements
        wfExecs = session.db_wfExecs
        for wfExec in wfExecs.itervalues():
            node.appendChild(self.getDao('wfExec').toXML(wfExec, doc))
        
        return node

class DBPortXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'port':
            return None
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        type = self.convertFromStr(self.getAttribute(node, 'type'), 'str')
        moduleId = self.convertFromStr(self.getAttribute(node, 'moduleId'), 'long')
        moduleName = self.convertFromStr(self.getAttribute(node, 'moduleName'), 'str')
        sig = self.convertFromStr(self.getAttribute(node, 'sig'), 'str')
        
        obj = DBPort(id=id,
                     type=type,
                     moduleId=moduleId,
                     moduleName=moduleName,
                     sig=sig)
        obj.is_dirty = False
        return obj
    
    def toXML(self, port, doc):
        node = doc.createElement('port')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(port.db_id, 'long'))
        node.setAttribute('type',self.convertToStr(port.db_type, 'str'))
        node.setAttribute('moduleId',self.convertToStr(port.db_moduleId, 'long'))
        node.setAttribute('moduleName',self.convertToStr(port.db_moduleName, 'str'))
        node.setAttribute('sig',self.convertToStr(port.db_sig, 'str'))
        
        return node

class DBLogXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'log':
            return None
        
        sessions = {}
        machines = {}
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        
        
        # read children
        for child in node.childNodes:
            if child.nodeName == 'session':
                session = self.getDao('session').fromXML(child)
                sessions[session.db_id] = session
            elif child.nodeName == 'machine':
                machine = self.getDao('machine').fromXML(child)
                machines[machine.db_id] = machine
            elif child.nodeType != child.TEXT_NODE:
                print '*** ERROR *** nodeName = %s' % child.nodeName
        
        obj = DBLog(id=id,
                    sessions=sessions,
                    machines=machines)
        obj.is_dirty = False
        return obj
    
    def toXML(self, log, doc):
        node = doc.createElement('log')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(log.db_id, 'long'))
        
        # set elements
        sessions = log.db_sessions
        for session in sessions.itervalues():
            node.appendChild(self.getDao('session').toXML(session, doc))
        machines = log.db_machines
        for machine in machines.itervalues():
            node.appendChild(self.getDao('machine').toXML(machine, doc))
        
        return node

class DBMachineXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'machine':
            return None
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        name = self.convertFromStr(self.getAttribute(node, 'name'), 'str')
        os = self.convertFromStr(self.getAttribute(node, 'os'), 'str')
        architecture = self.convertFromStr(self.getAttribute(node, 'architecture'), 'str')
        processor = self.convertFromStr(self.getAttribute(node, 'processor'), 'str')
        ram = self.convertFromStr(self.getAttribute(node, 'ram'), 'int')
        
        obj = DBMachine(id=id,
                        name=name,
                        os=os,
                        architecture=architecture,
                        processor=processor,
                        ram=ram)
        obj.is_dirty = False
        return obj
    
    def toXML(self, machine, doc):
        node = doc.createElement('machine')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(machine.db_id, 'long'))
        node.setAttribute('name',self.convertToStr(machine.db_name, 'str'))
        node.setAttribute('os',self.convertToStr(machine.db_os, 'str'))
        node.setAttribute('architecture',self.convertToStr(machine.db_architecture, 'str'))
        node.setAttribute('processor',self.convertToStr(machine.db_processor, 'str'))
        node.setAttribute('ram',self.convertToStr(machine.db_ram, 'int'))
        
        return node

class DBAddXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'add':
            return None
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        what = self.convertFromStr(self.getAttribute(node, 'what'), 'str')
        objectId = self.convertFromStr(self.getAttribute(node, 'objectId'), 'long')
        parentObjId = self.convertFromStr(self.getAttribute(node, 'parentObjId'), 'long')
        parentObjType = self.convertFromStr(self.getAttribute(node, 'parentObjType'), 'str')
        
        data = None
        
        # read children
        for child in node.childNodes:
            if child.nodeName == 'module':
                data = self.getDao('module').fromXML(child)
            elif child.nodeName == 'location':
                data = self.getDao('location').fromXML(child)
            elif child.nodeName == 'annotation':
                data = self.getDao('annotation').fromXML(child)
            elif child.nodeName == 'function':
                data = self.getDao('function').fromXML(child)
            elif child.nodeName == 'connection':
                data = self.getDao('connection').fromXML(child)
            elif child.nodeName == 'port':
                data = self.getDao('port').fromXML(child)
            elif child.nodeName == 'parameter':
                data = self.getDao('parameter').fromXML(child)
            elif child.nodeName == 'portSpec':
                data = self.getDao('portSpec').fromXML(child)
            elif child.nodeName == 'other':
                data = self.getDao('other').fromXML(child)
            elif child.nodeType != child.TEXT_NODE:
                print '*** ERROR *** nodeName = %s' % child.nodeName
        
        obj = DBAdd(id=id,
                    what=what,
                    objectId=objectId,
                    parentObjId=parentObjId,
                    parentObjType=parentObjType,
                    data=data)
        obj.is_dirty = False
        return obj
    
    def toXML(self, add, doc):
        node = doc.createElement('add')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(add.db_id, 'long'))
        node.setAttribute('what',self.convertToStr(add.db_what, 'str'))
        node.setAttribute('objectId',self.convertToStr(add.db_objectId, 'long'))
        node.setAttribute('parentObjId',self.convertToStr(add.db_parentObjId, 'long'))
        node.setAttribute('parentObjType',self.convertToStr(add.db_parentObjType, 'str'))
        
        # set elements
        data = add.db_data
        if data.vtType == 'module':
            node.appendChild(self.getDao('module').toXML(data, doc))
        elif data.vtType == 'location':
            node.appendChild(self.getDao('location').toXML(data, doc))
        elif data.vtType == 'annotation':
            node.appendChild(self.getDao('annotation').toXML(data, doc))
        elif data.vtType == 'function':
            node.appendChild(self.getDao('function').toXML(data, doc))
        elif data.vtType == 'connection':
            node.appendChild(self.getDao('connection').toXML(data, doc))
        elif data.vtType == 'port':
            node.appendChild(self.getDao('port').toXML(data, doc))
        elif data.vtType == 'parameter':
            node.appendChild(self.getDao('parameter').toXML(data, doc))
        elif data.vtType == 'portSpec':
            node.appendChild(self.getDao('portSpec').toXML(data, doc))
        elif data.vtType == 'other':
            node.appendChild(self.getDao('other').toXML(data, doc))
        
        return node

class DBOtherXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'other':
            return None
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        key = self.convertFromStr(self.getAttribute(node, 'key'), 'str')
        
        value = None
        
        # read children
        for child in node.childNodes:
            if child.nodeName == 'value':
                value = self.convertFromStr(child.firstChild.nodeValue,'str')
            elif child.nodeType != child.TEXT_NODE:
                print '*** ERROR *** nodeName = %s' % child.nodeName
        
        obj = DBOther(id=id,
                      key=key,
                      value=value)
        obj.is_dirty = False
        return obj
    
    def toXML(self, other, doc):
        node = doc.createElement('other')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(other.db_id, 'long'))
        node.setAttribute('key',self.convertToStr(other.db_key, 'str'))
        
        # set elements
        if other.db_value is not None:
            child = other.db_value
            valueNode = doc.createElement('value')
            valueText = doc.createTextNode(self.convertToStr(child, 'str'))
            valueNode.appendChild(valueText)
            node.appendChild(valueNode)
        
        return node

class DBLocationXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'location':
            return None
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        x = self.convertFromStr(self.getAttribute(node, 'x'), 'float')
        y = self.convertFromStr(self.getAttribute(node, 'y'), 'float')
        
        obj = DBLocation(id=id,
                         x=x,
                         y=y)
        obj.is_dirty = False
        return obj
    
    def toXML(self, location, doc):
        node = doc.createElement('location')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(location.db_id, 'long'))
        node.setAttribute('x',self.convertToStr(location.db_x, 'float'))
        node.setAttribute('y',self.convertToStr(location.db_y, 'float'))
        
        return node

class DBWfExecXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'wfExec':
            return None
        
        execRecs = {}
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        tsStart = self.convertFromStr(self.getAttribute(node, 'tsStart'), 'datetime')
        tsEnd = self.convertFromStr(self.getAttribute(node, 'tsEnd'), 'datetime')
        wfVersion = self.convertFromStr(self.getAttribute(node, 'wfVersion'), 'int')
        vistrailId = self.convertFromStr(self.getAttribute(node, 'vistrailId'), 'long')
        vistrailName = self.convertFromStr(self.getAttribute(node, 'vistrailName'), 'str')
        
        
        # read children
        for child in node.childNodes:
            if child.nodeName == 'execRec':
                execRec = self.getDao('execRec').fromXML(child)
                execRecs[execRec.db_id] = execRec
            elif child.nodeType != child.TEXT_NODE:
                print '*** ERROR *** nodeName = %s' % child.nodeName
        
        obj = DBWfExec(id=id,
                       tsStart=tsStart,
                       tsEnd=tsEnd,
                       wfVersion=wfVersion,
                       vistrailId=vistrailId,
                       vistrailName=vistrailName,
                       execRecs=execRecs)
        obj.is_dirty = False
        return obj
    
    def toXML(self, wfExec, doc):
        node = doc.createElement('wfExec')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(wfExec.db_id, 'long'))
        node.setAttribute('tsStart',self.convertToStr(wfExec.db_tsStart, 'datetime'))
        node.setAttribute('tsEnd',self.convertToStr(wfExec.db_tsEnd, 'datetime'))
        node.setAttribute('wfVersion',self.convertToStr(wfExec.db_wfVersion, 'int'))
        node.setAttribute('vistrailId',self.convertToStr(wfExec.db_vistrailId, 'long'))
        node.setAttribute('vistrailName',self.convertToStr(wfExec.db_vistrailName, 'str'))
        
        # set elements
        execRecs = wfExec.db_execRecs
        for execRec in execRecs.itervalues():
            node.appendChild(self.getDao('execRec').toXML(execRec, doc))
        
        return node

class DBParameterXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'parameter':
            return None
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        pos = self.convertFromStr(self.getAttribute(node, 'pos'), 'long')
        name = self.convertFromStr(self.getAttribute(node, 'name'), 'str')
        type = self.convertFromStr(self.getAttribute(node, 'type'), 'str')
        val = self.convertFromStr(self.getAttribute(node, 'val'), 'str')
        alias = self.convertFromStr(self.getAttribute(node, 'alias'), 'str')
        
        obj = DBParameter(id=id,
                          pos=pos,
                          name=name,
                          type=type,
                          val=val,
                          alias=alias)
        obj.is_dirty = False
        return obj
    
    def toXML(self, parameter, doc):
        node = doc.createElement('parameter')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(parameter.db_id, 'long'))
        node.setAttribute('pos',self.convertToStr(parameter.db_pos, 'long'))
        node.setAttribute('name',self.convertToStr(parameter.db_name, 'str'))
        node.setAttribute('type',self.convertToStr(parameter.db_type, 'str'))
        node.setAttribute('val',self.convertToStr(parameter.db_val, 'str'))
        node.setAttribute('alias',self.convertToStr(parameter.db_alias, 'str'))
        
        return node

class DBFunctionXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'function':
            return None
        
        parameters = []
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        pos = self.convertFromStr(self.getAttribute(node, 'pos'), 'long')
        name = self.convertFromStr(self.getAttribute(node, 'name'), 'str')
        
        
        # read children
        for child in node.childNodes:
            if child.nodeName == 'parameter':
                parameter = self.getDao('parameter').fromXML(child)
                parameters.append(parameter)
            elif child.nodeType != child.TEXT_NODE:
                print '*** ERROR *** nodeName = %s' % child.nodeName
        
        obj = DBFunction(id=id,
                         pos=pos,
                         name=name,
                         parameters=parameters)
        obj.is_dirty = False
        return obj
    
    def toXML(self, function, doc):
        node = doc.createElement('function')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(function.db_id, 'long'))
        node.setAttribute('pos',self.convertToStr(function.db_pos, 'long'))
        node.setAttribute('name',self.convertToStr(function.db_name, 'str'))
        
        # set elements
        parameters = function.db_parameters
        for parameter in parameters:
            node.appendChild(self.getDao('parameter').toXML(parameter, doc))
        
        return node

class DBWorkflowXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'workflow':
            return None
        
        annotations = []
        others = []
        
        modules = {}
        connections = {}
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        name = self.convertFromStr(self.getAttribute(node, 'name'), 'str')
        
        
        # read children
        for child in node.childNodes:
            if child.nodeName == 'module':
                module = self.getDao('module').fromXML(child)
                modules[module.db_id] = module
            elif child.nodeName == 'connection':
                connection = self.getDao('connection').fromXML(child)
                connections[connection.db_id] = connection
            elif child.nodeName == 'annotation':
                annotation = self.getDao('annotation').fromXML(child)
                annotations.append(annotation)
            elif child.nodeName == 'other':
                other = self.getDao('other').fromXML(child)
                others.append(other)
            elif child.nodeType != child.TEXT_NODE:
                print '*** ERROR *** nodeName = %s' % child.nodeName
        
        obj = DBWorkflow(id=id,
                         name=name,
                         modules=modules,
                         connections=connections,
                         annotations=annotations,
                         others=others)
        obj.is_dirty = False
        return obj
    
    def toXML(self, workflow, doc):
        node = doc.createElement('workflow')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(workflow.db_id, 'long'))
        node.setAttribute('name',self.convertToStr(workflow.db_name, 'str'))
        
        # set elements
        modules = workflow.db_modules
        for module in modules.itervalues():
            node.appendChild(self.getDao('module').toXML(module, doc))
        connections = workflow.db_connections
        for connection in connections.itervalues():
            node.appendChild(self.getDao('connection').toXML(connection, doc))
        annotations = workflow.db_annotations
        for annotation in annotations:
            node.appendChild(self.getDao('annotation').toXML(annotation, doc))
        others = workflow.db_others
        for other in others:
            node.appendChild(self.getDao('other').toXML(other, doc))
        
        return node

class DBActionXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'action':
            return None
        
        operations = []
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        prevId = self.convertFromStr(self.getAttribute(node, 'prevId'), 'long')
        date = self.convertFromStr(self.getAttribute(node, 'date'), 'datetime')
        user = self.convertFromStr(self.getAttribute(node, 'user'), 'str')
        
        
        # read children
        for child in node.childNodes:
            if child.nodeName == 'add':
                operation = self.getDao('add').fromXML(child)
                operations.append(operation)
            elif child.nodeName == 'delete':
                operation = self.getDao('delete').fromXML(child)
                operations.append(operation)
            elif child.nodeName == 'change':
                operation = self.getDao('change').fromXML(child)
                operations.append(operation)
            elif child.nodeType != child.TEXT_NODE:
                print '*** ERROR *** nodeName = %s' % child.nodeName
        
        obj = DBAction(id=id,
                       prevId=prevId,
                       date=date,
                       user=user,
                       operations=operations)
        obj.is_dirty = False
        return obj
    
    def toXML(self, action, doc):
        node = doc.createElement('action')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(action.db_id, 'long'))
        node.setAttribute('prevId',self.convertToStr(action.db_prevId, 'long'))
        node.setAttribute('date',self.convertToStr(action.db_date, 'datetime'))
        node.setAttribute('user',self.convertToStr(action.db_user, 'str'))
        
        # set elements
        operations = action.db_operations
        for operation in operations:
            if operation.vtType == 'add':
                node.appendChild(self.getDao('add').toXML(operation, doc))
            elif operation.vtType == 'delete':
                node.appendChild(self.getDao('delete').toXML(operation, doc))
            elif operation.vtType == 'change':
                node.appendChild(self.getDao('change').toXML(operation, doc))
        
        return node

class DBAnnotationXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'annotation':
            return None
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        key = self.convertFromStr(self.getAttribute(node, 'key'), 'str')
        value = self.convertFromStr(self.getAttribute(node, 'value'), 'str')
        
        obj = DBAnnotation(id=id,
                           key=key,
                           value=value)
        obj.is_dirty = False
        return obj
    
    def toXML(self, annotation, doc):
        node = doc.createElement('annotation')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(annotation.db_id, 'long'))
        node.setAttribute('key',self.convertToStr(annotation.db_key, 'str'))
        node.setAttribute('value',self.convertToStr(annotation.db_value, 'str'))
        
        return node

class DBChangeXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'change':
            return None
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        what = self.convertFromStr(self.getAttribute(node, 'what'), 'str')
        oldObjId = self.convertFromStr(self.getAttribute(node, 'oldObjId'), 'long')
        newObjId = self.convertFromStr(self.getAttribute(node, 'newObjId'), 'long')
        parentObjId = self.convertFromStr(self.getAttribute(node, 'parentObjId'), 'long')
        parentObjType = self.convertFromStr(self.getAttribute(node, 'parentObjType'), 'str')
        
        data = None
        
        # read children
        for child in node.childNodes:
            if child.nodeName == 'module':
                data = self.getDao('module').fromXML(child)
            elif child.nodeName == 'location':
                data = self.getDao('location').fromXML(child)
            elif child.nodeName == 'annotation':
                data = self.getDao('annotation').fromXML(child)
            elif child.nodeName == 'function':
                data = self.getDao('function').fromXML(child)
            elif child.nodeName == 'connection':
                data = self.getDao('connection').fromXML(child)
            elif child.nodeName == 'port':
                data = self.getDao('port').fromXML(child)
            elif child.nodeName == 'parameter':
                data = self.getDao('parameter').fromXML(child)
            elif child.nodeName == 'portSpec':
                data = self.getDao('portSpec').fromXML(child)
            elif child.nodeName == 'other':
                data = self.getDao('other').fromXML(child)
            elif child.nodeType != child.TEXT_NODE:
                print '*** ERROR *** nodeName = %s' % child.nodeName
        
        obj = DBChange(id=id,
                       what=what,
                       oldObjId=oldObjId,
                       newObjId=newObjId,
                       parentObjId=parentObjId,
                       parentObjType=parentObjType,
                       data=data)
        obj.is_dirty = False
        return obj
    
    def toXML(self, change, doc):
        node = doc.createElement('change')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(change.db_id, 'long'))
        node.setAttribute('what',self.convertToStr(change.db_what, 'str'))
        node.setAttribute('oldObjId',self.convertToStr(change.db_oldObjId, 'long'))
        node.setAttribute('newObjId',self.convertToStr(change.db_newObjId, 'long'))
        node.setAttribute('parentObjId',self.convertToStr(change.db_parentObjId, 'long'))
        node.setAttribute('parentObjType',self.convertToStr(change.db_parentObjType, 'str'))
        
        # set elements
        data = change.db_data
        if data.vtType == 'module':
            node.appendChild(self.getDao('module').toXML(data, doc))
        elif data.vtType == 'location':
            node.appendChild(self.getDao('location').toXML(data, doc))
        elif data.vtType == 'annotation':
            node.appendChild(self.getDao('annotation').toXML(data, doc))
        elif data.vtType == 'function':
            node.appendChild(self.getDao('function').toXML(data, doc))
        elif data.vtType == 'connection':
            node.appendChild(self.getDao('connection').toXML(data, doc))
        elif data.vtType == 'port':
            node.appendChild(self.getDao('port').toXML(data, doc))
        elif data.vtType == 'parameter':
            node.appendChild(self.getDao('parameter').toXML(data, doc))
        elif data.vtType == 'portSpec':
            node.appendChild(self.getDao('portSpec').toXML(data, doc))
        elif data.vtType == 'other':
            node.appendChild(self.getDao('other').toXML(data, doc))
        
        return node

class DBMacroXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'macro':
            return None
        
        actions = {}
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        name = self.convertFromStr(self.getAttribute(node, 'name'), 'str')
        descrptn = self.convertFromStr(self.getAttribute(node, 'descrptn'), 'str')
        
        
        # read children
        for child in node.childNodes:
            if child.nodeName == 'action':
                action = self.getDao('action').fromXML(child)
                actions[action.db_id] = action
            elif child.nodeType != child.TEXT_NODE:
                print '*** ERROR *** nodeName = %s' % child.nodeName
        
        obj = DBMacro(id=id,
                      name=name,
                      descrptn=descrptn,
                      actions=actions)
        obj.is_dirty = False
        return obj
    
    def toXML(self, macro, doc):
        node = doc.createElement('macro')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(macro.db_id, 'long'))
        node.setAttribute('name',self.convertToStr(macro.db_name, 'str'))
        node.setAttribute('descrptn',self.convertToStr(macro.db_descrptn, 'str'))
        
        # set elements
        actions = macro.db_actions
        for action in actions.itervalues():
            node.appendChild(self.getDao('action').toXML(action, doc))
        
        return node

class DBConnectionXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'connection':
            return None
        
        ports = []
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        
        
        # read children
        for child in node.childNodes:
            if child.nodeName == 'port':
                port = self.getDao('port').fromXML(child)
                ports.append(port)
            elif child.nodeType != child.TEXT_NODE:
                print '*** ERROR *** nodeName = %s' % child.nodeName
        
        obj = DBConnection(id=id,
                           ports=ports)
        obj.is_dirty = False
        return obj
    
    def toXML(self, connection, doc):
        node = doc.createElement('connection')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(connection.db_id, 'long'))
        
        # set elements
        ports = connection.db_ports
        for port in ports:
            node.appendChild(self.getDao('port').toXML(port, doc))
        
        return node

class DBTagXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'tag':
            return None
        
        # read attributes
        name = self.convertFromStr(self.getAttribute(node, 'name'), 'str')
        time = self.convertFromStr(self.getAttribute(node, 'time'), 'long')
        
        obj = DBTag(name=name,
                    time=time)
        obj.is_dirty = False
        return obj
    
    def toXML(self, tag, doc):
        node = doc.createElement('tag')
        
        # set attributes
        node.setAttribute('name',self.convertToStr(tag.db_name, 'str'))
        node.setAttribute('time',self.convertToStr(tag.db_time, 'long'))
        
        return node

class DBExecRecXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'exec':
            return None
        
        annotations = []
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        tsStart = self.convertFromStr(self.getAttribute(node, 'tsStart'), 'datetime')
        tsEnd = self.convertFromStr(self.getAttribute(node, 'tsEnd'), 'datetime')
        moduleId = self.convertFromStr(self.getAttribute(node, 'moduleId'), 'long')
        moduleName = self.convertFromStr(self.getAttribute(node, 'moduleName'), 'str')
        
        
        # read children
        for child in node.childNodes:
            if child.nodeName == 'annotation':
                annotation = self.getDao('annotation').fromXML(child)
                annotations.append(annotation)
            elif child.nodeType != child.TEXT_NODE:
                print '*** ERROR *** nodeName = %s' % child.nodeName
        
        obj = DBExecRec(id=id,
                        tsStart=tsStart,
                        tsEnd=tsEnd,
                        moduleId=moduleId,
                        moduleName=moduleName,
                        annotations=annotations)
        obj.is_dirty = False
        return obj
    
    def toXML(self, execRec, doc):
        node = doc.createElement('exec')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(execRec.db_id, 'long'))
        node.setAttribute('tsStart',self.convertToStr(execRec.db_tsStart, 'datetime'))
        node.setAttribute('tsEnd',self.convertToStr(execRec.db_tsEnd, 'datetime'))
        node.setAttribute('moduleId',self.convertToStr(execRec.db_moduleId, 'long'))
        node.setAttribute('moduleName',self.convertToStr(execRec.db_moduleName, 'str'))
        
        # set elements
        annotations = execRec.db_annotations
        for annotation in annotations:
            node.appendChild(self.getDao('annotation').toXML(annotation, doc))
        
        return node

class DBVistrailXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'vistrail':
            return None
        
        actions = {}
        tags = {}
        macros = {}
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        version = self.convertFromStr(self.getAttribute(node, 'version'), 'str')
        name = self.convertFromStr(self.getAttribute(node, 'name'), 'str')
        dbHost = self.convertFromStr(self.getAttribute(node, 'dbHost'), 'str')
        dbPort = self.convertFromStr(self.getAttribute(node, 'dbPort'), 'int')
        dbName = self.convertFromStr(self.getAttribute(node, 'dbName'), 'str')
        
        
        # read children
        for child in node.childNodes:
            if child.nodeName == 'action':
                action = self.getDao('action').fromXML(child)
                actions[action.db_id] = action
            elif child.nodeName == 'tag':
                tag = self.getDao('tag').fromXML(child)
                tags[tag.db_name] = tag
            elif child.nodeName == 'macro':
                macro = self.getDao('macro').fromXML(child)
                macros[macro.db_id] = macro
            elif child.nodeType != child.TEXT_NODE:
                print '*** ERROR *** nodeName = %s' % child.nodeName
        
        obj = DBVistrail(id=id,
                         version=version,
                         name=name,
                         dbHost=dbHost,
                         dbPort=dbPort,
                         dbName=dbName,
                         actions=actions,
                         tags=tags,
                         macros=macros)
        obj.is_dirty = False
        return obj
    
    def toXML(self, vistrail, doc):
        node = doc.createElement('vistrail')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(vistrail.db_id, 'long'))
        node.setAttribute('version',self.convertToStr(vistrail.db_version, 'str'))
        node.setAttribute('name',self.convertToStr(vistrail.db_name, 'str'))
        node.setAttribute('dbHost',self.convertToStr(vistrail.db_dbHost, 'str'))
        node.setAttribute('dbPort',self.convertToStr(vistrail.db_dbPort, 'int'))
        node.setAttribute('dbName',self.convertToStr(vistrail.db_dbName, 'str'))
        
        # set elements
        actions = vistrail.db_actions
        for action in actions.itervalues():
            node.appendChild(self.getDao('action').toXML(action, doc))
        tags = vistrail.db_tags
        for tag in tags.itervalues():
            node.appendChild(self.getDao('tag').toXML(tag, doc))
        macros = vistrail.db_macros
        for macro in macros.itervalues():
            node.appendChild(self.getDao('macro').toXML(macro, doc))
        
        return node

class DBDeleteXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'delete':
            return None
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        what = self.convertFromStr(self.getAttribute(node, 'what'), 'str')
        objectId = self.convertFromStr(self.getAttribute(node, 'objectId'), 'long')
        parentObjId = self.convertFromStr(self.getAttribute(node, 'parentObjId'), 'long')
        parentObjType = self.convertFromStr(self.getAttribute(node, 'parentObjType'), 'str')
        
        obj = DBDelete(id=id,
                       what=what,
                       objectId=objectId,
                       parentObjId=parentObjId,
                       parentObjType=parentObjType)
        obj.is_dirty = False
        return obj
    
    def toXML(self, delete, doc):
        node = doc.createElement('delete')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(delete.db_id, 'long'))
        node.setAttribute('what',self.convertToStr(delete.db_what, 'str'))
        node.setAttribute('objectId',self.convertToStr(delete.db_objectId, 'long'))
        node.setAttribute('parentObjId',self.convertToStr(delete.db_parentObjId, 'long'))
        node.setAttribute('parentObjType',self.convertToStr(delete.db_parentObjType, 'str'))
        
        return node

"""generated automatically by auto_dao.py"""

class XMLDAOListBase(dict):

    def __init__(self, daos=None):
        if daos is not None:
            dict.update(self, daos)

        if 'portSpec' not in self:
            self['portSpec'] = DBPortSpecXMLDAOBase(self)
        if 'module' not in self:
            self['module'] = DBModuleXMLDAOBase(self)
        if 'session' not in self:
            self['session'] = DBSessionXMLDAOBase(self)
        if 'port' not in self:
            self['port'] = DBPortXMLDAOBase(self)
        if 'log' not in self:
            self['log'] = DBLogXMLDAOBase(self)
        if 'machine' not in self:
            self['machine'] = DBMachineXMLDAOBase(self)
        if 'add' not in self:
            self['add'] = DBAddXMLDAOBase(self)
        if 'other' not in self:
            self['other'] = DBOtherXMLDAOBase(self)
        if 'location' not in self:
            self['location'] = DBLocationXMLDAOBase(self)
        if 'wfExec' not in self:
            self['wfExec'] = DBWfExecXMLDAOBase(self)
        if 'parameter' not in self:
            self['parameter'] = DBParameterXMLDAOBase(self)
        if 'function' not in self:
            self['function'] = DBFunctionXMLDAOBase(self)
        if 'workflow' not in self:
            self['workflow'] = DBWorkflowXMLDAOBase(self)
        if 'action' not in self:
            self['action'] = DBActionXMLDAOBase(self)
        if 'annotation' not in self:
            self['annotation'] = DBAnnotationXMLDAOBase(self)
        if 'change' not in self:
            self['change'] = DBChangeXMLDAOBase(self)
        if 'macro' not in self:
            self['macro'] = DBMacroXMLDAOBase(self)
        if 'connection' not in self:
            self['connection'] = DBConnectionXMLDAOBase(self)
        if 'tag' not in self:
            self['tag'] = DBTagXMLDAOBase(self)
        if 'execRec' not in self:
            self['execRec'] = DBExecRecXMLDAOBase(self)
        if 'vistrail' not in self:
            self['vistrail'] = DBVistrailXMLDAOBase(self)
        if 'delete' not in self:
            self['delete'] = DBDeleteXMLDAOBase(self)
