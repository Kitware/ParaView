
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

from core.system import get_elementtree_library
ElementTree = get_elementtree_library()

from xml_dao import XMLDAO
from db.versions.v0_9_1.domain import *

class DBPortSpecXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.tag != 'portSpec':
            return None
        
        # read attributes
        data = node.get('id', None)
        id = self.convertFromStr(data, 'long')
        data = node.get('name', None)
        name = self.convertFromStr(data, 'str')
        data = node.get('type', None)
        type = self.convertFromStr(data, 'str')
        data = node.get('spec', None)
        spec = self.convertFromStr(data, 'str')
        
        obj = DBPortSpec(id=id,
                         name=name,
                         type=type,
                         spec=spec)
        obj.is_dirty = False
        return obj
    
    def toXML(self, portSpec, node=None):
        if node is None:
            node = ElementTree.Element('portSpec')
        
        # set attributes
        node.set('id',self.convertToStr(portSpec.db_id, 'long'))
        node.set('name',self.convertToStr(portSpec.db_name, 'str'))
        node.set('type',self.convertToStr(portSpec.db_type, 'str'))
        node.set('spec',self.convertToStr(portSpec.db_spec, 'str'))
        
        return node

class DBModuleXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.tag != 'module':
            return None
        
        # read attributes
        data = node.get('id', None)
        id = self.convertFromStr(data, 'long')
        data = node.get('cache', None)
        cache = self.convertFromStr(data, 'int')
        data = node.get('name', None)
        name = self.convertFromStr(data, 'str')
        data = node.get('namespace', None)
        namespace = self.convertFromStr(data, 'str')
        data = node.get('package', None)
        package = self.convertFromStr(data, 'str')
        data = node.get('version', None)
        version = self.convertFromStr(data, 'str')
        data = node.get('tag', None)
        tag = self.convertFromStr(data, 'str')
        
        location = None
        functions = []
        annotations = []
        portSpecs = []
        
        # read children
        for child in node.getchildren():
            if child.tag == 'location':
                _data = self.getDao('location').fromXML(child)
                location = _data
            elif child.tag == 'function':
                _data = self.getDao('function').fromXML(child)
                functions.append(_data)
            elif child.tag == 'annotation':
                _data = self.getDao('annotation').fromXML(child)
                annotations.append(_data)
            elif child.tag == 'portSpec':
                _data = self.getDao('portSpec').fromXML(child)
                portSpecs.append(_data)
            elif child.text.strip() == '':
                pass
            else:
                print '*** ERROR *** tag = %s' % child.tag
        
        obj = DBModule(id=id,
                       cache=cache,
                       name=name,
                       namespace=namespace,
                       package=package,
                       version=version,
                       tag=tag,
                       location=location,
                       functions=functions,
                       annotations=annotations,
                       portSpecs=portSpecs)
        obj.is_dirty = False
        return obj
    
    def toXML(self, module, node=None):
        if node is None:
            node = ElementTree.Element('module')
        
        # set attributes
        node.set('id',self.convertToStr(module.db_id, 'long'))
        node.set('cache',self.convertToStr(module.db_cache, 'int'))
        node.set('name',self.convertToStr(module.db_name, 'str'))
        node.set('namespace',self.convertToStr(module.db_namespace, 'str'))
        node.set('package',self.convertToStr(module.db_package, 'str'))
        node.set('version',self.convertToStr(module.db_version, 'str'))
        node.set('tag',self.convertToStr(module.db_tag, 'str'))
        
        # set elements
        location = module.db_location
        if location is not None:
            childNode = ElementTree.SubElement(node, 'location')
            self.getDao('location').toXML(location, childNode)
        functions = module.db_functions
        for function in functions:
            childNode = ElementTree.SubElement(node, 'function')
            self.getDao('function').toXML(function, childNode)
        annotations = module.db_annotations
        for annotation in annotations:
            childNode = ElementTree.SubElement(node, 'annotation')
            self.getDao('annotation').toXML(annotation, childNode)
        portSpecs = module.db_portSpecs
        for portSpec in portSpecs:
            childNode = ElementTree.SubElement(node, 'portSpec')
            self.getDao('portSpec').toXML(portSpec, childNode)
        
        return node

class DBTagXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.tag != 'tag':
            return None
        
        # read attributes
        data = node.get('id', None)
        id = self.convertFromStr(data, 'long')
        data = node.get('name', None)
        name = self.convertFromStr(data, 'str')
        
        obj = DBTag(id=id,
                    name=name)
        obj.is_dirty = False
        return obj
    
    def toXML(self, tag, node=None):
        if node is None:
            node = ElementTree.Element('tag')
        
        # set attributes
        node.set('id',self.convertToStr(tag.db_id, 'long'))
        node.set('name',self.convertToStr(tag.db_name, 'str'))
        
        return node

class DBPortXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.tag != 'port':
            return None
        
        # read attributes
        data = node.get('id', None)
        id = self.convertFromStr(data, 'long')
        data = node.get('type', None)
        type = self.convertFromStr(data, 'str')
        data = node.get('moduleId', None)
        moduleId = self.convertFromStr(data, 'long')
        data = node.get('moduleName', None)
        moduleName = self.convertFromStr(data, 'str')
        data = node.get('name', None)
        name = self.convertFromStr(data, 'str')
        data = node.get('spec', None)
        spec = self.convertFromStr(data, 'str')
        
        obj = DBPort(id=id,
                     type=type,
                     moduleId=moduleId,
                     moduleName=moduleName,
                     name=name,
                     spec=spec)
        obj.is_dirty = False
        return obj
    
    def toXML(self, port, node=None):
        if node is None:
            node = ElementTree.Element('port')
        
        # set attributes
        node.set('id',self.convertToStr(port.db_id, 'long'))
        node.set('type',self.convertToStr(port.db_type, 'str'))
        node.set('moduleId',self.convertToStr(port.db_moduleId, 'long'))
        node.set('moduleName',self.convertToStr(port.db_moduleName, 'str'))
        node.set('name',self.convertToStr(port.db_name, 'str'))
        node.set('spec',self.convertToStr(port.db_spec, 'str'))
        
        return node

class DBGroupXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.tag != 'group':
            return None
        
        # read attributes
        data = node.get('id', None)
        id = self.convertFromStr(data, 'long')
        data = node.get('cache', None)
        cache = self.convertFromStr(data, 'int')
        data = node.get('name', None)
        name = self.convertFromStr(data, 'str')
        data = node.get('namespace', None)
        namespace = self.convertFromStr(data, 'str')
        data = node.get('package', None)
        package = self.convertFromStr(data, 'str')
        data = node.get('version', None)
        version = self.convertFromStr(data, 'str')
        data = node.get('tag', None)
        tag = self.convertFromStr(data, 'str')
        
        workflow = None
        location = None
        functions = []
        annotations = []
        
        # read children
        for child in node.getchildren():
            if child.tag == 'workflow':
                _data = self.getDao('workflow').fromXML(child)
                workflow = _data
            elif child.tag == 'location':
                _data = self.getDao('location').fromXML(child)
                location = _data
            elif child.tag == 'function':
                _data = self.getDao('function').fromXML(child)
                functions.append(_data)
            elif child.tag == 'annotation':
                _data = self.getDao('annotation').fromXML(child)
                annotations.append(_data)
            elif child.text.strip() == '':
                pass
            else:
                print '*** ERROR *** tag = %s' % child.tag
        
        obj = DBGroup(id=id,
                      workflow=workflow,
                      cache=cache,
                      name=name,
                      namespace=namespace,
                      package=package,
                      version=version,
                      tag=tag,
                      location=location,
                      functions=functions,
                      annotations=annotations)
        obj.is_dirty = False
        return obj
    
    def toXML(self, group, node=None):
        if node is None:
            node = ElementTree.Element('group')
        
        # set attributes
        node.set('id',self.convertToStr(group.db_id, 'long'))
        node.set('cache',self.convertToStr(group.db_cache, 'int'))
        node.set('name',self.convertToStr(group.db_name, 'str'))
        node.set('namespace',self.convertToStr(group.db_namespace, 'str'))
        node.set('package',self.convertToStr(group.db_package, 'str'))
        node.set('version',self.convertToStr(group.db_version, 'str'))
        node.set('tag',self.convertToStr(group.db_tag, 'str'))
        
        # set elements
        workflow = group.db_workflow
        if workflow is not None:
            childNode = ElementTree.SubElement(node, 'workflow')
            self.getDao('workflow').toXML(workflow, childNode)
        location = group.db_location
        if location is not None:
            childNode = ElementTree.SubElement(node, 'location')
            self.getDao('location').toXML(location, childNode)
        functions = group.db_functions
        for function in functions:
            childNode = ElementTree.SubElement(node, 'function')
            self.getDao('function').toXML(function, childNode)
        annotations = group.db_annotations
        for annotation in annotations:
            childNode = ElementTree.SubElement(node, 'annotation')
            self.getDao('annotation').toXML(annotation, childNode)
        
        return node

class DBLogXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.tag != 'log':
            return None
        
        # read attributes
        data = node.get('id', None)
        id = self.convertFromStr(data, 'long')
        data = node.get('version', None)
        version = self.convertFromStr(data, 'str')
        data = node.get('name', None)
        name = self.convertFromStr(data, 'str')
        data = node.get('vistrail_id', None)
        vistrail_id = self.convertFromStr(data, 'long')
        
        workflow_execs = []
        machines = []
        
        # read children
        for child in node.getchildren():
            if child.tag == 'workflowExec':
                _data = self.getDao('workflow_exec').fromXML(child)
                workflow_execs.append(_data)
            elif child.tag == 'machine':
                _data = self.getDao('machine').fromXML(child)
                machines.append(_data)
            elif child.text.strip() == '':
                pass
            else:
                print '*** ERROR *** tag = %s' % child.tag
        
        obj = DBLog(id=id,
                    version=version,
                    name=name,
                    workflow_execs=workflow_execs,
                    machines=machines,
                    vistrail_id=vistrail_id)
        obj.is_dirty = False
        return obj
    
    def toXML(self, log, node=None):
        if node is None:
            node = ElementTree.Element('log')
        
        # set attributes
        node.set('id',self.convertToStr(log.db_id, 'long'))
        node.set('version',self.convertToStr(log.db_version, 'str'))
        node.set('name',self.convertToStr(log.db_name, 'str'))
        node.set('vistrail_id',self.convertToStr(log.db_vistrail_id, 'long'))
        
        # set elements
        workflow_execs = log.db_workflow_execs
        for workflow_exec in workflow_execs:
            childNode = ElementTree.SubElement(node, 'workflow_exec')
            self.getDao('workflow_exec').toXML(workflow_exec, childNode)
        machines = log.db_machines
        for machine in machines:
            childNode = ElementTree.SubElement(node, 'machine')
            self.getDao('machine').toXML(machine, childNode)
        
        return node

class DBMachineXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.tag != 'machine':
            return None
        
        # read attributes
        data = node.get('id', None)
        id = self.convertFromStr(data, 'long')
        data = node.get('name', None)
        name = self.convertFromStr(data, 'str')
        data = node.get('os', None)
        os = self.convertFromStr(data, 'str')
        data = node.get('architecture', None)
        architecture = self.convertFromStr(data, 'str')
        data = node.get('processor', None)
        processor = self.convertFromStr(data, 'str')
        data = node.get('ram', None)
        ram = self.convertFromStr(data, 'int')
        
        obj = DBMachine(id=id,
                        name=name,
                        os=os,
                        architecture=architecture,
                        processor=processor,
                        ram=ram)
        obj.is_dirty = False
        return obj
    
    def toXML(self, machine, node=None):
        if node is None:
            node = ElementTree.Element('machine')
        
        # set attributes
        node.set('id',self.convertToStr(machine.db_id, 'long'))
        node.set('name',self.convertToStr(machine.db_name, 'str'))
        node.set('os',self.convertToStr(machine.db_os, 'str'))
        node.set('architecture',self.convertToStr(machine.db_architecture, 'str'))
        node.set('processor',self.convertToStr(machine.db_processor, 'str'))
        node.set('ram',self.convertToStr(machine.db_ram, 'int'))
        
        return node

class DBAddXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.tag != 'add':
            return None
        
        # read attributes
        data = node.get('id', None)
        id = self.convertFromStr(data, 'long')
        data = node.get('what', None)
        what = self.convertFromStr(data, 'str')
        data = node.get('objectId', None)
        objectId = self.convertFromStr(data, 'long')
        data = node.get('parentObjId', None)
        parentObjId = self.convertFromStr(data, 'long')
        data = node.get('parentObjType', None)
        parentObjType = self.convertFromStr(data, 'str')
        
        data = None
        
        # read children
        for child in node.getchildren():
            if child.tag == 'module':
                _data = self.getDao('module').fromXML(child)
                data = _data
            elif child.tag == 'location':
                _data = self.getDao('location').fromXML(child)
                data = _data
            elif child.tag == 'annotation':
                _data = self.getDao('annotation').fromXML(child)
                data = _data
            elif child.tag == 'function':
                _data = self.getDao('function').fromXML(child)
                data = _data
            elif child.tag == 'connection':
                _data = self.getDao('connection').fromXML(child)
                data = _data
            elif child.tag == 'port':
                _data = self.getDao('port').fromXML(child)
                data = _data
            elif child.tag == 'parameter':
                _data = self.getDao('parameter').fromXML(child)
                data = _data
            elif child.tag == 'portSpec':
                _data = self.getDao('portSpec').fromXML(child)
                data = _data
            elif child.tag == 'abstractionRef':
                _data = self.getDao('abstractionRef').fromXML(child)
                data = _data
            elif child.tag == 'group':
                _data = self.getDao('group').fromXML(child)
                data = _data
            elif child.tag == 'other':
                _data = self.getDao('other').fromXML(child)
                data = _data
            elif child.text.strip() == '':
                pass
            else:
                print '*** ERROR *** tag = %s' % child.tag
        
        obj = DBAdd(data=data,
                    id=id,
                    what=what,
                    objectId=objectId,
                    parentObjId=parentObjId,
                    parentObjType=parentObjType)
        obj.is_dirty = False
        return obj
    
    def toXML(self, add, node=None):
        if node is None:
            node = ElementTree.Element('add')
        
        # set attributes
        node.set('id',self.convertToStr(add.db_id, 'long'))
        node.set('what',self.convertToStr(add.db_what, 'str'))
        node.set('objectId',self.convertToStr(add.db_objectId, 'long'))
        node.set('parentObjId',self.convertToStr(add.db_parentObjId, 'long'))
        node.set('parentObjType',self.convertToStr(add.db_parentObjType, 'str'))
        
        # set elements
        data = add.db_data
        if data is not None:
            if data.vtType == 'module':
                childNode = ElementTree.SubElement(node, 'module')
                self.getDao('module').toXML(data, childNode)
            elif data.vtType == 'location':
                childNode = ElementTree.SubElement(node, 'location')
                self.getDao('location').toXML(data, childNode)
            elif data.vtType == 'annotation':
                childNode = ElementTree.SubElement(node, 'annotation')
                self.getDao('annotation').toXML(data, childNode)
            elif data.vtType == 'function':
                childNode = ElementTree.SubElement(node, 'function')
                self.getDao('function').toXML(data, childNode)
            elif data.vtType == 'connection':
                childNode = ElementTree.SubElement(node, 'connection')
                self.getDao('connection').toXML(data, childNode)
            elif data.vtType == 'port':
                childNode = ElementTree.SubElement(node, 'port')
                self.getDao('port').toXML(data, childNode)
            elif data.vtType == 'parameter':
                childNode = ElementTree.SubElement(node, 'parameter')
                self.getDao('parameter').toXML(data, childNode)
            elif data.vtType == 'portSpec':
                childNode = ElementTree.SubElement(node, 'portSpec')
                self.getDao('portSpec').toXML(data, childNode)
            elif data.vtType == 'abstractionRef':
                childNode = ElementTree.SubElement(node, 'abstractionRef')
                self.getDao('abstractionRef').toXML(data, childNode)
            elif data.vtType == 'group':
                childNode = ElementTree.SubElement(node, 'group')
                self.getDao('group').toXML(data, childNode)
            elif data.vtType == 'other':
                childNode = ElementTree.SubElement(node, 'other')
                self.getDao('other').toXML(data, childNode)
        
        return node

class DBOtherXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.tag != 'other':
            return None
        
        # read attributes
        data = node.get('id', None)
        id = self.convertFromStr(data, 'long')
        data = node.get('key', None)
        key = self.convertFromStr(data, 'str')
        
        value = None
        
        # read children
        for child in node.getchildren():
            if child.tag == 'value':
                _data = self.convertFromStr(child.text,'')
                value = _data
            elif child.text.strip() == '':
                pass
            else:
                print '*** ERROR *** tag = %s' % child.tag
        
        obj = DBOther(id=id,
                      key=key,
                      value=value)
        obj.is_dirty = False
        return obj
    
    def toXML(self, other, node=None):
        if node is None:
            node = ElementTree.Element('other')
        
        # set attributes
        node.set('id',self.convertToStr(other.db_id, 'long'))
        node.set('key',self.convertToStr(other.db_key, 'str'))
        
        # set elements
        
        return node

class DBLocationXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.tag != 'location':
            return None
        
        # read attributes
        data = node.get('id', None)
        id = self.convertFromStr(data, 'long')
        data = node.get('x', None)
        x = self.convertFromStr(data, 'float')
        data = node.get('y', None)
        y = self.convertFromStr(data, 'float')
        
        obj = DBLocation(id=id,
                         x=x,
                         y=y)
        obj.is_dirty = False
        return obj
    
    def toXML(self, location, node=None):
        if node is None:
            node = ElementTree.Element('location')
        
        # set attributes
        node.set('id',self.convertToStr(location.db_id, 'long'))
        node.set('x',self.convertToStr(location.db_x, 'float'))
        node.set('y',self.convertToStr(location.db_y, 'float'))
        
        return node

class DBParameterXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.tag != 'parameter':
            return None
        
        # read attributes
        data = node.get('id', None)
        id = self.convertFromStr(data, 'long')
        data = node.get('pos', None)
        pos = self.convertFromStr(data, 'long')
        data = node.get('name', None)
        name = self.convertFromStr(data, 'str')
        data = node.get('type', None)
        type = self.convertFromStr(data, 'str')
        data = node.get('val', None)
        val = self.convertFromStr(data, 'str')
        data = node.get('alias', None)
        alias = self.convertFromStr(data, 'str')
        
        obj = DBParameter(id=id,
                          pos=pos,
                          name=name,
                          type=type,
                          val=val,
                          alias=alias)
        obj.is_dirty = False
        return obj
    
    def toXML(self, parameter, node=None):
        if node is None:
            node = ElementTree.Element('parameter')
        
        # set attributes
        node.set('id',self.convertToStr(parameter.db_id, 'long'))
        node.set('pos',self.convertToStr(parameter.db_pos, 'long'))
        node.set('name',self.convertToStr(parameter.db_name, 'str'))
        node.set('type',self.convertToStr(parameter.db_type, 'str'))
        node.set('val',self.convertToStr(parameter.db_val, 'str'))
        node.set('alias',self.convertToStr(parameter.db_alias, 'str'))
        
        return node

class DBFunctionXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.tag != 'function':
            return None
        
        # read attributes
        data = node.get('id', None)
        id = self.convertFromStr(data, 'long')
        data = node.get('pos', None)
        pos = self.convertFromStr(data, 'long')
        data = node.get('name', None)
        name = self.convertFromStr(data, 'str')
        
        parameters = []
        
        # read children
        for child in node.getchildren():
            if child.tag == 'parameter':
                _data = self.getDao('parameter').fromXML(child)
                parameters.append(_data)
            elif child.text.strip() == '':
                pass
            else:
                print '*** ERROR *** tag = %s' % child.tag
        
        obj = DBFunction(id=id,
                         pos=pos,
                         name=name,
                         parameters=parameters)
        obj.is_dirty = False
        return obj
    
    def toXML(self, function, node=None):
        if node is None:
            node = ElementTree.Element('function')
        
        # set attributes
        node.set('id',self.convertToStr(function.db_id, 'long'))
        node.set('pos',self.convertToStr(function.db_pos, 'long'))
        node.set('name',self.convertToStr(function.db_name, 'str'))
        
        # set elements
        parameters = function.db_parameters
        for parameter in parameters:
            childNode = ElementTree.SubElement(node, 'parameter')
            self.getDao('parameter').toXML(parameter, childNode)
        
        return node

class DBAbstractionXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.tag != 'abstraction':
            return None
        
        # read attributes
        data = node.get('id', None)
        id = self.convertFromStr(data, 'long')
        data = node.get('name', None)
        name = self.convertFromStr(data, 'str')
        
        actions = []
        tags = []
        
        # read children
        for child in node.getchildren():
            if child.tag == 'action':
                _data = self.getDao('action').fromXML(child)
                actions.append(_data)
            elif child.tag == 'tag':
                _data = self.getDao('tag').fromXML(child)
                tags.append(_data)
            elif child.text.strip() == '':
                pass
            else:
                print '*** ERROR *** tag = %s' % child.tag
        
        obj = DBAbstraction(id=id,
                            name=name,
                            actions=actions,
                            tags=tags)
        obj.is_dirty = False
        return obj
    
    def toXML(self, abstraction, node=None):
        if node is None:
            node = ElementTree.Element('abstraction')
        
        # set attributes
        node.set('id',self.convertToStr(abstraction.db_id, 'long'))
        node.set('name',self.convertToStr(abstraction.db_name, 'str'))
        
        # set elements
        actions = abstraction.db_actions
        for action in actions:
            childNode = ElementTree.SubElement(node, 'action')
            self.getDao('action').toXML(action, childNode)
        tags = abstraction.db_tags
        for tag in tags:
            childNode = ElementTree.SubElement(node, 'tag')
            self.getDao('tag').toXML(tag, childNode)
        
        return node

class DBWorkflowXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.tag != 'workflow':
            return None
        
        # read attributes
        data = node.get('id', None)
        id = self.convertFromStr(data, 'long')
        data = node.get('name', None)
        name = self.convertFromStr(data, 'str')
        data = node.get('version', None)
        version = self.convertFromStr(data, 'str')
        data = node.get('vistrail_id', None)
        vistrail_id = self.convertFromStr(data, 'long')
        
        connections = []
        annotations = []
        abstractions = []
        others = []
        modules = []
        
        # read children
        for child in node.getchildren():
            if child.tag == 'connection':
                _data = self.getDao('connection').fromXML(child)
                connections.append(_data)
            elif child.tag == 'annotation':
                _data = self.getDao('annotation').fromXML(child)
                annotations.append(_data)
            elif child.tag == 'abstraction':
                _data = self.getDao('abstraction').fromXML(child)
                abstractions.append(_data)
            elif child.tag == 'other':
                _data = self.getDao('other').fromXML(child)
                others.append(_data)
            elif child.tag == 'module':
                _data = self.getDao('module').fromXML(child)
                modules.append(_data)
            elif child.tag == 'abstractionRef':
                _data = self.getDao('abstractionRef').fromXML(child)
                modules.append(_data)
            elif child.tag == 'group':
                _data = self.getDao('group').fromXML(child)
                modules.append(_data)
            elif child.text.strip() == '':
                pass
            else:
                print '*** ERROR *** tag = %s' % child.tag
        
        obj = DBWorkflow(modules=modules,
                         id=id,
                         name=name,
                         version=version,
                         connections=connections,
                         annotations=annotations,
                         abstractions=abstractions,
                         others=others,
                         vistrail_id=vistrail_id)
        obj.is_dirty = False
        return obj
    
    def toXML(self, workflow, node=None):
        if node is None:
            node = ElementTree.Element('workflow')
        
        # set attributes
        node.set('id',self.convertToStr(workflow.db_id, 'long'))
        node.set('name',self.convertToStr(workflow.db_name, 'str'))
        node.set('version',self.convertToStr(workflow.db_version, 'str'))
        node.set('vistrail_id',self.convertToStr(workflow.db_vistrail_id, 'long'))
        
        # set elements
        connections = workflow.db_connections
        for connection in connections:
            childNode = ElementTree.SubElement(node, 'connection')
            self.getDao('connection').toXML(connection, childNode)
        annotations = workflow.db_annotations
        for annotation in annotations:
            childNode = ElementTree.SubElement(node, 'annotation')
            self.getDao('annotation').toXML(annotation, childNode)
        abstractions = workflow.db_abstractions
        for abstraction in abstractions:
            childNode = ElementTree.SubElement(node, 'abstraction')
            self.getDao('abstraction').toXML(abstraction, childNode)
        others = workflow.db_others
        for other in others:
            childNode = ElementTree.SubElement(node, 'other')
            self.getDao('other').toXML(other, childNode)
        modules = workflow.db_modules
        for module in modules:
            if module.vtType == 'module':
                childNode = ElementTree.SubElement(node, 'module')
                self.getDao('module').toXML(module, childNode)
            elif module.vtType == 'abstractionRef':
                childNode = ElementTree.SubElement(node, 'abstractionRef')
                self.getDao('abstractionRef').toXML(module, childNode)
            elif module.vtType == 'group':
                childNode = ElementTree.SubElement(node, 'group')
                self.getDao('group').toXML(module, childNode)
        
        return node

class DBAbstractionRefXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.tag != 'abstractionRef':
            return None
        
        # read attributes
        data = node.get('id', None)
        id = self.convertFromStr(data, 'long')
        data = node.get('name', None)
        name = self.convertFromStr(data, 'str')
        data = node.get('cache', None)
        cache = self.convertFromStr(data, 'int')
        data = node.get('abstractionId', None)
        abstraction_id = self.convertFromStr(data, 'long')
        data = node.get('version', None)
        version = self.convertFromStr(data, 'long')
        
        location = None
        functions = []
        annotations = []
        
        # read children
        for child in node.getchildren():
            if child.tag == 'location':
                _data = self.getDao('location').fromXML(child)
                location = _data
            elif child.tag == 'function':
                _data = self.getDao('function').fromXML(child)
                functions.append(_data)
            elif child.tag == 'annotation':
                _data = self.getDao('annotation').fromXML(child)
                annotations.append(_data)
            elif child.text.strip() == '':
                pass
            else:
                print '*** ERROR *** tag = %s' % child.tag
        
        obj = DBAbstractionRef(id=id,
                               name=name,
                               cache=cache,
                               abstraction_id=abstraction_id,
                               version=version,
                               location=location,
                               functions=functions,
                               annotations=annotations)
        obj.is_dirty = False
        return obj
    
    def toXML(self, abstractionRef, node=None):
        if node is None:
            node = ElementTree.Element('abstractionRef')
        
        # set attributes
        node.set('id',self.convertToStr(abstractionRef.db_id, 'long'))
        node.set('name',self.convertToStr(abstractionRef.db_name, 'str'))
        node.set('cache',self.convertToStr(abstractionRef.db_cache, 'int'))
        node.set('abstractionId',self.convertToStr(abstractionRef.db_abstraction_id, 'long'))
        node.set('version',self.convertToStr(abstractionRef.db_version, 'long'))
        
        # set elements
        location = abstractionRef.db_location
        if location is not None:
            childNode = ElementTree.SubElement(node, 'location')
            self.getDao('location').toXML(location, childNode)
        functions = abstractionRef.db_functions
        for function in functions:
            childNode = ElementTree.SubElement(node, 'function')
            self.getDao('function').toXML(function, childNode)
        annotations = abstractionRef.db_annotations
        for annotation in annotations:
            childNode = ElementTree.SubElement(node, 'annotation')
            self.getDao('annotation').toXML(annotation, childNode)
        
        return node

class DBAnnotationXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.tag != 'annotation':
            return None
        
        # read attributes
        data = node.get('id', None)
        id = self.convertFromStr(data, 'long')
        data = node.get('key', None)
        key = self.convertFromStr(data, 'str')
        data = node.get('value', None)
        value = self.convertFromStr(data, 'str')
        
        obj = DBAnnotation(id=id,
                           key=key,
                           value=value)
        obj.is_dirty = False
        return obj
    
    def toXML(self, annotation, node=None):
        if node is None:
            node = ElementTree.Element('annotation')
        
        # set attributes
        node.set('id',self.convertToStr(annotation.db_id, 'long'))
        node.set('key',self.convertToStr(annotation.db_key, 'str'))
        node.set('value',self.convertToStr(annotation.db_value, 'str'))
        
        return node

class DBChangeXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.tag != 'change':
            return None
        
        # read attributes
        data = node.get('id', None)
        id = self.convertFromStr(data, 'long')
        data = node.get('what', None)
        what = self.convertFromStr(data, 'str')
        data = node.get('oldObjId', None)
        oldObjId = self.convertFromStr(data, 'long')
        data = node.get('newObjId', None)
        newObjId = self.convertFromStr(data, 'long')
        data = node.get('parentObjId', None)
        parentObjId = self.convertFromStr(data, 'long')
        data = node.get('parentObjType', None)
        parentObjType = self.convertFromStr(data, 'str')
        
        data = None
        
        # read children
        for child in node.getchildren():
            if child.tag == 'module':
                _data = self.getDao('module').fromXML(child)
                data = _data
            elif child.tag == 'location':
                _data = self.getDao('location').fromXML(child)
                data = _data
            elif child.tag == 'annotation':
                _data = self.getDao('annotation').fromXML(child)
                data = _data
            elif child.tag == 'function':
                _data = self.getDao('function').fromXML(child)
                data = _data
            elif child.tag == 'connection':
                _data = self.getDao('connection').fromXML(child)
                data = _data
            elif child.tag == 'port':
                _data = self.getDao('port').fromXML(child)
                data = _data
            elif child.tag == 'parameter':
                _data = self.getDao('parameter').fromXML(child)
                data = _data
            elif child.tag == 'portSpec':
                _data = self.getDao('portSpec').fromXML(child)
                data = _data
            elif child.tag == 'abstractionRef':
                _data = self.getDao('abstractionRef').fromXML(child)
                data = _data
            elif child.tag == 'group':
                _data = self.getDao('group').fromXML(child)
                data = _data
            elif child.tag == 'other':
                _data = self.getDao('other').fromXML(child)
                data = _data
            elif child.text.strip() == '':
                pass
            else:
                print '*** ERROR *** tag = %s' % child.tag
        
        obj = DBChange(data=data,
                       id=id,
                       what=what,
                       oldObjId=oldObjId,
                       newObjId=newObjId,
                       parentObjId=parentObjId,
                       parentObjType=parentObjType)
        obj.is_dirty = False
        return obj
    
    def toXML(self, change, node=None):
        if node is None:
            node = ElementTree.Element('change')
        
        # set attributes
        node.set('id',self.convertToStr(change.db_id, 'long'))
        node.set('what',self.convertToStr(change.db_what, 'str'))
        node.set('oldObjId',self.convertToStr(change.db_oldObjId, 'long'))
        node.set('newObjId',self.convertToStr(change.db_newObjId, 'long'))
        node.set('parentObjId',self.convertToStr(change.db_parentObjId, 'long'))
        node.set('parentObjType',self.convertToStr(change.db_parentObjType, 'str'))
        
        # set elements
        data = change.db_data
        if data is not None:
            if data.vtType == 'module':
                childNode = ElementTree.SubElement(node, 'module')
                self.getDao('module').toXML(data, childNode)
            elif data.vtType == 'location':
                childNode = ElementTree.SubElement(node, 'location')
                self.getDao('location').toXML(data, childNode)
            elif data.vtType == 'annotation':
                childNode = ElementTree.SubElement(node, 'annotation')
                self.getDao('annotation').toXML(data, childNode)
            elif data.vtType == 'function':
                childNode = ElementTree.SubElement(node, 'function')
                self.getDao('function').toXML(data, childNode)
            elif data.vtType == 'connection':
                childNode = ElementTree.SubElement(node, 'connection')
                self.getDao('connection').toXML(data, childNode)
            elif data.vtType == 'port':
                childNode = ElementTree.SubElement(node, 'port')
                self.getDao('port').toXML(data, childNode)
            elif data.vtType == 'parameter':
                childNode = ElementTree.SubElement(node, 'parameter')
                self.getDao('parameter').toXML(data, childNode)
            elif data.vtType == 'portSpec':
                childNode = ElementTree.SubElement(node, 'portSpec')
                self.getDao('portSpec').toXML(data, childNode)
            elif data.vtType == 'abstractionRef':
                childNode = ElementTree.SubElement(node, 'abstractionRef')
                self.getDao('abstractionRef').toXML(data, childNode)
            elif data.vtType == 'group':
                childNode = ElementTree.SubElement(node, 'group')
                self.getDao('group').toXML(data, childNode)
            elif data.vtType == 'other':
                childNode = ElementTree.SubElement(node, 'other')
                self.getDao('other').toXML(data, childNode)
        
        return node

class DBWorkflowExecXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.tag != 'workflowExec':
            return None
        
        # read attributes
        data = node.get('id', None)
        id = self.convertFromStr(data, 'long')
        data = node.get('user', None)
        user = self.convertFromStr(data, 'str')
        data = node.get('ip', None)
        ip = self.convertFromStr(data, 'str')
        data = node.get('vtVersion', None)
        vt_version = self.convertFromStr(data, 'str')
        data = node.get('tsStart', None)
        ts_start = self.convertFromStr(data, 'datetime')
        data = node.get('tsEnd', None)
        ts_end = self.convertFromStr(data, 'datetime')
        data = node.get('parentId', None)
        parent_id = self.convertFromStr(data, 'long')
        data = node.get('parentType', None)
        parent_type = self.convertFromStr(data, 'str')
        data = node.get('parentVersion', None)
        parent_version = self.convertFromStr(data, 'long')
        data = node.get('name', None)
        name = self.convertFromStr(data, 'str')
        
        module_execs = []
        
        # read children
        for child in node.getchildren():
            if child.tag == 'moduleExec':
                _data = self.getDao('module_exec').fromXML(child)
                module_execs.append(_data)
            elif child.text.strip() == '':
                pass
            else:
                print '*** ERROR *** tag = %s' % child.tag
        
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
    
    def toXML(self, workflow_exec, node=None):
        if node is None:
            node = ElementTree.Element('workflowExec')
        
        # set attributes
        node.set('id',self.convertToStr(workflow_exec.db_id, 'long'))
        node.set('user',self.convertToStr(workflow_exec.db_user, 'str'))
        node.set('ip',self.convertToStr(workflow_exec.db_ip, 'str'))
        node.set('vtVersion',self.convertToStr(workflow_exec.db_vt_version, 'str'))
        node.set('tsStart',self.convertToStr(workflow_exec.db_ts_start, 'datetime'))
        node.set('tsEnd',self.convertToStr(workflow_exec.db_ts_end, 'datetime'))
        node.set('parentId',self.convertToStr(workflow_exec.db_parent_id, 'long'))
        node.set('parentType',self.convertToStr(workflow_exec.db_parent_type, 'str'))
        node.set('parentVersion',self.convertToStr(workflow_exec.db_parent_version, 'long'))
        node.set('name',self.convertToStr(workflow_exec.db_name, 'str'))
        
        # set elements
        module_execs = workflow_exec.db_module_execs
        for module_exec in module_execs:
            childNode = ElementTree.SubElement(node, 'module_exec')
            self.getDao('module_exec').toXML(module_exec, childNode)
        
        return node

class DBConnectionXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.tag != 'connection':
            return None
        
        # read attributes
        data = node.get('id', None)
        id = self.convertFromStr(data, 'long')
        
        ports = []
        
        # read children
        for child in node.getchildren():
            if child.tag == 'port':
                _data = self.getDao('port').fromXML(child)
                ports.append(_data)
            elif child.text.strip() == '':
                pass
            else:
                print '*** ERROR *** tag = %s' % child.tag
        
        obj = DBConnection(id=id,
                           ports=ports)
        obj.is_dirty = False
        return obj
    
    def toXML(self, connection, node=None):
        if node is None:
            node = ElementTree.Element('connection')
        
        # set attributes
        node.set('id',self.convertToStr(connection.db_id, 'long'))
        
        # set elements
        ports = connection.db_ports
        for port in ports:
            childNode = ElementTree.SubElement(node, 'port')
            self.getDao('port').toXML(port, childNode)
        
        return node

class DBActionXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.tag != 'action':
            return None
        
        # read attributes
        data = node.get('id', None)
        id = self.convertFromStr(data, 'long')
        data = node.get('prevId', None)
        prevId = self.convertFromStr(data, 'long')
        data = node.get('date', None)
        date = self.convertFromStr(data, 'datetime')
        data = node.get('session', None)
        session = self.convertFromStr(data, 'str')
        data = node.get('user', None)
        user = self.convertFromStr(data, 'str')
        data = node.get('prune', None)
        prune = self.convertFromStr(data, 'int')
        
        annotations = []
        operations = []
        
        # read children
        for child in node.getchildren():
            if child.tag == 'annotation':
                _data = self.getDao('annotation').fromXML(child)
                annotations.append(_data)
            elif child.tag == 'add':
                _data = self.getDao('add').fromXML(child)
                operations.append(_data)
            elif child.tag == 'delete':
                _data = self.getDao('delete').fromXML(child)
                operations.append(_data)
            elif child.tag == 'change':
                _data = self.getDao('change').fromXML(child)
                operations.append(_data)
            elif child.text.strip() == '':
                pass
            else:
                print '*** ERROR *** tag = %s' % child.tag
        
        obj = DBAction(operations=operations,
                       id=id,
                       prevId=prevId,
                       date=date,
                       session=session,
                       user=user,
                       prune=prune,
                       annotations=annotations)
        obj.is_dirty = False
        return obj
    
    def toXML(self, action, node=None):
        if node is None:
            node = ElementTree.Element('action')
        
        # set attributes
        node.set('id',self.convertToStr(action.db_id, 'long'))
        node.set('prevId',self.convertToStr(action.db_prevId, 'long'))
        node.set('date',self.convertToStr(action.db_date, 'datetime'))
        node.set('session',self.convertToStr(action.db_session, 'str'))
        node.set('user',self.convertToStr(action.db_user, 'str'))
        node.set('prune',self.convertToStr(action.db_prune, 'int'))
        
        # set elements
        annotations = action.db_annotations
        for annotation in annotations:
            childNode = ElementTree.SubElement(node, 'annotation')
            self.getDao('annotation').toXML(annotation, childNode)
        operations = action.db_operations
        for operation in operations:
            if operation.vtType == 'add':
                childNode = ElementTree.SubElement(node, 'add')
                self.getDao('add').toXML(operation, childNode)
            elif operation.vtType == 'delete':
                childNode = ElementTree.SubElement(node, 'delete')
                self.getDao('delete').toXML(operation, childNode)
            elif operation.vtType == 'change':
                childNode = ElementTree.SubElement(node, 'change')
                self.getDao('change').toXML(operation, childNode)
        
        return node

class DBDeleteXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.tag != 'delete':
            return None
        
        # read attributes
        data = node.get('id', None)
        id = self.convertFromStr(data, 'long')
        data = node.get('what', None)
        what = self.convertFromStr(data, 'str')
        data = node.get('objectId', None)
        objectId = self.convertFromStr(data, 'long')
        data = node.get('parentObjId', None)
        parentObjId = self.convertFromStr(data, 'long')
        data = node.get('parentObjType', None)
        parentObjType = self.convertFromStr(data, 'str')
        
        obj = DBDelete(id=id,
                       what=what,
                       objectId=objectId,
                       parentObjId=parentObjId,
                       parentObjType=parentObjType)
        obj.is_dirty = False
        return obj
    
    def toXML(self, delete, node=None):
        if node is None:
            node = ElementTree.Element('delete')
        
        # set attributes
        node.set('id',self.convertToStr(delete.db_id, 'long'))
        node.set('what',self.convertToStr(delete.db_what, 'str'))
        node.set('objectId',self.convertToStr(delete.db_objectId, 'long'))
        node.set('parentObjId',self.convertToStr(delete.db_parentObjId, 'long'))
        node.set('parentObjType',self.convertToStr(delete.db_parentObjType, 'str'))
        
        return node

class DBVistrailXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.tag != 'vistrail':
            return None
        
        # read attributes
        data = node.get('id', None)
        id = self.convertFromStr(data, 'long')
        data = node.get('version', None)
        version = self.convertFromStr(data, 'str')
        data = node.get('name', None)
        name = self.convertFromStr(data, 'str')
        data = node.get('dbHost', None)
        dbHost = self.convertFromStr(data, 'str')
        data = node.get('dbPort', None)
        dbPort = self.convertFromStr(data, 'int')
        data = node.get('dbName', None)
        dbName = self.convertFromStr(data, 'str')
        
        actions = []
        tags = []
        abstractions = []
        
        # read children
        for child in node.getchildren():
            if child.tag == 'action':
                _data = self.getDao('action').fromXML(child)
                actions.append(_data)
            elif child.tag == 'tag':
                _data = self.getDao('tag').fromXML(child)
                tags.append(_data)
            elif child.tag == 'abstraction':
                _data = self.getDao('abstraction').fromXML(child)
                abstractions.append(_data)
            elif child.text.strip() == '':
                pass
            else:
                print '*** ERROR *** tag = %s' % child.tag
        
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
    
    def toXML(self, vistrail, node=None):
        if node is None:
            node = ElementTree.Element('vistrail')
        
        # set attributes
        node.set('id',self.convertToStr(vistrail.db_id, 'long'))
        node.set('version',self.convertToStr(vistrail.db_version, 'str'))
        node.set('name',self.convertToStr(vistrail.db_name, 'str'))
        node.set('dbHost',self.convertToStr(vistrail.db_dbHost, 'str'))
        node.set('dbPort',self.convertToStr(vistrail.db_dbPort, 'int'))
        node.set('dbName',self.convertToStr(vistrail.db_dbName, 'str'))
        
        # set elements
        actions = vistrail.db_actions
        for action in actions:
            childNode = ElementTree.SubElement(node, 'action')
            self.getDao('action').toXML(action, childNode)
        tags = vistrail.db_tags
        for tag in tags:
            childNode = ElementTree.SubElement(node, 'tag')
            self.getDao('tag').toXML(tag, childNode)
        abstractions = vistrail.db_abstractions
        for abstraction in abstractions:
            childNode = ElementTree.SubElement(node, 'abstraction')
            self.getDao('abstraction').toXML(abstraction, childNode)
        
        return node

class DBModuleExecXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.tag != 'moduleExec':
            return None
        
        # read attributes
        data = node.get('id', None)
        id = self.convertFromStr(data, 'long')
        data = node.get('tsStart', None)
        ts_start = self.convertFromStr(data, 'datetime')
        data = node.get('tsEnd', None)
        ts_end = self.convertFromStr(data, 'datetime')
        data = node.get('cached', None)
        cached = self.convertFromStr(data, 'int')
        data = node.get('moduleId', None)
        module_id = self.convertFromStr(data, 'long')
        data = node.get('moduleName', None)
        module_name = self.convertFromStr(data, 'str')
        data = node.get('completed', None)
        completed = self.convertFromStr(data, 'int')
        data = node.get('abstraction_id', None)
        abstraction_id = self.convertFromStr(data, 'long')
        data = node.get('abstraction_version', None)
        abstraction_version = self.convertFromStr(data, 'long')
        data = node.get('machine_id', None)
        machine_id = self.convertFromStr(data, 'long')
        
        annotations = []
        
        # read children
        for child in node.getchildren():
            if child.tag == 'annotation':
                _data = self.getDao('annotation').fromXML(child)
                annotations.append(_data)
            elif child.text.strip() == '':
                pass
            else:
                print '*** ERROR *** tag = %s' % child.tag
        
        obj = DBModuleExec(id=id,
                           ts_start=ts_start,
                           ts_end=ts_end,
                           cached=cached,
                           module_id=module_id,
                           module_name=module_name,
                           completed=completed,
                           abstraction_id=abstraction_id,
                           abstraction_version=abstraction_version,
                           machine_id=machine_id,
                           annotations=annotations)
        obj.is_dirty = False
        return obj
    
    def toXML(self, module_exec, node=None):
        if node is None:
            node = ElementTree.Element('moduleExec')
        
        # set attributes
        node.set('id',self.convertToStr(module_exec.db_id, 'long'))
        node.set('tsStart',self.convertToStr(module_exec.db_ts_start, 'datetime'))
        node.set('tsEnd',self.convertToStr(module_exec.db_ts_end, 'datetime'))
        node.set('cached',self.convertToStr(module_exec.db_cached, 'int'))
        node.set('moduleId',self.convertToStr(module_exec.db_module_id, 'long'))
        node.set('moduleName',self.convertToStr(module_exec.db_module_name, 'str'))
        node.set('completed',self.convertToStr(module_exec.db_completed, 'int'))
        node.set('abstraction_id',self.convertToStr(module_exec.db_abstraction_id, 'long'))
        node.set('abstraction_version',self.convertToStr(module_exec.db_abstraction_version, 'long'))
        node.set('machine_id',self.convertToStr(module_exec.db_machine_id, 'long'))
        
        # set elements
        annotations = module_exec.db_annotations
        for annotation in annotations:
            childNode = ElementTree.SubElement(node, 'annotation')
            self.getDao('annotation').toXML(annotation, childNode)
        
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
        if 'group' not in self:
            self['group'] = DBGroupXMLDAOBase(self)
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
        if 'parameter' not in self:
            self['parameter'] = DBParameterXMLDAOBase(self)
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
        if 'workflow_exec' not in self:
            self['workflow_exec'] = DBWorkflowExecXMLDAOBase(self)
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
