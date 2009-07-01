
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
from db.versions.v0_6_0.domain import *

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
    
    def toXML(self, portSpec, doc, node):
        if node is None:
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
        
        annotations = {}
        portSpecs = {}
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        cache = self.convertFromStr(self.getAttribute(node, 'cache'), 'int')
        name = self.convertFromStr(self.getAttribute(node, 'name'), 'str')
        package = self.convertFromStr(self.getAttribute(node, 'package'), 'str')
        version = self.convertFromStr(self.getAttribute(node, 'version'), 'str')
        
        location = None
        
        # read children
        for child in list(node.childNodes):
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
                portSpecs[portSpec.db_id] = portSpec
            elif child.nodeType == child.TEXT_NODE and child.nodeValue.strip() == '':
                # node.removeChild(child)
                pass
            elif child.nodeType != child.TEXT_NODE:
                print '*** ERROR *** nodeName = %s' % child.nodeName
        
        obj = DBModule(id=id,
                       cache=cache,
                       name=name,
                       package=package,
                       version=version,
                       location=location,
                       functions=functions,
                       annotations=annotations,
                       portSpecs=portSpecs)
        obj.is_dirty = False
        return obj
    
    def toXML(self, module, doc, node):
        if node is None:
            node = doc.createElement('module')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(module.db_id, 'long'))
        node.setAttribute('cache',self.convertToStr(module.db_cache, 'int'))
        node.setAttribute('name',self.convertToStr(module.db_name, 'str'))
        node.setAttribute('package',self.convertToStr(module.db_package, 'str'))
        node.setAttribute('version',self.convertToStr(module.db_version, 'str'))
        
        # load DOM node map
        nodeMap = {}
        for childNode in node.childNodes:
            if childNode.nodeType == childNode.ELEMENT_NODE and self.hasAttribute(childNode, 'id'):
                nodeId = self.convertFromStr(self.getAttribute(childNode, 'id'), 'long')
                nodeMap[(childNode.nodeName, nodeId)] = childNode
        
        # set elements
        location = module.db_location
        if location is not None:
            if nodeMap.has_key(('location', location.db_id)):
                childNode = nodeMap[('location', location.db_id)]
                del nodeMap[('location', location.db_id)]
            else:
                childNode = doc.createElement('location')
                node.appendChild(childNode)
            self.getDao('location').toXML(location, doc, childNode)
        functions = module.db_functions
        for function in functions:
            if nodeMap.has_key(('function', function.db_id)):
                childNode = nodeMap[('function', function.db_id)]
                del nodeMap[('function', function.db_id)]
            else:
                childNode = doc.createElement('function')
                node.appendChild(childNode)
            self.getDao('function').toXML(function, doc, childNode)
        annotations = module.db_annotations
        for annotation in annotations.itervalues():
            if nodeMap.has_key(('annotation', annotation.db_id)):
                childNode = nodeMap[('annotation', annotation.db_id)]
                del nodeMap[('annotation', annotation.db_id)]
            else:
                childNode = doc.createElement('annotation')
                node.appendChild(childNode)
            self.getDao('annotation').toXML(annotation, doc, childNode)
        portSpecs = module.db_portSpecs
        for portSpec in portSpecs.itervalues():
            if nodeMap.has_key(('portSpec', portSpec.db_id)):
                childNode = nodeMap[('portSpec', portSpec.db_id)]
                del nodeMap[('portSpec', portSpec.db_id)]
            else:
                childNode = doc.createElement('portSpec')
                node.appendChild(childNode)
            self.getDao('portSpec').toXML(portSpec, doc, childNode)
        
        # delete nodes not around anymore
        for childNode in nodeMap.itervalues():
            childNode.parentNode.removeChild(childNode)
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
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        name = self.convertFromStr(self.getAttribute(node, 'name'), 'str')
        
        obj = DBTag(id=id,
                    name=name)
        obj.is_dirty = False
        return obj
    
    def toXML(self, tag, doc, node):
        if node is None:
            node = doc.createElement('tag')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(tag.db_id, 'long'))
        node.setAttribute('name',self.convertToStr(tag.db_name, 'str'))
        
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
    
    def toXML(self, port, doc, node):
        if node is None:
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
        
        workflow_execs = {}
        machines = {}
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        
        
        # read children
        for child in list(node.childNodes):
            if child.nodeName == 'workflowExec':
                workflow_exec = self.getDao('workflow_exec').fromXML(child)
                workflow_execs[workflow_exec.db_id] = workflow_exec
            elif child.nodeName == 'machine':
                machine = self.getDao('machine').fromXML(child)
                machines[machine.db_id] = machine
            elif child.nodeType == child.TEXT_NODE and child.nodeValue.strip() == '':
                # node.removeChild(child)
                pass
            elif child.nodeType != child.TEXT_NODE:
                print '*** ERROR *** nodeName = %s' % child.nodeName
        
        obj = DBLog(id=id,
                    workflow_execs=workflow_execs,
                    machines=machines)
        obj.is_dirty = False
        return obj
    
    def toXML(self, log, doc, node):
        if node is None:
            node = doc.createElement('log')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(log.db_id, 'long'))
        
        # load DOM node map
        nodeMap = {}
        for childNode in node.childNodes:
            if childNode.nodeType == childNode.ELEMENT_NODE and self.hasAttribute(childNode, 'id'):
                nodeId = self.convertFromStr(self.getAttribute(childNode, 'id'), 'long')
                nodeMap[(childNode.nodeName, nodeId)] = childNode
        
        # set elements
        workflow_execs = log.db_workflow_execs
        for workflow_exec in workflow_execs.itervalues():
            if nodeMap.has_key(('workflow_exec', workflow_exec.db_id)):
                childNode = nodeMap[('workflow_exec', workflow_exec.db_id)]
                del nodeMap[('workflow_exec', workflow_exec.db_id)]
            else:
                childNode = doc.createElement('workflow_exec')
                node.appendChild(childNode)
            self.getDao('workflow_exec').toXML(workflow_exec, doc, childNode)
        machines = log.db_machines
        for machine in machines.itervalues():
            if nodeMap.has_key(('machine', machine.db_id)):
                childNode = nodeMap[('machine', machine.db_id)]
                del nodeMap[('machine', machine.db_id)]
            else:
                childNode = doc.createElement('machine')
                node.appendChild(childNode)
            self.getDao('machine').toXML(machine, doc, childNode)
        
        # delete nodes not around anymore
        for childNode in nodeMap.itervalues():
            childNode.parentNode.removeChild(childNode)
        return node

class DBMachineXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'machine':
            return None
        
        module_execs = []
        
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
                        ram=ram,
                        module_execs=module_execs)
        obj.is_dirty = False
        return obj
    
    def toXML(self, machine, doc, node):
        if node is None:
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
        for child in list(node.childNodes):
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
            elif child.nodeName == 'abstractionRef':
                data = self.getDao('abstractionRef').fromXML(child)
            elif child.nodeName == 'other':
                data = self.getDao('other').fromXML(child)
            elif child.nodeType == child.TEXT_NODE and child.nodeValue.strip() == '':
                # node.removeChild(child)
                pass
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
    
    def toXML(self, add, doc, node):
        if node is None:
            node = doc.createElement('add')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(add.db_id, 'long'))
        node.setAttribute('what',self.convertToStr(add.db_what, 'str'))
        node.setAttribute('objectId',self.convertToStr(add.db_objectId, 'long'))
        node.setAttribute('parentObjId',self.convertToStr(add.db_parentObjId, 'long'))
        node.setAttribute('parentObjType',self.convertToStr(add.db_parentObjType, 'str'))
        
        # load DOM node map
        nodeMap = {}
        for childNode in node.childNodes:
            if childNode.nodeType == childNode.ELEMENT_NODE and self.hasAttribute(childNode, 'id'):
                nodeId = self.convertFromStr(self.getAttribute(childNode, 'id'), 'long')
                nodeMap[(childNode.nodeName, nodeId)] = childNode
        
        # set elements
        data = add.db_data
        if data.vtType == 'module':
            if nodeMap.has_key(('module', data.db_id)):
                childNode = nodeMap[('module', data.db_id)]
                del nodeMap[('module', data.db_id)]
            else:
                childNode = doc.createElement('module')
                node.appendChild(childNode)
            self.getDao('module').toXML(data, doc, childNode)
        elif data.vtType == 'location':
            if nodeMap.has_key(('location', data.db_id)):
                childNode = nodeMap[('location', data.db_id)]
                del nodeMap[('location', data.db_id)]
            else:
                childNode = doc.createElement('location')
                node.appendChild(childNode)
            self.getDao('location').toXML(data, doc, childNode)
        elif data.vtType == 'annotation':
            if nodeMap.has_key(('annotation', data.db_id)):
                childNode = nodeMap[('annotation', data.db_id)]
                del nodeMap[('annotation', data.db_id)]
            else:
                childNode = doc.createElement('annotation')
                node.appendChild(childNode)
            self.getDao('annotation').toXML(data, doc, childNode)
        elif data.vtType == 'function':
            if nodeMap.has_key(('function', data.db_id)):
                childNode = nodeMap[('function', data.db_id)]
                del nodeMap[('function', data.db_id)]
            else:
                childNode = doc.createElement('function')
                node.appendChild(childNode)
            self.getDao('function').toXML(data, doc, childNode)
        elif data.vtType == 'connection':
            if nodeMap.has_key(('connection', data.db_id)):
                childNode = nodeMap[('connection', data.db_id)]
                del nodeMap[('connection', data.db_id)]
            else:
                childNode = doc.createElement('connection')
                node.appendChild(childNode)
            self.getDao('connection').toXML(data, doc, childNode)
        elif data.vtType == 'port':
            if nodeMap.has_key(('port', data.db_id)):
                childNode = nodeMap[('port', data.db_id)]
                del nodeMap[('port', data.db_id)]
            else:
                childNode = doc.createElement('port')
                node.appendChild(childNode)
            self.getDao('port').toXML(data, doc, childNode)
        elif data.vtType == 'parameter':
            if nodeMap.has_key(('parameter', data.db_id)):
                childNode = nodeMap[('parameter', data.db_id)]
                del nodeMap[('parameter', data.db_id)]
            else:
                childNode = doc.createElement('parameter')
                node.appendChild(childNode)
            self.getDao('parameter').toXML(data, doc, childNode)
        elif data.vtType == 'portSpec':
            if nodeMap.has_key(('portSpec', data.db_id)):
                childNode = nodeMap[('portSpec', data.db_id)]
                del nodeMap[('portSpec', data.db_id)]
            else:
                childNode = doc.createElement('portSpec')
                node.appendChild(childNode)
            self.getDao('portSpec').toXML(data, doc, childNode)
        elif data.vtType == 'abstractionRef':
            if nodeMap.has_key(('abstractionRef', data.db_id)):
                childNode = nodeMap[('abstractionRef', data.db_id)]
                del nodeMap[('abstractionRef', data.db_id)]
            else:
                childNode = doc.createElement('abstractionRef')
                node.appendChild(childNode)
            self.getDao('abstractionRef').toXML(data, doc, childNode)
        elif data.vtType == 'other':
            if nodeMap.has_key(('other', data.db_id)):
                childNode = nodeMap[('other', data.db_id)]
                del nodeMap[('other', data.db_id)]
            else:
                childNode = doc.createElement('other')
                node.appendChild(childNode)
            self.getDao('other').toXML(data, doc, childNode)
        
        # delete nodes not around anymore
        for childNode in nodeMap.itervalues():
            childNode.parentNode.removeChild(childNode)
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
        for child in list(node.childNodes):
            if child.nodeName == 'value':
                value = self.convertFromStr(child.firstChild.nodeValue,'str')
            elif child.nodeType == child.TEXT_NODE and child.nodeValue.strip() == '':
                # node.removeChild(child)
                pass
            elif child.nodeType != child.TEXT_NODE:
                print '*** ERROR *** nodeName = %s' % child.nodeName
        
        obj = DBOther(id=id,
                      key=key,
                      value=value)
        obj.is_dirty = False
        return obj
    
    def toXML(self, other, doc, node):
        if node is None:
            node = doc.createElement('other')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(other.db_id, 'long'))
        node.setAttribute('key',self.convertToStr(other.db_key, 'str'))
        
        # load DOM node map
        nodeMap = {}
        for childNode in node.childNodes:
            if childNode.nodeType == childNode.ELEMENT_NODE and self.hasAttribute(childNode, 'id'):
                nodeId = self.convertFromStr(self.getAttribute(childNode, 'id'), 'long')
                nodeMap[(childNode.nodeName, nodeId)] = childNode
        
        # set elements
        if other.db_value is not None:
            child = other.db_value
            if nodeMap.has_key(('value',child.db_id)):
                childNode = nodeMap[('value',child.db_id)]
                del nodeMap[('value', child.db_id)]
                textNode = childNode.firstChild
                textNode.replaceWholeText(self.convertToStr(child, 'str'))
            else:
                childNode = doc.createElement('value')
                node.appendChild(childNode)
                textNode = doc.createTextNode(self.convertToStr(child, 'str'))
                childNode.appendChild(textNode)
        
        # delete nodes not around anymore
        for childNode in nodeMap.itervalues():
            childNode.parentNode.removeChild(childNode)
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
    
    def toXML(self, location, doc, node):
        if node is None:
            node = doc.createElement('location')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(location.db_id, 'long'))
        node.setAttribute('x',self.convertToStr(location.db_x, 'float'))
        node.setAttribute('y',self.convertToStr(location.db_y, 'float'))
        
        return node

class DBWorkflowExecXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'workflowExec':
            return None
        
        module_execs = {}
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        user = self.convertFromStr(self.getAttribute(node, 'user'), 'str')
        ip = self.convertFromStr(self.getAttribute(node, 'ip'), 'str')
        vt_version = self.convertFromStr(self.getAttribute(node, 'vtVersion'), 'str')
        ts_start = self.convertFromStr(self.getAttribute(node, 'tsStart'), 'datetime')
        ts_end = self.convertFromStr(self.getAttribute(node, 'tsEnd'), 'datetime')
        parent_id = self.convertFromStr(self.getAttribute(node, 'parentId'), 'long')
        parent_type = self.convertFromStr(self.getAttribute(node, 'parentType'), 'str')
        parent_version = self.convertFromStr(self.getAttribute(node, 'parentVersion'), 'long')
        name = self.convertFromStr(self.getAttribute(node, 'name'), 'str')
        
        
        # read children
        for child in list(node.childNodes):
            if child.nodeName == 'moduleExec':
                module_exec = self.getDao('module_exec').fromXML(child)
                module_execs[module_exec.db_id] = module_exec
            elif child.nodeType == child.TEXT_NODE and child.nodeValue.strip() == '':
                # node.removeChild(child)
                pass
            elif child.nodeType != child.TEXT_NODE:
                print '*** ERROR *** nodeName = %s' % child.nodeName
        
        obj = DBWorkflowExec(id=id,
                             user=user,
                             ip=ip,
                             vt_version=vt_version,
                             ts_start=ts_start,
                             ts_end=ts_end,
                             parent_id=parent_id,
                             parent_type=parent_type,
                             parent_version=parent_version,
                             name=name,
                             module_execs=module_execs)
        obj.is_dirty = False
        return obj
    
    def toXML(self, workflow_exec, doc, node):
        if node is None:
            node = doc.createElement('workflowExec')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(workflow_exec.db_id, 'long'))
        node.setAttribute('user',self.convertToStr(workflow_exec.db_user, 'str'))
        node.setAttribute('ip',self.convertToStr(workflow_exec.db_ip, 'str'))
        node.setAttribute('vtVersion',self.convertToStr(workflow_exec.db_vt_version, 'str'))
        node.setAttribute('tsStart',self.convertToStr(workflow_exec.db_ts_start, 'datetime'))
        node.setAttribute('tsEnd',self.convertToStr(workflow_exec.db_ts_end, 'datetime'))
        node.setAttribute('parentId',self.convertToStr(workflow_exec.db_parent_id, 'long'))
        node.setAttribute('parentType',self.convertToStr(workflow_exec.db_parent_type, 'str'))
        node.setAttribute('parentVersion',self.convertToStr(workflow_exec.db_parent_version, 'long'))
        node.setAttribute('name',self.convertToStr(workflow_exec.db_name, 'str'))
        
        # load DOM node map
        nodeMap = {}
        for childNode in node.childNodes:
            if childNode.nodeType == childNode.ELEMENT_NODE and self.hasAttribute(childNode, 'id'):
                nodeId = self.convertFromStr(self.getAttribute(childNode, 'id'), 'long')
                nodeMap[(childNode.nodeName, nodeId)] = childNode
        
        # set elements
        module_execs = workflow_exec.db_module_execs
        for module_exec in module_execs.itervalues():
            if nodeMap.has_key(('module_exec', module_exec.db_id)):
                childNode = nodeMap[('module_exec', module_exec.db_id)]
                del nodeMap[('module_exec', module_exec.db_id)]
            else:
                childNode = doc.createElement('module_exec')
                node.appendChild(childNode)
            self.getDao('module_exec').toXML(module_exec, doc, childNode)
        
        # delete nodes not around anymore
        for childNode in nodeMap.itervalues():
            childNode.parentNode.removeChild(childNode)
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
        for child in list(node.childNodes):
            if child.nodeName == 'parameter':
                parameter = self.getDao('parameter').fromXML(child)
                parameters.append(parameter)
            elif child.nodeType == child.TEXT_NODE and child.nodeValue.strip() == '':
                # node.removeChild(child)
                pass
            elif child.nodeType != child.TEXT_NODE:
                print '*** ERROR *** nodeName = %s' % child.nodeName
        
        obj = DBFunction(id=id,
                         pos=pos,
                         name=name,
                         parameters=parameters)
        obj.is_dirty = False
        return obj
    
    def toXML(self, function, doc, node):
        if node is None:
            node = doc.createElement('function')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(function.db_id, 'long'))
        node.setAttribute('pos',self.convertToStr(function.db_pos, 'long'))
        node.setAttribute('name',self.convertToStr(function.db_name, 'str'))
        
        # load DOM node map
        nodeMap = {}
        for childNode in node.childNodes:
            if childNode.nodeType == childNode.ELEMENT_NODE and self.hasAttribute(childNode, 'id'):
                nodeId = self.convertFromStr(self.getAttribute(childNode, 'id'), 'long')
                nodeMap[(childNode.nodeName, nodeId)] = childNode
        
        # set elements
        parameters = function.db_parameters
        for parameter in parameters:
            if nodeMap.has_key(('parameter', parameter.db_id)):
                childNode = nodeMap[('parameter', parameter.db_id)]
                del nodeMap[('parameter', parameter.db_id)]
            else:
                childNode = doc.createElement('parameter')
                node.appendChild(childNode)
            self.getDao('parameter').toXML(parameter, doc, childNode)
        
        # delete nodes not around anymore
        for childNode in nodeMap.itervalues():
            childNode.parentNode.removeChild(childNode)
        return node

class DBAbstractionXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'abstraction':
            return None
        
        actions = {}
        tags = {}
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        name = self.convertFromStr(self.getAttribute(node, 'name'), 'str')
        
        
        # read children
        for child in list(node.childNodes):
            if child.nodeName == 'action':
                action = self.getDao('action').fromXML(child)
                actions[action.db_id] = action
            elif child.nodeName == 'tag':
                tag = self.getDao('tag').fromXML(child)
                tags[tag.db_id] = tag
            elif child.nodeType == child.TEXT_NODE and child.nodeValue.strip() == '':
                # node.removeChild(child)
                pass
            elif child.nodeType != child.TEXT_NODE:
                print '*** ERROR *** nodeName = %s' % child.nodeName
        
        obj = DBAbstraction(id=id,
                            name=name,
                            actions=actions,
                            tags=tags)
        obj.is_dirty = False
        return obj
    
    def toXML(self, abstraction, doc, node):
        if node is None:
            node = doc.createElement('abstraction')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(abstraction.db_id, 'long'))
        node.setAttribute('name',self.convertToStr(abstraction.db_name, 'str'))
        
        # load DOM node map
        nodeMap = {}
        for childNode in node.childNodes:
            if childNode.nodeType == childNode.ELEMENT_NODE and self.hasAttribute(childNode, 'id'):
                nodeId = self.convertFromStr(self.getAttribute(childNode, 'id'), 'long')
                nodeMap[(childNode.nodeName, nodeId)] = childNode
        
        # set elements
        actions = abstraction.db_actions
        for action in actions.itervalues():
            if nodeMap.has_key(('action', action.db_id)):
                childNode = nodeMap[('action', action.db_id)]
                del nodeMap[('action', action.db_id)]
            else:
                childNode = doc.createElement('action')
                node.appendChild(childNode)
            self.getDao('action').toXML(action, doc, childNode)
        tags = abstraction.db_tags
        for tag in tags.itervalues():
            if nodeMap.has_key(('tag', tag.db_id)):
                childNode = nodeMap[('tag', tag.db_id)]
                del nodeMap[('tag', tag.db_id)]
            else:
                childNode = doc.createElement('tag')
                node.appendChild(childNode)
            self.getDao('tag').toXML(tag, doc, childNode)
        
        # delete nodes not around anymore
        for childNode in nodeMap.itervalues():
            childNode.parentNode.removeChild(childNode)
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
        abstractionRefs = {}
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        name = self.convertFromStr(self.getAttribute(node, 'name'), 'str')
        
        
        # read children
        for child in list(node.childNodes):
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
            elif child.nodeName == 'abstractionRef':
                abstractionRef = self.getDao('abstractionRef').fromXML(child)
                abstractionRefs[abstractionRef.db_id] = abstractionRef
            elif child.nodeType == child.TEXT_NODE and child.nodeValue.strip() == '':
                # node.removeChild(child)
                pass
            elif child.nodeType != child.TEXT_NODE:
                print '*** ERROR *** nodeName = %s' % child.nodeName
        
        obj = DBWorkflow(id=id,
                         name=name,
                         modules=modules,
                         connections=connections,
                         annotations=annotations,
                         others=others,
                         abstractionRefs=abstractionRefs)
        obj.is_dirty = False
        return obj
    
    def toXML(self, workflow, doc, node):
        if node is None:
            node = doc.createElement('workflow')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(workflow.db_id, 'long'))
        node.setAttribute('name',self.convertToStr(workflow.db_name, 'str'))
        
        # load DOM node map
        nodeMap = {}
        for childNode in node.childNodes:
            if childNode.nodeType == childNode.ELEMENT_NODE and self.hasAttribute(childNode, 'id'):
                nodeId = self.convertFromStr(self.getAttribute(childNode, 'id'), 'long')
                nodeMap[(childNode.nodeName, nodeId)] = childNode
        
        # set elements
        modules = workflow.db_modules
        for module in modules.itervalues():
            if nodeMap.has_key(('module', module.db_id)):
                childNode = nodeMap[('module', module.db_id)]
                del nodeMap[('module', module.db_id)]
            else:
                childNode = doc.createElement('module')
                node.appendChild(childNode)
            self.getDao('module').toXML(module, doc, childNode)
        connections = workflow.db_connections
        for connection in connections.itervalues():
            if nodeMap.has_key(('connection', connection.db_id)):
                childNode = nodeMap[('connection', connection.db_id)]
                del nodeMap[('connection', connection.db_id)]
            else:
                childNode = doc.createElement('connection')
                node.appendChild(childNode)
            self.getDao('connection').toXML(connection, doc, childNode)
        annotations = workflow.db_annotations
        for annotation in annotations:
            if nodeMap.has_key(('annotation', annotation.db_id)):
                childNode = nodeMap[('annotation', annotation.db_id)]
                del nodeMap[('annotation', annotation.db_id)]
            else:
                childNode = doc.createElement('annotation')
                node.appendChild(childNode)
            self.getDao('annotation').toXML(annotation, doc, childNode)
        others = workflow.db_others
        for other in others:
            if nodeMap.has_key(('other', other.db_id)):
                childNode = nodeMap[('other', other.db_id)]
                del nodeMap[('other', other.db_id)]
            else:
                childNode = doc.createElement('other')
                node.appendChild(childNode)
            self.getDao('other').toXML(other, doc, childNode)
        abstractionRefs = workflow.db_abstractionRefs
        for abstractionRef in abstractionRefs.itervalues():
            if nodeMap.has_key(('abstractionRef', abstractionRef.db_id)):
                childNode = nodeMap[('abstractionRef', abstractionRef.db_id)]
                del nodeMap[('abstractionRef', abstractionRef.db_id)]
            else:
                childNode = doc.createElement('abstractionRef')
                node.appendChild(childNode)
            self.getDao('abstractionRef').toXML(abstractionRef, doc, childNode)
        
        # delete nodes not around anymore
        for childNode in nodeMap.itervalues():
            childNode.parentNode.removeChild(childNode)
        return node

class DBAbstractionRefXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'abstractionRef':
            return None
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        abstraction_id = self.convertFromStr(self.getAttribute(node, 'abstractionId'), 'long')
        version = self.convertFromStr(self.getAttribute(node, 'version'), 'long')
        
        obj = DBAbstractionRef(id=id,
                               abstraction_id=abstraction_id,
                               version=version)
        obj.is_dirty = False
        return obj
    
    def toXML(self, abstractionRef, doc, node):
        if node is None:
            node = doc.createElement('abstractionRef')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(abstractionRef.db_id, 'long'))
        node.setAttribute('abstractionId',self.convertToStr(abstractionRef.db_abstraction_id, 'long'))
        node.setAttribute('version',self.convertToStr(abstractionRef.db_version, 'long'))
        
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
    
    def toXML(self, annotation, doc, node):
        if node is None:
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
        for child in list(node.childNodes):
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
            elif child.nodeName == 'abstractionRef':
                data = self.getDao('abstractionRef').fromXML(child)
            elif child.nodeName == 'other':
                data = self.getDao('other').fromXML(child)
            elif child.nodeType == child.TEXT_NODE and child.nodeValue.strip() == '':
                # node.removeChild(child)
                pass
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
    
    def toXML(self, change, doc, node):
        if node is None:
            node = doc.createElement('change')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(change.db_id, 'long'))
        node.setAttribute('what',self.convertToStr(change.db_what, 'str'))
        node.setAttribute('oldObjId',self.convertToStr(change.db_oldObjId, 'long'))
        node.setAttribute('newObjId',self.convertToStr(change.db_newObjId, 'long'))
        node.setAttribute('parentObjId',self.convertToStr(change.db_parentObjId, 'long'))
        node.setAttribute('parentObjType',self.convertToStr(change.db_parentObjType, 'str'))
        
        # load DOM node map
        nodeMap = {}
        for childNode in node.childNodes:
            if childNode.nodeType == childNode.ELEMENT_NODE and self.hasAttribute(childNode, 'id'):
                nodeId = self.convertFromStr(self.getAttribute(childNode, 'id'), 'long')
                nodeMap[(childNode.nodeName, nodeId)] = childNode
        
        # set elements
        data = change.db_data
        if data.vtType == 'module':
            if nodeMap.has_key(('module', data.db_id)):
                childNode = nodeMap[('module', data.db_id)]
                del nodeMap[('module', data.db_id)]
            else:
                childNode = doc.createElement('module')
                node.appendChild(childNode)
            self.getDao('module').toXML(data, doc, childNode)
        elif data.vtType == 'location':
            if nodeMap.has_key(('location', data.db_id)):
                childNode = nodeMap[('location', data.db_id)]
                del nodeMap[('location', data.db_id)]
            else:
                childNode = doc.createElement('location')
                node.appendChild(childNode)
            self.getDao('location').toXML(data, doc, childNode)
        elif data.vtType == 'annotation':
            if nodeMap.has_key(('annotation', data.db_id)):
                childNode = nodeMap[('annotation', data.db_id)]
                del nodeMap[('annotation', data.db_id)]
            else:
                childNode = doc.createElement('annotation')
                node.appendChild(childNode)
            self.getDao('annotation').toXML(data, doc, childNode)
        elif data.vtType == 'function':
            if nodeMap.has_key(('function', data.db_id)):
                childNode = nodeMap[('function', data.db_id)]
                del nodeMap[('function', data.db_id)]
            else:
                childNode = doc.createElement('function')
                node.appendChild(childNode)
            self.getDao('function').toXML(data, doc, childNode)
        elif data.vtType == 'connection':
            if nodeMap.has_key(('connection', data.db_id)):
                childNode = nodeMap[('connection', data.db_id)]
                del nodeMap[('connection', data.db_id)]
            else:
                childNode = doc.createElement('connection')
                node.appendChild(childNode)
            self.getDao('connection').toXML(data, doc, childNode)
        elif data.vtType == 'port':
            if nodeMap.has_key(('port', data.db_id)):
                childNode = nodeMap[('port', data.db_id)]
                del nodeMap[('port', data.db_id)]
            else:
                childNode = doc.createElement('port')
                node.appendChild(childNode)
            self.getDao('port').toXML(data, doc, childNode)
        elif data.vtType == 'parameter':
            if nodeMap.has_key(('parameter', data.db_id)):
                childNode = nodeMap[('parameter', data.db_id)]
                del nodeMap[('parameter', data.db_id)]
            else:
                childNode = doc.createElement('parameter')
                node.appendChild(childNode)
            self.getDao('parameter').toXML(data, doc, childNode)
        elif data.vtType == 'portSpec':
            if nodeMap.has_key(('portSpec', data.db_id)):
                childNode = nodeMap[('portSpec', data.db_id)]
                del nodeMap[('portSpec', data.db_id)]
            else:
                childNode = doc.createElement('portSpec')
                node.appendChild(childNode)
            self.getDao('portSpec').toXML(data, doc, childNode)
        elif data.vtType == 'abstractionRef':
            if nodeMap.has_key(('abstractionRef', data.db_id)):
                childNode = nodeMap[('abstractionRef', data.db_id)]
                del nodeMap[('abstractionRef', data.db_id)]
            else:
                childNode = doc.createElement('abstractionRef')
                node.appendChild(childNode)
            self.getDao('abstractionRef').toXML(data, doc, childNode)
        elif data.vtType == 'other':
            if nodeMap.has_key(('other', data.db_id)):
                childNode = nodeMap[('other', data.db_id)]
                del nodeMap[('other', data.db_id)]
            else:
                childNode = doc.createElement('other')
                node.appendChild(childNode)
            self.getDao('other').toXML(data, doc, childNode)
        
        # delete nodes not around anymore
        for childNode in nodeMap.itervalues():
            childNode.parentNode.removeChild(childNode)
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
    
    def toXML(self, parameter, doc, node):
        if node is None:
            node = doc.createElement('parameter')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(parameter.db_id, 'long'))
        node.setAttribute('pos',self.convertToStr(parameter.db_pos, 'long'))
        node.setAttribute('name',self.convertToStr(parameter.db_name, 'str'))
        node.setAttribute('type',self.convertToStr(parameter.db_type, 'str'))
        node.setAttribute('val',self.convertToStr(parameter.db_val, 'str'))
        node.setAttribute('alias',self.convertToStr(parameter.db_alias, 'str'))
        
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
        for child in list(node.childNodes):
            if child.nodeName == 'port':
                port = self.getDao('port').fromXML(child)
                ports.append(port)
            elif child.nodeType == child.TEXT_NODE and child.nodeValue.strip() == '':
                # node.removeChild(child)
                pass
            elif child.nodeType != child.TEXT_NODE:
                print '*** ERROR *** nodeName = %s' % child.nodeName
        
        obj = DBConnection(id=id,
                           ports=ports)
        obj.is_dirty = False
        return obj
    
    def toXML(self, connection, doc, node):
        if node is None:
            node = doc.createElement('connection')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(connection.db_id, 'long'))
        
        # load DOM node map
        nodeMap = {}
        for childNode in node.childNodes:
            if childNode.nodeType == childNode.ELEMENT_NODE and self.hasAttribute(childNode, 'id'):
                nodeId = self.convertFromStr(self.getAttribute(childNode, 'id'), 'long')
                nodeMap[(childNode.nodeName, nodeId)] = childNode
        
        # set elements
        ports = connection.db_ports
        for port in ports:
            if nodeMap.has_key(('port', port.db_id)):
                childNode = nodeMap[('port', port.db_id)]
                del nodeMap[('port', port.db_id)]
            else:
                childNode = doc.createElement('port')
                node.appendChild(childNode)
            self.getDao('port').toXML(port, doc, childNode)
        
        # delete nodes not around anymore
        for childNode in nodeMap.itervalues():
            childNode.parentNode.removeChild(childNode)
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
        
        annotations = {}
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        prevId = self.convertFromStr(self.getAttribute(node, 'prevId'), 'long')
        date = self.convertFromStr(self.getAttribute(node, 'date'), 'datetime')
        user = self.convertFromStr(self.getAttribute(node, 'user'), 'str')
        
        
        # read children
        for child in list(node.childNodes):
            if child.nodeName == 'annotation':
                annotation = self.getDao('annotation').fromXML(child)
                annotations[annotation.db_id] = annotation
            elif child.nodeName == 'add':
                operation = self.getDao('add').fromXML(child)
                operations.append(operation)
            elif child.nodeName == 'delete':
                operation = self.getDao('delete').fromXML(child)
                operations.append(operation)
            elif child.nodeName == 'change':
                operation = self.getDao('change').fromXML(child)
                operations.append(operation)
            elif child.nodeType == child.TEXT_NODE and child.nodeValue.strip() == '':
                # node.removeChild(child)
                pass
            elif child.nodeType != child.TEXT_NODE:
                print '*** ERROR *** nodeName = %s' % child.nodeName
        
        obj = DBAction(id=id,
                       prevId=prevId,
                       date=date,
                       user=user,
                       annotations=annotations,
                       operations=operations)
        obj.is_dirty = False
        return obj
    
    def toXML(self, action, doc, node):
        if node is None:
            node = doc.createElement('action')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(action.db_id, 'long'))
        node.setAttribute('prevId',self.convertToStr(action.db_prevId, 'long'))
        node.setAttribute('date',self.convertToStr(action.db_date, 'datetime'))
        node.setAttribute('user',self.convertToStr(action.db_user, 'str'))
        
        # load DOM node map
        nodeMap = {}
        for childNode in node.childNodes:
            if childNode.nodeType == childNode.ELEMENT_NODE and self.hasAttribute(childNode, 'id'):
                nodeId = self.convertFromStr(self.getAttribute(childNode, 'id'), 'long')
                nodeMap[(childNode.nodeName, nodeId)] = childNode
        
        # set elements
        annotations = action.db_annotations
        for annotation in annotations.itervalues():
            if nodeMap.has_key(('annotation', annotation.db_id)):
                childNode = nodeMap[('annotation', annotation.db_id)]
                del nodeMap[('annotation', annotation.db_id)]
            else:
                childNode = doc.createElement('annotation')
                node.appendChild(childNode)
            self.getDao('annotation').toXML(annotation, doc, childNode)
        operations = action.db_operations
        for operation in operations:
            if operation.vtType == 'add':
                if nodeMap.has_key(('add', operation.db_id)):
                    childNode = nodeMap[('add', operation.db_id)]
                    del nodeMap[('add', operation.db_id)]
                else:
                    childNode = doc.createElement('add')
                    node.appendChild(childNode)
                self.getDao('add').toXML(operation, doc, childNode)
            elif operation.vtType == 'delete':
                if nodeMap.has_key(('delete', operation.db_id)):
                    childNode = nodeMap[('delete', operation.db_id)]
                    del nodeMap[('delete', operation.db_id)]
                else:
                    childNode = doc.createElement('delete')
                    node.appendChild(childNode)
                self.getDao('delete').toXML(operation, doc, childNode)
            elif operation.vtType == 'change':
                if nodeMap.has_key(('change', operation.db_id)):
                    childNode = nodeMap[('change', operation.db_id)]
                    del nodeMap[('change', operation.db_id)]
                else:
                    childNode = doc.createElement('change')
                    node.appendChild(childNode)
                self.getDao('change').toXML(operation, doc, childNode)
        
        # delete nodes not around anymore
        for childNode in nodeMap.itervalues():
            childNode.parentNode.removeChild(childNode)
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
    
    def toXML(self, delete, doc, node):
        if node is None:
            node = doc.createElement('delete')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(delete.db_id, 'long'))
        node.setAttribute('what',self.convertToStr(delete.db_what, 'str'))
        node.setAttribute('objectId',self.convertToStr(delete.db_objectId, 'long'))
        node.setAttribute('parentObjId',self.convertToStr(delete.db_parentObjId, 'long'))
        node.setAttribute('parentObjType',self.convertToStr(delete.db_parentObjType, 'str'))
        
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
        abstractions = {}
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        version = self.convertFromStr(self.getAttribute(node, 'version'), 'str')
        name = self.convertFromStr(self.getAttribute(node, 'name'), 'str')
        dbHost = self.convertFromStr(self.getAttribute(node, 'dbHost'), 'str')
        dbPort = self.convertFromStr(self.getAttribute(node, 'dbPort'), 'int')
        dbName = self.convertFromStr(self.getAttribute(node, 'dbName'), 'str')
        
        
        # read children
        for child in list(node.childNodes):
            if child.nodeName == 'action':
                action = self.getDao('action').fromXML(child)
                actions[action.db_id] = action
            elif child.nodeName == 'tag':
                tag = self.getDao('tag').fromXML(child)
                tags[tag.db_id] = tag
            elif child.nodeName == 'abstraction':
                abstraction = self.getDao('abstraction').fromXML(child)
                abstractions[abstraction.db_id] = abstraction
            elif child.nodeType == child.TEXT_NODE and child.nodeValue.strip() == '':
                # node.removeChild(child)
                pass
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
                         abstractions=abstractions)
        obj.is_dirty = False
        return obj
    
    def toXML(self, vistrail, doc, node):
        if node is None:
            node = doc.createElement('vistrail')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(vistrail.db_id, 'long'))
        node.setAttribute('version',self.convertToStr(vistrail.db_version, 'str'))
        node.setAttribute('name',self.convertToStr(vistrail.db_name, 'str'))
        node.setAttribute('dbHost',self.convertToStr(vistrail.db_dbHost, 'str'))
        node.setAttribute('dbPort',self.convertToStr(vistrail.db_dbPort, 'int'))
        node.setAttribute('dbName',self.convertToStr(vistrail.db_dbName, 'str'))
        
        # load DOM node map
        nodeMap = {}
        for childNode in node.childNodes:
            if childNode.nodeType == childNode.ELEMENT_NODE and self.hasAttribute(childNode, 'id'):
                nodeId = self.convertFromStr(self.getAttribute(childNode, 'id'), 'long')
                nodeMap[(childNode.nodeName, nodeId)] = childNode
        
        # set elements
        actions = vistrail.db_actions
        for action in actions.itervalues():
            if nodeMap.has_key(('action', action.db_id)):
                childNode = nodeMap[('action', action.db_id)]
                del nodeMap[('action', action.db_id)]
            else:
                childNode = doc.createElement('action')
                node.appendChild(childNode)
            self.getDao('action').toXML(action, doc, childNode)
        tags = vistrail.db_tags
        for tag in tags.itervalues():
            if nodeMap.has_key(('tag', tag.db_id)):
                childNode = nodeMap[('tag', tag.db_id)]
                del nodeMap[('tag', tag.db_id)]
            else:
                childNode = doc.createElement('tag')
                node.appendChild(childNode)
            self.getDao('tag').toXML(tag, doc, childNode)
        abstractions = vistrail.db_abstractions
        for abstraction in abstractions.itervalues():
            if nodeMap.has_key(('abstraction', abstraction.db_id)):
                childNode = nodeMap[('abstraction', abstraction.db_id)]
                del nodeMap[('abstraction', abstraction.db_id)]
            else:
                childNode = doc.createElement('abstraction')
                node.appendChild(childNode)
            self.getDao('abstraction').toXML(abstraction, doc, childNode)
        
        # delete nodes not around anymore
        for childNode in nodeMap.itervalues():
            childNode.parentNode.removeChild(childNode)
        return node

class DBModuleExecXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'moduleExec':
            return None
        
        annotations = []
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        ts_start = self.convertFromStr(self.getAttribute(node, 'tsStart'), 'datetime')
        ts_end = self.convertFromStr(self.getAttribute(node, 'tsEnd'), 'datetime')
        module_id = self.convertFromStr(self.getAttribute(node, 'moduleId'), 'long')
        module_name = self.convertFromStr(self.getAttribute(node, 'moduleName'), 'str')
        
        
        # read children
        for child in list(node.childNodes):
            if child.nodeName == 'annotation':
                annotation = self.getDao('annotation').fromXML(child)
                annotations.append(annotation)
            elif child.nodeType == child.TEXT_NODE and child.nodeValue.strip() == '':
                # node.removeChild(child)
                pass
            elif child.nodeType != child.TEXT_NODE:
                print '*** ERROR *** nodeName = %s' % child.nodeName
        
        obj = DBModuleExec(id=id,
                           ts_start=ts_start,
                           ts_end=ts_end,
                           module_id=module_id,
                           module_name=module_name,
                           annotations=annotations)
        obj.is_dirty = False
        return obj
    
    def toXML(self, module_exec, doc, node):
        if node is None:
            node = doc.createElement('moduleExec')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(module_exec.db_id, 'long'))
        node.setAttribute('tsStart',self.convertToStr(module_exec.db_ts_start, 'datetime'))
        node.setAttribute('tsEnd',self.convertToStr(module_exec.db_ts_end, 'datetime'))
        node.setAttribute('moduleId',self.convertToStr(module_exec.db_module_id, 'long'))
        node.setAttribute('moduleName',self.convertToStr(module_exec.db_module_name, 'str'))
        
        # load DOM node map
        nodeMap = {}
        for childNode in node.childNodes:
            if childNode.nodeType == childNode.ELEMENT_NODE and self.hasAttribute(childNode, 'id'):
                nodeId = self.convertFromStr(self.getAttribute(childNode, 'id'), 'long')
                nodeMap[(childNode.nodeName, nodeId)] = childNode
        
        # set elements
        annotations = module_exec.db_annotations
        for annotation in annotations:
            if nodeMap.has_key(('annotation', annotation.db_id)):
                childNode = nodeMap[('annotation', annotation.db_id)]
                del nodeMap[('annotation', annotation.db_id)]
            else:
                childNode = doc.createElement('annotation')
                node.appendChild(childNode)
            self.getDao('annotation').toXML(annotation, doc, childNode)
        
        # delete nodes not around anymore
        for childNode in nodeMap.itervalues():
            childNode.parentNode.removeChild(childNode)
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
        if 'tag' not in self:
            self['tag'] = DBTagXMLDAOBase(self)
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
        if 'workflow_exec' not in self:
            self['workflow_exec'] = DBWorkflowExecXMLDAOBase(self)
        if 'function' not in self:
            self['function'] = DBFunctionXMLDAOBase(self)
        if 'abstraction' not in self:
            self['abstraction'] = DBAbstractionXMLDAOBase(self)
        if 'workflow' not in self:
            self['workflow'] = DBWorkflowXMLDAOBase(self)
        if 'abstractionRef' not in self:
            self['abstractionRef'] = DBAbstractionRefXMLDAOBase(self)
        if 'annotation' not in self:
            self['annotation'] = DBAnnotationXMLDAOBase(self)
        if 'change' not in self:
            self['change'] = DBChangeXMLDAOBase(self)
        if 'parameter' not in self:
            self['parameter'] = DBParameterXMLDAOBase(self)
        if 'connection' not in self:
            self['connection'] = DBConnectionXMLDAOBase(self)
        if 'action' not in self:
            self['action'] = DBActionXMLDAOBase(self)
        if 'delete' not in self:
            self['delete'] = DBDeleteXMLDAOBase(self)
        if 'vistrail' not in self:
            self['vistrail'] = DBVistrailXMLDAOBase(self)
        if 'module_exec' not in self:
            self['module_exec'] = DBModuleExecXMLDAOBase(self)
