
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

from db.versions.v0_3_0.persistence.xml.xml_dao import XMLDAO
from db.versions.v0_3_0.domain.auto_gen import *

class DBChangeParameterXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'set':
            return None
        
        # read attributes
        moduleId = self.convertFromStr(self.getAttribute(node, 'moduleId'), 'long')
        alias = self.convertFromStr(self.getAttribute(node, 'alias'), 'str')
        functionId = self.convertFromStr(self.getAttribute(node, 'functionId'), 'long')
        function = self.convertFromStr(self.getAttribute(node, 'function'), 'str')
        parameterId = self.convertFromStr(self.getAttribute(node, 'parameterId'), 'long')
        parameter = self.convertFromStr(self.getAttribute(node, 'parameter'), 'str')
        type = self.convertFromStr(self.getAttribute(node, 'type'), 'str')
        value = self.convertFromStr(self.getAttribute(node, 'value'), 'str')
        
        return DBChangeParameter(moduleId=moduleId,
                                 alias=alias,
                                 functionId=functionId,
                                 function=function,
                                 parameterId=parameterId,
                                 parameter=parameter,
                                 type=type,
                                 value=value)
    
    def toXML(self, changeParameter, doc):
        node = doc.createElement('set')
        
        # set attributes
        node.setAttribute('moduleId',self.convertToStr(changeParameter.db_moduleId, 'long'))
        node.setAttribute('alias',self.convertToStr(changeParameter.db_alias, 'str'))
        node.setAttribute('functionId',self.convertToStr(changeParameter.db_functionId, 'long'))
        node.setAttribute('function',self.convertToStr(changeParameter.db_function, 'str'))
        node.setAttribute('parameterId',self.convertToStr(changeParameter.db_parameterId, 'long'))
        node.setAttribute('parameter',self.convertToStr(changeParameter.db_parameter, 'str'))
        node.setAttribute('type',self.convertToStr(changeParameter.db_type, 'str'))
        node.setAttribute('value',self.convertToStr(changeParameter.db_value, 'str'))
        
        return node

class DBDeleteFunctionXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'function':
            return None
        
        # read attributes
        moduleId = self.convertFromStr(self.getAttribute(node, 'moduleId'), 'long')
        functionId = self.convertFromStr(self.getAttribute(node, 'functionId'), 'long')
        
        return DBDeleteFunction(moduleId=moduleId,
                                functionId=functionId)
    
    def toXML(self, deleteFunction, doc):
        node = doc.createElement('function')
        
        # set attributes
        node.setAttribute('moduleId',self.convertToStr(deleteFunction.db_moduleId, 'long'))
        node.setAttribute('functionId',self.convertToStr(deleteFunction.db_functionId, 'long'))
        
        return node

class DBDeleteConnectionXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'connection':
            return None
        
        # read attributes
        connectionId = self.convertFromStr(self.getAttribute(node, 'connectionId'), 'long')
        
        return DBDeleteConnection(connectionId=connectionId)
    
    def toXML(self, deleteConnection, doc):
        node = doc.createElement('connection')
        
        # set attributes
        node.setAttribute('connectionId',self.convertToStr(deleteConnection.db_connectionId, 'long'))
        
        return node

class DBAddModuleXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'object':
            return None
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        cache = self.convertFromStr(self.getAttribute(node, 'cache'), 'int')
        name = self.convertFromStr(self.getAttribute(node, 'name'), 'str')
        x = self.convertFromStr(self.getAttribute(node, 'x'), 'float')
        y = self.convertFromStr(self.getAttribute(node, 'y'), 'float')
        
        return DBAddModule(id=id,
                           cache=cache,
                           name=name,
                           x=x,
                           y=y)
    
    def toXML(self, addModule, doc):
        node = doc.createElement('object')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(addModule.db_id, 'long'))
        node.setAttribute('cache',self.convertToStr(addModule.db_cache, 'int'))
        node.setAttribute('name',self.convertToStr(addModule.db_name, 'str'))
        node.setAttribute('x',self.convertToStr(addModule.db_x, 'float'))
        node.setAttribute('y',self.convertToStr(addModule.db_y, 'float'))
        
        return node

class DBDeleteAnnotationXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'annotation':
            return None
        
        # read attributes
        moduleId = self.convertFromStr(self.getAttribute(node, 'moduleId'), 'long')
        key = self.convertFromStr(self.getAttribute(node, 'key'), 'str')
        
        return DBDeleteAnnotation(moduleId=moduleId,
                                  key=key)
    
    def toXML(self, deleteAnnotation, doc):
        node = doc.createElement('annotation')
        
        # set attributes
        node.setAttribute('moduleId',self.convertToStr(deleteAnnotation.db_moduleId, 'long'))
        node.setAttribute('key',self.convertToStr(deleteAnnotation.db_key, 'str'))
        
        return node

class DBDeleteModulePortXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'deletePort':
            return None
        
        # read attributes
        moduleId = self.convertFromStr(self.getAttribute(node, 'moduleId'), 'long')
        portType = self.convertFromStr(self.getAttribute(node, 'portType'), 'str')
        portName = self.convertFromStr(self.getAttribute(node, 'portName'), 'str')
        
        return DBDeleteModulePort(moduleId=moduleId,
                                  portType=portType,
                                  portName=portName)
    
    def toXML(self, deleteModulePort, doc):
        node = doc.createElement('deletePort')
        
        # set attributes
        node.setAttribute('moduleId',self.convertToStr(deleteModulePort.db_moduleId, 'long'))
        node.setAttribute('portType',self.convertToStr(deleteModulePort.db_portType, 'str'))
        node.setAttribute('portName',self.convertToStr(deleteModulePort.db_portName, 'str'))
        
        return node

class DBDeleteModuleXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'module':
            return None
        
        # read attributes
        moduleId = self.convertFromStr(self.getAttribute(node, 'moduleId'), 'long')
        
        return DBDeleteModule(moduleId=moduleId)
    
    def toXML(self, deleteModule, doc):
        node = doc.createElement('module')
        
        # set attributes
        node.setAttribute('moduleId',self.convertToStr(deleteModule.db_moduleId, 'long'))
        
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
        time = self.convertFromStr(self.getAttribute(node, 'time'), 'long')
        name = self.convertFromStr(self.getAttribute(node, 'name'), 'str')
        
        return DBTag(time=time,
                     name=name)
    
    def toXML(self, tag, doc):
        node = doc.createElement('tag')
        
        # set attributes
        node.setAttribute('time',self.convertToStr(tag.db_time, 'long'))
        node.setAttribute('name',self.convertToStr(tag.db_name, 'str'))
        
        return node

class DBAddModulePortXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'addPort':
            return None
        
        # read attributes
        moduleId = self.convertFromStr(self.getAttribute(node, 'moduleId'), 'long')
        portType = self.convertFromStr(self.getAttribute(node, 'portType'), 'str')
        portName = self.convertFromStr(self.getAttribute(node, 'portName'), 'str')
        portSpec = self.convertFromStr(self.getAttribute(node, 'portSpec'), 'str')
        
        return DBAddModulePort(moduleId=moduleId,
                               portType=portType,
                               portName=portName,
                               portSpec=portSpec)
    
    def toXML(self, addModulePort, doc):
        node = doc.createElement('addPort')
        
        # set attributes
        node.setAttribute('moduleId',self.convertToStr(addModulePort.db_moduleId, 'long'))
        node.setAttribute('portType',self.convertToStr(addModulePort.db_portType, 'str'))
        node.setAttribute('portName',self.convertToStr(addModulePort.db_portName, 'str'))
        node.setAttribute('portSpec',self.convertToStr(addModulePort.db_portSpec, 'str'))
        
        return node

class DBActionXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'action':
            return None
        
        datas = []
        
        # read attributes
        time = self.convertFromStr(self.getAttribute(node, 'time'), 'long')
        parent = self.convertFromStr(self.getAttribute(node, 'parent'), 'long')
        user = self.convertFromStr(self.getAttribute(node, 'user'), 'str')
        what = self.convertFromStr(self.getAttribute(node, 'what'), 'str')
        date = self.convertFromStr(self.getAttribute(node, 'date'), 'str')
        
        notes = None
        
        # read children
        for child in node.childNodes:
            if child.nodeName == 'notes':
                notes = self.convertFromStr(child.firstChild.nodeValue,'str')
            elif what == 'addModule' and child.nodeName == 'object':
                data = self.getDao('addModule').fromXML(child)
                datas.append(data)
                what = 'addModule'
            elif what == 'addConnection' and child.nodeName == 'connect':
                data = self.getDao('addConnection').fromXML(child)
                datas.append(data)
                what = 'addConnection'
            elif what == 'changeParameter' and child.nodeName == 'set':
                data = self.getDao('changeParameter').fromXML(child)
                datas.append(data)
                what = 'changeParameter'
            elif what == 'changeAnnotation' and child.nodeName == 'set':
                data = self.getDao('changeAnnotation').fromXML(child)
                datas.append(data)
                what = 'changeAnnotation'
            elif what == 'addModulePort' and child.nodeName == 'addPort':
                data = self.getDao('addModulePort').fromXML(child)
                datas.append(data)
                what = 'addModulePort'
            elif what == 'moveModule' and child.nodeName == 'move':
                data = self.getDao('moveModule').fromXML(child)
                datas.append(data)
                what = 'moveModule'
            elif what == 'deleteModule' and child.nodeName == 'module':
                data = self.getDao('deleteModule').fromXML(child)
                datas.append(data)
                what = 'deleteModule'
            elif what == 'deleteConnection' and child.nodeName == 'connection':
                data = self.getDao('deleteConnection').fromXML(child)
                datas.append(data)
                what = 'deleteConnection'
            elif what == 'deleteFunction' and child.nodeName == 'function':
                data = self.getDao('deleteFunction').fromXML(child)
                datas.append(data)
                what = 'deleteFunction'
            elif what == 'deleteAnnotation' and child.nodeName == 'annotation':
                data = self.getDao('deleteAnnotation').fromXML(child)
                datas.append(data)
                what = 'deleteAnnotation'
            elif what == 'deleteModulePort' and child.nodeName == 'deletePort':
                data = self.getDao('deleteModulePort').fromXML(child)
                datas.append(data)
                what = 'deleteModulePort'
            elif child.nodeType != child.TEXT_NODE:
                print '*** ERROR *** nodeName = %s' % child.nodeName
        
        return DBAction(time=time,
                        parent=parent,
                        user=user,
                        what=what,
                        date=date,
                        notes=notes,
                        datas=datas)
    
    def toXML(self, action, doc):
        node = doc.createElement('action')
        
        # set attributes
        node.setAttribute('time',self.convertToStr(action.db_time, 'long'))
        node.setAttribute('parent',self.convertToStr(action.db_parent, 'long'))
        node.setAttribute('user',self.convertToStr(action.db_user, 'str'))
        node.setAttribute('what',self.convertToStr(action.db_what, 'str'))
        node.setAttribute('date',self.convertToStr(action.db_date, 'str'))
        
        # set elements
        if action.db_notes is not None:
            child = action.db_notes
            notesNode = doc.createElement('notes')
            notesText = doc.createTextNode(self.convertToStr(child, 'str'))
            notesNode.appendChild(notesText)
            node.appendChild(notesNode)
        datas = action.db_datas
        for data in datas:
            if action.db_what == 'addModule':
                node.appendChild(self.getDao('addModule').toXML(data, doc))
            elif action.db_what == 'addConnection':
                node.appendChild(self.getDao('addConnection').toXML(data, doc))
            elif action.db_what == 'changeParameter':
                node.appendChild(self.getDao('changeParameter').toXML(data, doc))
            elif action.db_what == 'changeAnnotation':
                node.appendChild(self.getDao('changeAnnotation').toXML(data, doc))
            elif action.db_what == 'addModulePort':
                node.appendChild(self.getDao('addModulePort').toXML(data, doc))
            elif action.db_what == 'moveModule':
                node.appendChild(self.getDao('moveModule').toXML(data, doc))
            elif action.db_what == 'deleteModule':
                node.appendChild(self.getDao('deleteModule').toXML(data, doc))
            elif action.db_what == 'deleteConnection':
                node.appendChild(self.getDao('deleteConnection').toXML(data, doc))
            elif action.db_what == 'deleteFunction':
                node.appendChild(self.getDao('deleteFunction').toXML(data, doc))
            elif action.db_what == 'deleteAnnotation':
                node.appendChild(self.getDao('deleteAnnotation').toXML(data, doc))
            elif action.db_what == 'deleteModulePort':
                node.appendChild(self.getDao('deleteModulePort').toXML(data, doc))
        
        return node

class DBAddConnectionXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'connect':
            return None
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        destinationId = self.convertFromStr(self.getAttribute(node, 'destinationId'), 'long')
        destinationModule = self.convertFromStr(self.getAttribute(node, 'destinationModule'), 'str')
        destinationPort = self.convertFromStr(self.getAttribute(node, 'destinationPort'), 'str')
        sourceId = self.convertFromStr(self.getAttribute(node, 'sourceId'), 'long')
        sourceModule = self.convertFromStr(self.getAttribute(node, 'sourceModule'), 'str')
        sourcePort = self.convertFromStr(self.getAttribute(node, 'sourcePort'), 'str')
        
        return DBAddConnection(id=id,
                               destinationId=destinationId,
                               destinationModule=destinationModule,
                               destinationPort=destinationPort,
                               sourceId=sourceId,
                               sourceModule=sourceModule,
                               sourcePort=sourcePort)
    
    def toXML(self, addConnection, doc):
        node = doc.createElement('connect')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(addConnection.db_id, 'long'))
        node.setAttribute('destinationId',self.convertToStr(addConnection.db_destinationId, 'long'))
        node.setAttribute('destinationModule',self.convertToStr(addConnection.db_destinationModule, 'str'))
        node.setAttribute('destinationPort',self.convertToStr(addConnection.db_destinationPort, 'str'))
        node.setAttribute('sourceId',self.convertToStr(addConnection.db_sourceId, 'long'))
        node.setAttribute('sourceModule',self.convertToStr(addConnection.db_sourceModule, 'str'))
        node.setAttribute('sourcePort',self.convertToStr(addConnection.db_sourcePort, 'str'))
        
        return node

class DBMoveModuleXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'move':
            return None
        
        # read attributes
        id = self.convertFromStr(self.getAttribute(node, 'id'), 'long')
        dx = self.convertFromStr(self.getAttribute(node, 'dx'), 'float')
        dy = self.convertFromStr(self.getAttribute(node, 'dy'), 'float')
        
        return DBMoveModule(id=id,
                            dx=dx,
                            dy=dy)
    
    def toXML(self, moveModule, doc):
        node = doc.createElement('move')
        
        # set attributes
        node.setAttribute('id',self.convertToStr(moveModule.db_id, 'long'))
        node.setAttribute('dx',self.convertToStr(moveModule.db_dx, 'float'))
        node.setAttribute('dy',self.convertToStr(moveModule.db_dy, 'float'))
        
        return node

class DBVistrailXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'visTrail':
            return None
        
        actions = {}
        tags = {}
        
        # read attributes
        version = self.convertFromStr(self.getAttribute(node, 'version'), 'str')
        
        
        # read children
        for child in node.childNodes:
            if child.nodeName == 'action':
                action = self.getDao('action').fromXML(child)
                actions[action.db_time] = action
            elif child.nodeName == 'tag':
                tag = self.getDao('tag').fromXML(child)
                tags[tag.db_time] = tag
            elif child.nodeType != child.TEXT_NODE:
                print '*** ERROR *** nodeName = %s' % child.nodeName
        
        return DBVistrail(version=version,
                          actions=actions,
                          tags=tags)
    
    def toXML(self, vistrail, doc):
        node = doc.createElement('visTrail')
        
        # set attributes
        node.setAttribute('version',self.convertToStr(vistrail.db_version, 'str'))
        
        # set elements
        actions = vistrail.db_actions
        for action in actions.itervalues():
            node.appendChild(self.getDao('action').toXML(action, doc))
        tags = vistrail.db_tags
        for tag in tags.itervalues():
            node.appendChild(self.getDao('tag').toXML(tag, doc))
        
        return node

class DBChangeAnnotationXMLDAOBase(XMLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromXML(self, node):
        if node.nodeName != 'set':
            return None
        
        # read attributes
        moduleId = self.convertFromStr(self.getAttribute(node, 'moduleId'), 'long')
        key = self.convertFromStr(self.getAttribute(node, 'key'), 'str')
        value = self.convertFromStr(self.getAttribute(node, 'value'), 'str')
        
        return DBChangeAnnotation(moduleId=moduleId,
                                  key=key,
                                  value=value)
    
    def toXML(self, changeAnnotation, doc):
        node = doc.createElement('set')
        
        # set attributes
        node.setAttribute('moduleId',self.convertToStr(changeAnnotation.db_moduleId, 'long'))
        node.setAttribute('key',self.convertToStr(changeAnnotation.db_key, 'str'))
        node.setAttribute('value',self.convertToStr(changeAnnotation.db_value, 'str'))
        
        return node

"""generated automatically by auto_dao.py"""

class XMLDAOListBase(dict):

    def __init__(self, daos=None):
        if daos is not None:
            dict.update(self, daos)

        if 'changeParameter' not in self:
            self['changeParameter'] = DBChangeParameterXMLDAOBase(self)
        if 'deleteFunction' not in self:
            self['deleteFunction'] = DBDeleteFunctionXMLDAOBase(self)
        if 'deleteConnection' not in self:
            self['deleteConnection'] = DBDeleteConnectionXMLDAOBase(self)
        if 'addModule' not in self:
            self['addModule'] = DBAddModuleXMLDAOBase(self)
        if 'deleteAnnotation' not in self:
            self['deleteAnnotation'] = DBDeleteAnnotationXMLDAOBase(self)
        if 'deleteModulePort' not in self:
            self['deleteModulePort'] = DBDeleteModulePortXMLDAOBase(self)
        if 'deleteModule' not in self:
            self['deleteModule'] = DBDeleteModuleXMLDAOBase(self)
        if 'tag' not in self:
            self['tag'] = DBTagXMLDAOBase(self)
        if 'addModulePort' not in self:
            self['addModulePort'] = DBAddModulePortXMLDAOBase(self)
        if 'action' not in self:
            self['action'] = DBActionXMLDAOBase(self)
        if 'addConnection' not in self:
            self['addConnection'] = DBAddConnectionXMLDAOBase(self)
        if 'moveModule' not in self:
            self['moveModule'] = DBMoveModuleXMLDAOBase(self)
        if 'vistrail' not in self:
            self['vistrail'] = DBVistrailXMLDAOBase(self)
        if 'changeAnnotation' not in self:
            self['changeAnnotation'] = DBChangeAnnotationXMLDAOBase(self)
