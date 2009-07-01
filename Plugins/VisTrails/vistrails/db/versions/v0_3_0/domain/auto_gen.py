
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

class DBChangeParameter(object):

    vtType = 'changeParameter'

    def __init__(self, moduleId=None, alias=None, functionId=None, function=None, parameterId=None, parameter=None, type=None, value=None):
        self.__db_moduleId = moduleId
        self.__db_alias = alias
        self.__db_functionId = functionId
        self.__db_function = function
        self.__db_parameterId = parameterId
        self.__db_parameter = parameter
        self.__db_type = type
        self.__db_value = value
    def __get_db_moduleId(self):
        return self.__db_moduleId
    def __set_db_moduleId(self, moduleId):
        self.__db_moduleId = moduleId
    db_moduleId = property(__get_db_moduleId, __set_db_moduleId)
    def db_add_moduleId(self, moduleId):
        self.__db_moduleId = moduleId
    def db_change_moduleId(self, moduleId):
        self.__db_moduleId = moduleId
    def db_delete_moduleId(self, moduleId):
        self.__db_moduleId = None
    
    def __get_db_alias(self):
        return self.__db_alias
    def __set_db_alias(self, alias):
        self.__db_alias = alias
    db_alias = property(__get_db_alias, __set_db_alias)
    def db_add_alias(self, alias):
        self.__db_alias = alias
    def db_change_alias(self, alias):
        self.__db_alias = alias
    def db_delete_alias(self, alias):
        self.__db_alias = None
    
    def __get_db_functionId(self):
        return self.__db_functionId
    def __set_db_functionId(self, functionId):
        self.__db_functionId = functionId
    db_functionId = property(__get_db_functionId, __set_db_functionId)
    def db_add_functionId(self, functionId):
        self.__db_functionId = functionId
    def db_change_functionId(self, functionId):
        self.__db_functionId = functionId
    def db_delete_functionId(self, functionId):
        self.__db_functionId = None
    
    def __get_db_function(self):
        return self.__db_function
    def __set_db_function(self, function):
        self.__db_function = function
    db_function = property(__get_db_function, __set_db_function)
    def db_add_function(self, function):
        self.__db_function = function
    def db_change_function(self, function):
        self.__db_function = function
    def db_delete_function(self, function):
        self.__db_function = None
    
    def __get_db_parameterId(self):
        return self.__db_parameterId
    def __set_db_parameterId(self, parameterId):
        self.__db_parameterId = parameterId
    db_parameterId = property(__get_db_parameterId, __set_db_parameterId)
    def db_add_parameterId(self, parameterId):
        self.__db_parameterId = parameterId
    def db_change_parameterId(self, parameterId):
        self.__db_parameterId = parameterId
    def db_delete_parameterId(self, parameterId):
        self.__db_parameterId = None
    
    def __get_db_parameter(self):
        return self.__db_parameter
    def __set_db_parameter(self, parameter):
        self.__db_parameter = parameter
    db_parameter = property(__get_db_parameter, __set_db_parameter)
    def db_add_parameter(self, parameter):
        self.__db_parameter = parameter
    def db_change_parameter(self, parameter):
        self.__db_parameter = parameter
    def db_delete_parameter(self, parameter):
        self.__db_parameter = None
    
    def __get_db_type(self):
        return self.__db_type
    def __set_db_type(self, type):
        self.__db_type = type
    db_type = property(__get_db_type, __set_db_type)
    def db_add_type(self, type):
        self.__db_type = type
    def db_change_type(self, type):
        self.__db_type = type
    def db_delete_type(self, type):
        self.__db_type = None
    
    def __get_db_value(self):
        return self.__db_value
    def __set_db_value(self, value):
        self.__db_value = value
    db_value = property(__get_db_value, __set_db_value)
    def db_add_value(self, value):
        self.__db_value = value
    def db_change_value(self, value):
        self.__db_value = value
    def db_delete_value(self, value):
        self.__db_value = None
    
    def getPrimaryKey(self):
        return self.__db_moduleId

"""generated automatically by auto_dao.py"""

class DBDeleteFunction(object):

    vtType = 'deleteFunction'

    def __init__(self, moduleId=None, functionId=None):
        self.__db_moduleId = moduleId
        self.__db_functionId = functionId
    def __get_db_moduleId(self):
        return self.__db_moduleId
    def __set_db_moduleId(self, moduleId):
        self.__db_moduleId = moduleId
    db_moduleId = property(__get_db_moduleId, __set_db_moduleId)
    def db_add_moduleId(self, moduleId):
        self.__db_moduleId = moduleId
    def db_change_moduleId(self, moduleId):
        self.__db_moduleId = moduleId
    def db_delete_moduleId(self, moduleId):
        self.__db_moduleId = None
    
    def __get_db_functionId(self):
        return self.__db_functionId
    def __set_db_functionId(self, functionId):
        self.__db_functionId = functionId
    db_functionId = property(__get_db_functionId, __set_db_functionId)
    def db_add_functionId(self, functionId):
        self.__db_functionId = functionId
    def db_change_functionId(self, functionId):
        self.__db_functionId = functionId
    def db_delete_functionId(self, functionId):
        self.__db_functionId = None
    
    def getPrimaryKey(self):
        return self.__db_moduleId

"""generated automatically by auto_dao.py"""

class DBDeleteConnection(object):

    vtType = 'deleteConnection'

    def __init__(self, connectionId=None):
        self.__db_connectionId = connectionId
    def __get_db_connectionId(self):
        return self.__db_connectionId
    def __set_db_connectionId(self, connectionId):
        self.__db_connectionId = connectionId
    db_connectionId = property(__get_db_connectionId, __set_db_connectionId)
    def db_add_connectionId(self, connectionId):
        self.__db_connectionId = connectionId
    def db_change_connectionId(self, connectionId):
        self.__db_connectionId = connectionId
    def db_delete_connectionId(self, connectionId):
        self.__db_connectionId = None
    
    def getPrimaryKey(self):
        return self.__db_connectionId

"""generated automatically by auto_dao.py"""

class DBAddModule(object):

    vtType = 'addModule'

    def __init__(self, id=None, cache=None, name=None, x=None, y=None):
        self.__db_id = id
        self.__db_cache = cache
        self.__db_name = name
        self.__db_x = x
        self.__db_y = y
    def __get_db_id(self):
        return self.__db_id
    def __set_db_id(self, id):
        self.__db_id = id
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
    db_name = property(__get_db_name, __set_db_name)
    def db_add_name(self, name):
        self.__db_name = name
    def db_change_name(self, name):
        self.__db_name = name
    def db_delete_name(self, name):
        self.__db_name = None
    
    def __get_db_x(self):
        return self.__db_x
    def __set_db_x(self, x):
        self.__db_x = x
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
    db_y = property(__get_db_y, __set_db_y)
    def db_add_y(self, y):
        self.__db_y = y
    def db_change_y(self, y):
        self.__db_y = y
    def db_delete_y(self, y):
        self.__db_y = None
    
    def getPrimaryKey(self):
        return self.__db_id

"""generated automatically by auto_dao.py"""

class DBDeleteAnnotation(object):

    vtType = 'deleteAnnotation'

    def __init__(self, moduleId=None, key=None):
        self.__db_moduleId = moduleId
        self.__db_key = key
    def __get_db_moduleId(self):
        return self.__db_moduleId
    def __set_db_moduleId(self, moduleId):
        self.__db_moduleId = moduleId
    db_moduleId = property(__get_db_moduleId, __set_db_moduleId)
    def db_add_moduleId(self, moduleId):
        self.__db_moduleId = moduleId
    def db_change_moduleId(self, moduleId):
        self.__db_moduleId = moduleId
    def db_delete_moduleId(self, moduleId):
        self.__db_moduleId = None
    
    def __get_db_key(self):
        return self.__db_key
    def __set_db_key(self, key):
        self.__db_key = key
    db_key = property(__get_db_key, __set_db_key)
    def db_add_key(self, key):
        self.__db_key = key
    def db_change_key(self, key):
        self.__db_key = key
    def db_delete_key(self, key):
        self.__db_key = None
    
    def getPrimaryKey(self):
        return self.__db_moduleId

"""generated automatically by auto_dao.py"""

class DBDeleteModulePort(object):

    vtType = 'deleteModulePort'

    def __init__(self, moduleId=None, portType=None, portName=None):
        self.__db_moduleId = moduleId
        self.__db_portType = portType
        self.__db_portName = portName
    def __get_db_moduleId(self):
        return self.__db_moduleId
    def __set_db_moduleId(self, moduleId):
        self.__db_moduleId = moduleId
    db_moduleId = property(__get_db_moduleId, __set_db_moduleId)
    def db_add_moduleId(self, moduleId):
        self.__db_moduleId = moduleId
    def db_change_moduleId(self, moduleId):
        self.__db_moduleId = moduleId
    def db_delete_moduleId(self, moduleId):
        self.__db_moduleId = None
    
    def __get_db_portType(self):
        return self.__db_portType
    def __set_db_portType(self, portType):
        self.__db_portType = portType
    db_portType = property(__get_db_portType, __set_db_portType)
    def db_add_portType(self, portType):
        self.__db_portType = portType
    def db_change_portType(self, portType):
        self.__db_portType = portType
    def db_delete_portType(self, portType):
        self.__db_portType = None
    
    def __get_db_portName(self):
        return self.__db_portName
    def __set_db_portName(self, portName):
        self.__db_portName = portName
    db_portName = property(__get_db_portName, __set_db_portName)
    def db_add_portName(self, portName):
        self.__db_portName = portName
    def db_change_portName(self, portName):
        self.__db_portName = portName
    def db_delete_portName(self, portName):
        self.__db_portName = None
    
    def getPrimaryKey(self):
        return self.__db_moduleId

"""generated automatically by auto_dao.py"""

class DBDeleteModule(object):

    vtType = 'deleteModule'

    def __init__(self, moduleId=None):
        self.__db_moduleId = moduleId
    def __get_db_moduleId(self):
        return self.__db_moduleId
    def __set_db_moduleId(self, moduleId):
        self.__db_moduleId = moduleId
    db_moduleId = property(__get_db_moduleId, __set_db_moduleId)
    def db_add_moduleId(self, moduleId):
        self.__db_moduleId = moduleId
    def db_change_moduleId(self, moduleId):
        self.__db_moduleId = moduleId
    def db_delete_moduleId(self, moduleId):
        self.__db_moduleId = None
    
    def getPrimaryKey(self):
        return self.__db_moduleId

"""generated automatically by auto_dao.py"""

class DBTag(object):

    vtType = 'tag'

    def __init__(self, time=None, name=None):
        self.__db_time = time
        self.__db_name = name
    def __get_db_time(self):
        return self.__db_time
    def __set_db_time(self, time):
        self.__db_time = time
    db_time = property(__get_db_time, __set_db_time)
    def db_add_time(self, time):
        self.__db_time = time
    def db_change_time(self, time):
        self.__db_time = time
    def db_delete_time(self, time):
        self.__db_time = None
    
    def __get_db_name(self):
        return self.__db_name
    def __set_db_name(self, name):
        self.__db_name = name
    db_name = property(__get_db_name, __set_db_name)
    def db_add_name(self, name):
        self.__db_name = name
    def db_change_name(self, name):
        self.__db_name = name
    def db_delete_name(self, name):
        self.__db_name = None
    
    def getPrimaryKey(self):
        return self.__db_time

"""generated automatically by auto_dao.py"""

class DBAddModulePort(object):

    vtType = 'addModulePort'

    def __init__(self, moduleId=None, portType=None, portName=None, portSpec=None):
        self.__db_moduleId = moduleId
        self.__db_portType = portType
        self.__db_portName = portName
        self.__db_portSpec = portSpec
    def __get_db_moduleId(self):
        return self.__db_moduleId
    def __set_db_moduleId(self, moduleId):
        self.__db_moduleId = moduleId
    db_moduleId = property(__get_db_moduleId, __set_db_moduleId)
    def db_add_moduleId(self, moduleId):
        self.__db_moduleId = moduleId
    def db_change_moduleId(self, moduleId):
        self.__db_moduleId = moduleId
    def db_delete_moduleId(self, moduleId):
        self.__db_moduleId = None
    
    def __get_db_portType(self):
        return self.__db_portType
    def __set_db_portType(self, portType):
        self.__db_portType = portType
    db_portType = property(__get_db_portType, __set_db_portType)
    def db_add_portType(self, portType):
        self.__db_portType = portType
    def db_change_portType(self, portType):
        self.__db_portType = portType
    def db_delete_portType(self, portType):
        self.__db_portType = None
    
    def __get_db_portName(self):
        return self.__db_portName
    def __set_db_portName(self, portName):
        self.__db_portName = portName
    db_portName = property(__get_db_portName, __set_db_portName)
    def db_add_portName(self, portName):
        self.__db_portName = portName
    def db_change_portName(self, portName):
        self.__db_portName = portName
    def db_delete_portName(self, portName):
        self.__db_portName = None
    
    def __get_db_portSpec(self):
        return self.__db_portSpec
    def __set_db_portSpec(self, portSpec):
        self.__db_portSpec = portSpec
    db_portSpec = property(__get_db_portSpec, __set_db_portSpec)
    def db_add_portSpec(self, portSpec):
        self.__db_portSpec = portSpec
    def db_change_portSpec(self, portSpec):
        self.__db_portSpec = portSpec
    def db_delete_portSpec(self, portSpec):
        self.__db_portSpec = None
    
    def getPrimaryKey(self):
        return self.__db_moduleId

"""generated automatically by auto_dao.py"""

class DBAction(object):

    vtType = 'action'

    def __init__(self, time=None, parent=None, user=None, what=None, date=None, notes=None, datas=None):
        self.__db_time = time
        self.__db_parent = parent
        self.__db_user = user
        self.__db_what = what
        self.__db_date = date
        self.__db_notes = notes
        if datas is None:
            self.__db_datas = []
        else:
            self.__db_datas = datas
    def __get_db_time(self):
        return self.__db_time
    def __set_db_time(self, time):
        self.__db_time = time
    db_time = property(__get_db_time, __set_db_time)
    def db_add_time(self, time):
        self.__db_time = time
    def db_change_time(self, time):
        self.__db_time = time
    def db_delete_time(self, time):
        self.__db_time = None
    
    def __get_db_parent(self):
        return self.__db_parent
    def __set_db_parent(self, parent):
        self.__db_parent = parent
    db_parent = property(__get_db_parent, __set_db_parent)
    def db_add_parent(self, parent):
        self.__db_parent = parent
    def db_change_parent(self, parent):
        self.__db_parent = parent
    def db_delete_parent(self, parent):
        self.__db_parent = None
    
    def __get_db_user(self):
        return self.__db_user
    def __set_db_user(self, user):
        self.__db_user = user
    db_user = property(__get_db_user, __set_db_user)
    def db_add_user(self, user):
        self.__db_user = user
    def db_change_user(self, user):
        self.__db_user = user
    def db_delete_user(self, user):
        self.__db_user = None
    
    def __get_db_what(self):
        return self.__db_what
    def __set_db_what(self, what):
        self.__db_what = what
    db_what = property(__get_db_what, __set_db_what)
    def db_add_what(self, what):
        self.__db_what = what
    def db_change_what(self, what):
        self.__db_what = what
    def db_delete_what(self, what):
        self.__db_what = None
    
    def __get_db_date(self):
        return self.__db_date
    def __set_db_date(self, date):
        self.__db_date = date
    db_date = property(__get_db_date, __set_db_date)
    def db_add_date(self, date):
        self.__db_date = date
    def db_change_date(self, date):
        self.__db_date = date
    def db_delete_date(self, date):
        self.__db_date = None
    
    def __get_db_notes(self):
        return self.__db_notes
    def __set_db_notes(self, notes):
        self.__db_notes = notes
    db_notes = property(__get_db_notes, __set_db_notes)
    def db_add_notes(self, notes):
        self.__db_notes = notes
    def db_change_notes(self, notes):
        self.__db_notes = notes
    def db_delete_notes(self, notes):
        self.__db_notes = None
    
    def __get_db_datas(self):
        return self.__db_datas
    def __set_db_datas(self, datas):
        self.__db_datas = datas
    db_datas = property(__get_db_datas, __set_db_datas)
    def db_get_datas(self):
        return self.__db_datas
    def db_add_data(self, data):
        self.__db_datas.append(data)
    def db_change_data(self, data):
        found = False
        for i in xrange(len(self.__db_datas)):
            if self.__db_datas[i].db_id == data.db_id:
                self.__db_datas[i] = data
                found = True
                break
        if not found:
            self.__db_datas.append(data)
    def db_delete_data(self, data):
        for i in xrange(len(self.__db_datas)):
            if self.__db_datas[i].db_id == data.db_id:
                del self.__db_datas[i]
                break
    def db_get_data(self, key):
        for i in xrange(len(self.__db_datas)):
            if self.__db_datas[i].db_id == data.db_id:
                return self.__db_datas[i]
        return None
    
    def getPrimaryKey(self):
        return self.__db_time

"""generated automatically by auto_dao.py"""

class DBAddConnection(object):

    vtType = 'addConnection'

    def __init__(self, id=None, destinationId=None, destinationModule=None, destinationPort=None, sourceId=None, sourceModule=None, sourcePort=None):
        self.__db_id = id
        self.__db_destinationId = destinationId
        self.__db_destinationModule = destinationModule
        self.__db_destinationPort = destinationPort
        self.__db_sourceId = sourceId
        self.__db_sourceModule = sourceModule
        self.__db_sourcePort = sourcePort
    def __get_db_id(self):
        return self.__db_id
    def __set_db_id(self, id):
        self.__db_id = id
    db_id = property(__get_db_id, __set_db_id)
    def db_add_id(self, id):
        self.__db_id = id
    def db_change_id(self, id):
        self.__db_id = id
    def db_delete_id(self, id):
        self.__db_id = None
    
    def __get_db_destinationId(self):
        return self.__db_destinationId
    def __set_db_destinationId(self, destinationId):
        self.__db_destinationId = destinationId
    db_destinationId = property(__get_db_destinationId, __set_db_destinationId)
    def db_add_destinationId(self, destinationId):
        self.__db_destinationId = destinationId
    def db_change_destinationId(self, destinationId):
        self.__db_destinationId = destinationId
    def db_delete_destinationId(self, destinationId):
        self.__db_destinationId = None
    
    def __get_db_destinationModule(self):
        return self.__db_destinationModule
    def __set_db_destinationModule(self, destinationModule):
        self.__db_destinationModule = destinationModule
    db_destinationModule = property(__get_db_destinationModule, __set_db_destinationModule)
    def db_add_destinationModule(self, destinationModule):
        self.__db_destinationModule = destinationModule
    def db_change_destinationModule(self, destinationModule):
        self.__db_destinationModule = destinationModule
    def db_delete_destinationModule(self, destinationModule):
        self.__db_destinationModule = None
    
    def __get_db_destinationPort(self):
        return self.__db_destinationPort
    def __set_db_destinationPort(self, destinationPort):
        self.__db_destinationPort = destinationPort
    db_destinationPort = property(__get_db_destinationPort, __set_db_destinationPort)
    def db_add_destinationPort(self, destinationPort):
        self.__db_destinationPort = destinationPort
    def db_change_destinationPort(self, destinationPort):
        self.__db_destinationPort = destinationPort
    def db_delete_destinationPort(self, destinationPort):
        self.__db_destinationPort = None
    
    def __get_db_sourceId(self):
        return self.__db_sourceId
    def __set_db_sourceId(self, sourceId):
        self.__db_sourceId = sourceId
    db_sourceId = property(__get_db_sourceId, __set_db_sourceId)
    def db_add_sourceId(self, sourceId):
        self.__db_sourceId = sourceId
    def db_change_sourceId(self, sourceId):
        self.__db_sourceId = sourceId
    def db_delete_sourceId(self, sourceId):
        self.__db_sourceId = None
    
    def __get_db_sourceModule(self):
        return self.__db_sourceModule
    def __set_db_sourceModule(self, sourceModule):
        self.__db_sourceModule = sourceModule
    db_sourceModule = property(__get_db_sourceModule, __set_db_sourceModule)
    def db_add_sourceModule(self, sourceModule):
        self.__db_sourceModule = sourceModule
    def db_change_sourceModule(self, sourceModule):
        self.__db_sourceModule = sourceModule
    def db_delete_sourceModule(self, sourceModule):
        self.__db_sourceModule = None
    
    def __get_db_sourcePort(self):
        return self.__db_sourcePort
    def __set_db_sourcePort(self, sourcePort):
        self.__db_sourcePort = sourcePort
    db_sourcePort = property(__get_db_sourcePort, __set_db_sourcePort)
    def db_add_sourcePort(self, sourcePort):
        self.__db_sourcePort = sourcePort
    def db_change_sourcePort(self, sourcePort):
        self.__db_sourcePort = sourcePort
    def db_delete_sourcePort(self, sourcePort):
        self.__db_sourcePort = None
    
    def getPrimaryKey(self):
        return self.__db_id

"""generated automatically by auto_dao.py"""

class DBMoveModule(object):

    vtType = 'moveModule'

    def __init__(self, id=None, dx=None, dy=None):
        self.__db_id = id
        self.__db_dx = dx
        self.__db_dy = dy
    def __get_db_id(self):
        return self.__db_id
    def __set_db_id(self, id):
        self.__db_id = id
    db_id = property(__get_db_id, __set_db_id)
    def db_add_id(self, id):
        self.__db_id = id
    def db_change_id(self, id):
        self.__db_id = id
    def db_delete_id(self, id):
        self.__db_id = None
    
    def __get_db_dx(self):
        return self.__db_dx
    def __set_db_dx(self, dx):
        self.__db_dx = dx
    db_dx = property(__get_db_dx, __set_db_dx)
    def db_add_dx(self, dx):
        self.__db_dx = dx
    def db_change_dx(self, dx):
        self.__db_dx = dx
    def db_delete_dx(self, dx):
        self.__db_dx = None
    
    def __get_db_dy(self):
        return self.__db_dy
    def __set_db_dy(self, dy):
        self.__db_dy = dy
    db_dy = property(__get_db_dy, __set_db_dy)
    def db_add_dy(self, dy):
        self.__db_dy = dy
    def db_change_dy(self, dy):
        self.__db_dy = dy
    def db_delete_dy(self, dy):
        self.__db_dy = None
    
    def getPrimaryKey(self):
        return self.__db_id

"""generated automatically by auto_dao.py"""

class DBVistrail(object):

    vtType = 'vistrail'

    def __init__(self, version=None, actions=None, tags=None):
        self.__db_version = version
        if actions is None:
            self.__db_actions = {}
        else:
            self.__db_actions = actions
        if tags is None:
            self.__db_tags = {}
        else:
            self.__db_tags = tags
    def __get_db_version(self):
        return self.__db_version
    def __set_db_version(self, version):
        self.__db_version = version
    db_version = property(__get_db_version, __set_db_version)
    def db_add_version(self, version):
        self.__db_version = version
    def db_change_version(self, version):
        self.__db_version = version
    def db_delete_version(self, version):
        self.__db_version = None
    
    def __get_db_actions(self):
        return self.__db_actions
    def __set_db_actions(self, actions):
        self.__db_actions = actions
    db_actions = property(__get_db_actions, __set_db_actions)
    def db_get_actions(self):
        return self.__db_actions.values()
    def db_add_action(self, action):
        self.__db_actions[action.db_time] = action
    def db_change_action(self, action):
        self.__db_actions[action.db_time] = action
    def db_delete_action(self, action):
        del self.__db_actions[action.db_time]
    def db_get_action(self, key):
        if self.__db_actions.has_key(key):
            return self.__db_actions[key]
        return None
    
    def __get_db_tags(self):
        return self.__db_tags
    def __set_db_tags(self, tags):
        self.__db_tags = tags
    db_tags = property(__get_db_tags, __set_db_tags)
    def db_get_tags(self):
        return self.__db_tags.values()
    def db_add_tag(self, tag):
        self.__db_tags[tag.db_time] = tag
    def db_change_tag(self, tag):
        self.__db_tags[tag.db_time] = tag
    def db_delete_tag(self, tag):
        del self.__db_tags[tag.db_time]
    def db_get_tag(self, key):
        if self.__db_tags.has_key(key):
            return self.__db_tags[key]
        return None
    
    def getPrimaryKey(self):
        return self.__db_version

"""generated automatically by auto_dao.py"""

class DBChangeAnnotation(object):

    vtType = 'changeAnnotation'

    def __init__(self, moduleId=None, key=None, value=None):
        self.__db_moduleId = moduleId
        self.__db_key = key
        self.__db_value = value
    def __get_db_moduleId(self):
        return self.__db_moduleId
    def __set_db_moduleId(self, moduleId):
        self.__db_moduleId = moduleId
    db_moduleId = property(__get_db_moduleId, __set_db_moduleId)
    def db_add_moduleId(self, moduleId):
        self.__db_moduleId = moduleId
    def db_change_moduleId(self, moduleId):
        self.__db_moduleId = moduleId
    def db_delete_moduleId(self, moduleId):
        self.__db_moduleId = None
    
    def __get_db_key(self):
        return self.__db_key
    def __set_db_key(self, key):
        self.__db_key = key
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
    db_value = property(__get_db_value, __set_db_value)
    def db_add_value(self, value):
        self.__db_value = value
    def db_change_value(self, value):
        self.__db_value = value
    def db_delete_value(self, value):
        self.__db_value = None
    
    def getPrimaryKey(self):
        return self.__db_moduleId

