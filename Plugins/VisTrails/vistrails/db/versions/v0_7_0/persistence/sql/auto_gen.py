
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

from sql_dao import SQLDAO
from db.versions.v0_7_0.domain import *

class DBPortSpecSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props):
        columns = ['id', 'name', 'type', 'spec', 'parent_type', 'vt_id', 'parent_id']
        table = 'port_spec'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            name = self.convertFromDB(row[1], 'str', 'varchar(22)')
            type = self.convertFromDB(row[2], 'str', 'varchar(255)')
            spec = self.convertFromDB(row[3], 'str', 'varchar(255)')
            parentType = self.convertFromDB(row[4], 'str', 'char(16)')
            vistrailId = self.convertFromDB(row[5], 'long', 'int')
            parent = self.convertFromDB(row[6], 'long', 'long')
            
            portSpec = DBPortSpec(name=name,
                                  type=type,
                                  spec=spec,
                                  id=id)
            portSpec.db_parentType = parentType
            portSpec.db_vistrailId = vistrailId
            portSpec.db_parent = parent
            portSpec.is_dirty = False
            res[('portSpec', id)] = portSpec

        return res

    def from_sql_fast(self, obj, all_objects):
        if obj.db_parentType == 'module':
            p = all_objects[('module', obj.db_parent)]
            p.db_add_portSpec(obj)
        elif obj.db_parentType == 'add':
            p = all_objects[('add', obj.db_parent)]
            p.db_add_data(obj)
        elif obj.db_parentType == 'change':
            p = all_objects[('change', obj.db_parent)]
            p.db_add_data(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'name', 'type', 'spec', 'parent_type', 'vt_id', 'parent_id']
        table = 'port_spec'
        whereMap = {}
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        columnMap = {}
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            columnMap['id'] = \
                self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_name') and obj.db_name is not None:
            columnMap['name'] = \
                self.convertToDB(obj.db_name, 'str', 'varchar(22)')
        if hasattr(obj, 'db_type') and obj.db_type is not None:
            columnMap['type'] = \
                self.convertToDB(obj.db_type, 'str', 'varchar(255)')
        if hasattr(obj, 'db_spec') and obj.db_spec is not None:
            columnMap['spec'] = \
                self.convertToDB(obj.db_spec, 'str', 'varchar(255)')
        if hasattr(obj, 'db_parentType') and obj.db_parentType is not None:
            columnMap['parent_type'] = \
                self.convertToDB(obj.db_parentType, 'str', 'char(16)')
        if hasattr(obj, 'db_vistrailId') and obj.db_vistrailId is not None:
            columnMap['vt_id'] = \
                self.convertToDB(obj.db_vistrailId, 'long', 'int')
        if hasattr(obj, 'db_parent') and obj.db_parent is not None:
            columnMap['parent_id'] = \
                self.convertToDB(obj.db_parent, 'long', 'long')
        columnMap.update(global_props)

        if obj.is_new:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        pass
        
    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['name', 'type', 'spec', 'id']
        table = 'port_spec'
        whereMap = {}
        orderBy = 'id'

        if id is not None:
            keyStr = self.convertToDB(id, 'long', 'int')
            whereMap['id'] = keyStr
        elif foreignKey is not None:
            whereMap.update(foreignKey)
        elif globalProps is None:
            print '***ERROR: need to specify id or foreign key info'
        if globalProps is not None:
            whereMap.update(globalProps)
        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        list = []
        for row in data:
            name = self.convertFromDB(row[0], 'str', 'varchar(22)')
            type = self.convertFromDB(row[1], 'str', 'varchar(255)')
            spec = self.convertFromDB(row[2], 'str', 'varchar(255)')
            id = self.convertFromDB(row[3], 'long', 'int')
            keyStr = self.convertToDB(id,'long','int')

            portSpec = DBPortSpec(id=id,
                                  name=name,
                                  type=type,
                                  spec=spec)
            portSpec.is_dirty = False
            list.append(portSpec)

        return list

    def toSQL(self, db, obj, foreignKey=None, globalProps=None):
        keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if obj.is_dirty:
            columns = ['id']
            table = 'port_spec'
            whereMap = {}
            columnMap = {}

            whereMap['id'] = keyStr
            if globalProps is not None:
                whereMap.update(globalProps)
            if obj.db_name is not None:
                columnMap['name'] = \
                    self.convertToDB(obj.db_name, 'str', 'varchar(22)')
            if obj.db_type is not None:
                columnMap['type'] = \
                    self.convertToDB(obj.db_type, 'str', 'varchar(255)')
            if obj.db_spec is not None:
                columnMap['spec'] = \
                    self.convertToDB(obj.db_spec, 'str', 'varchar(255)')
            if foreignKey is not None:
                columnMap.update(foreignKey)

            dbCommand = self.createSQLSelect(table, columns, whereMap)
            data = self.executeSQL(db, dbCommand, True)
            if len(data) <= 0:
                columnMap['id'] = keyStr
                if globalProps is not None:
                    columnMap.update(globalProps)
                dbCommand = self.createSQLInsert(table, columnMap)
            else:
                dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
            lastId = self.executeSQL(db, dbCommand, False)
        


class DBModuleSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props):
        columns = ['id', 'cache', 'abstraction', 'name', 'package', 'version', 'parent_type', 'vt_id', 'parent_id']
        table = 'module'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            cache = self.convertFromDB(row[1], 'int', 'int')
            abstraction = self.convertFromDB(row[2], 'long', 'int')
            name = self.convertFromDB(row[3], 'str', 'varchar(255)')
            package = self.convertFromDB(row[4], 'str', 'varchar(511)')
            version = self.convertFromDB(row[5], 'str', 'varchar(255)')
            parentType = self.convertFromDB(row[6], 'str', 'char(16)')
            vistrailId = self.convertFromDB(row[7], 'long', 'int')
            parent = self.convertFromDB(row[8], 'long', 'long')
            
            module = DBModule(cache=cache,
                              abstraction=abstraction,
                              name=name,
                              package=package,
                              version=version,
                              id=id)
            module.db_parentType = parentType
            module.db_vistrailId = vistrailId
            module.db_parent = parent
            module.is_dirty = False
            res[('module', id)] = module

        return res

    def from_sql_fast(self, obj, all_objects):
        if obj.db_parentType == 'workflow':
            p = all_objects[('workflow', obj.db_parent)]
            p.db_add_module(obj)
        elif obj.db_parentType == 'add':
            p = all_objects[('add', obj.db_parent)]
            p.db_add_data(obj)
        elif obj.db_parentType == 'change':
            p = all_objects[('change', obj.db_parent)]
            p.db_add_data(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'cache', 'abstraction', 'name', 'package', 'version', 'parent_type', 'vt_id', 'parent_id']
        table = 'module'
        whereMap = {}
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        columnMap = {}
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            columnMap['id'] = \
                self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_cache') and obj.db_cache is not None:
            columnMap['cache'] = \
                self.convertToDB(obj.db_cache, 'int', 'int')
        if hasattr(obj, 'db_abstraction') and obj.db_abstraction is not None:
            columnMap['abstraction'] = \
                self.convertToDB(obj.db_abstraction, 'long', 'int')
        if hasattr(obj, 'db_name') and obj.db_name is not None:
            columnMap['name'] = \
                self.convertToDB(obj.db_name, 'str', 'varchar(255)')
        if hasattr(obj, 'db_package') and obj.db_package is not None:
            columnMap['package'] = \
                self.convertToDB(obj.db_package, 'str', 'varchar(511)')
        if hasattr(obj, 'db_version') and obj.db_version is not None:
            columnMap['version'] = \
                self.convertToDB(obj.db_version, 'str', 'varchar(255)')
        if hasattr(obj, 'db_parentType') and obj.db_parentType is not None:
            columnMap['parent_type'] = \
                self.convertToDB(obj.db_parentType, 'str', 'char(16)')
        if hasattr(obj, 'db_vistrailId') and obj.db_vistrailId is not None:
            columnMap['vt_id'] = \
                self.convertToDB(obj.db_vistrailId, 'long', 'int')
        if hasattr(obj, 'db_parent') and obj.db_parent is not None:
            columnMap['parent_id'] = \
                self.convertToDB(obj.db_parent, 'long', 'long')
        columnMap.update(global_props)

        if obj.is_new:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        if obj.db_location is not None:
            child = obj.db_location
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        for child in obj.db_functions:
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        for child in obj.db_annotations:
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        for child in obj.db_portSpecs:
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        
    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['cache', 'abstraction', 'name', 'package', 'version', 'id']
        table = 'module'
        whereMap = {}
        orderBy = 'id'

        if id is not None:
            keyStr = self.convertToDB(id, 'long', 'int')
            whereMap['id'] = keyStr
        elif foreignKey is not None:
            whereMap.update(foreignKey)
        elif globalProps is None:
            print '***ERROR: need to specify id or foreign key info'
        if globalProps is not None:
            whereMap.update(globalProps)
        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        list = []
        for row in data:
            cache = self.convertFromDB(row[0], 'int', 'int')
            abstraction = self.convertFromDB(row[1], 'long', 'int')
            name = self.convertFromDB(row[2], 'str', 'varchar(255)')
            package = self.convertFromDB(row[3], 'str', 'varchar(511)')
            version = self.convertFromDB(row[4], 'str', 'varchar(255)')
            id = self.convertFromDB(row[5], 'long', 'int')
            keyStr = self.convertToDB(id,'long','int')

            discStr = self.convertToDB('module','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            res = self.getDao('location').fromSQL(db, None, foreignKey, globalProps)
            if len(res) > 0:
                location = res[0]
            else:
                location = None
            
            discStr = self.convertToDB('module','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            res = self.getDao('function').fromSQL(db, None, foreignKey, globalProps)
            functions = res
            
            discStr = self.convertToDB('module','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            res = self.getDao('annotation').fromSQL(db, None, foreignKey, globalProps)
            annotations = res
            
            discStr = self.convertToDB('module','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            res = self.getDao('portSpec').fromSQL(db, None, foreignKey, globalProps)
            portSpecs = res
            
            module = DBModule(id=id,
                              cache=cache,
                              abstraction=abstraction,
                              name=name,
                              package=package,
                              version=version,
                              location=location,
                              functions=functions,
                              annotations=annotations,
                              portSpecs=portSpecs)
            module.is_dirty = False
            list.append(module)

        return list

    def toSQL(self, db, obj, foreignKey=None, globalProps=None):
        keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if obj.is_dirty:
            columns = ['id']
            table = 'module'
            whereMap = {}
            columnMap = {}

            whereMap['id'] = keyStr
            if globalProps is not None:
                whereMap.update(globalProps)
            if obj.db_cache is not None:
                columnMap['cache'] = \
                    self.convertToDB(obj.db_cache, 'int', 'int')
            if obj.db_abstraction is not None:
                columnMap['abstraction'] = \
                    self.convertToDB(obj.db_abstraction, 'long', 'int')
            if obj.db_name is not None:
                columnMap['name'] = \
                    self.convertToDB(obj.db_name, 'str', 'varchar(255)')
            if obj.db_package is not None:
                columnMap['package'] = \
                    self.convertToDB(obj.db_package, 'str', 'varchar(511)')
            if obj.db_version is not None:
                columnMap['version'] = \
                    self.convertToDB(obj.db_version, 'str', 'varchar(255)')
            if foreignKey is not None:
                columnMap.update(foreignKey)

            dbCommand = self.createSQLSelect(table, columns, whereMap)
            data = self.executeSQL(db, dbCommand, True)
            if len(data) <= 0:
                columnMap['id'] = keyStr
                if globalProps is not None:
                    columnMap.update(globalProps)
                dbCommand = self.createSQLInsert(table, columnMap)
            else:
                dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
            lastId = self.executeSQL(db, dbCommand, False)
        

        discStr = self.convertToDB('module','str','char(16)')
        foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
        child = obj.db_location
        if child is not None:
            self.getDao('location').toSQL(db, child, foreignKey, globalProps)
        
        discStr = self.convertToDB('module','str','char(16)')
        foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
        for child in obj.db_functions:
            self.getDao('function').toSQL(db, child, foreignKey, globalProps)
        
        discStr = self.convertToDB('module','str','char(16)')
        foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
        for child in obj.db_annotations:
            self.getDao('annotation').toSQL(db, child, foreignKey, globalProps)
        
        discStr = self.convertToDB('module','str','char(16)')
        foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
        for child in obj.db_portSpecs:
            self.getDao('portSpec').toSQL(db, child, foreignKey, globalProps)
        

class DBTagSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props):
        columns = ['id', 'name', 'parent_type', 'vt_id', 'parent_id']
        table = 'tag'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            name = self.convertFromDB(row[1], 'str', 'varchar(255)')
            parentType = self.convertFromDB(row[2], 'str', 'char(16)')
            vistrailId = self.convertFromDB(row[3], 'long', 'int')
            parent = self.convertFromDB(row[4], 'long', 'long')
            
            tag = DBTag(name=name,
                        id=id)
            tag.db_parentType = parentType
            tag.db_vistrailId = vistrailId
            tag.db_parent = parent
            tag.is_dirty = False
            res[('tag', id)] = tag

        return res

    def from_sql_fast(self, obj, all_objects):
        if obj.db_parentType == 'vistrail':
            p = all_objects[('vistrail', obj.db_parent)]
            p.db_add_tag(obj)
        elif obj.db_parentType == 'abstraction':
            p = all_objects[('abstraction', obj.db_parent)]
            p.db_add_tag(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'name', 'parent_type', 'vt_id', 'parent_id']
        table = 'tag'
        whereMap = {}
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        columnMap = {}
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            columnMap['id'] = \
                self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_name') and obj.db_name is not None:
            columnMap['name'] = \
                self.convertToDB(obj.db_name, 'str', 'varchar(255)')
        if hasattr(obj, 'db_parentType') and obj.db_parentType is not None:
            columnMap['parent_type'] = \
                self.convertToDB(obj.db_parentType, 'str', 'char(16)')
        if hasattr(obj, 'db_vistrailId') and obj.db_vistrailId is not None:
            columnMap['vt_id'] = \
                self.convertToDB(obj.db_vistrailId, 'long', 'int')
        if hasattr(obj, 'db_parent') and obj.db_parent is not None:
            columnMap['parent_id'] = \
                self.convertToDB(obj.db_parent, 'long', 'long')
        columnMap.update(global_props)

        if obj.is_new:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        pass
        
    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['name', 'id']
        table = 'tag'
        whereMap = {}
        orderBy = 'id'

        if id is not None:
            keyStr = self.convertToDB(id, 'long', 'int')
            whereMap['id'] = keyStr
        elif foreignKey is not None:
            whereMap.update(foreignKey)
        elif globalProps is None:
            print '***ERROR: need to specify id or foreign key info'
        if globalProps is not None:
            whereMap.update(globalProps)
        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        list = []
        for row in data:
            name = self.convertFromDB(row[0], 'str', 'varchar(255)')
            id = self.convertFromDB(row[1], 'long', 'int')
            keyStr = self.convertToDB(id,'long','int')

            tag = DBTag(id=id,
                        name=name)
            tag.is_dirty = False
            list.append(tag)

        return list

    def toSQL(self, db, obj, foreignKey=None, globalProps=None):
        keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if obj.is_dirty:
            columns = ['id']
            table = 'tag'
            whereMap = {}
            columnMap = {}

            whereMap['id'] = keyStr
            if globalProps is not None:
                whereMap.update(globalProps)
            if obj.db_name is not None:
                columnMap['name'] = \
                    self.convertToDB(obj.db_name, 'str', 'varchar(255)')
            if foreignKey is not None:
                columnMap.update(foreignKey)

            dbCommand = self.createSQLSelect(table, columns, whereMap)
            data = self.executeSQL(db, dbCommand, True)
            if len(data) <= 0:
                columnMap['id'] = keyStr
                if globalProps is not None:
                    columnMap.update(globalProps)
                dbCommand = self.createSQLInsert(table, columnMap)
            else:
                dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
            lastId = self.executeSQL(db, dbCommand, False)
        


class DBPortSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props):
        columns = ['id', 'type', 'moduleId', 'moduleName', 'name', 'spec', 'parent_type', 'vt_id', 'parent_id']
        table = 'port'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            type = self.convertFromDB(row[1], 'str', 'varchar(255)')
            moduleId = self.convertFromDB(row[2], 'long', 'int')
            moduleName = self.convertFromDB(row[3], 'str', 'varchar(255)')
            name = self.convertFromDB(row[4], 'str', 'varchar(255)')
            spec = self.convertFromDB(row[5], 'str', 'varchar(4095)')
            parentType = self.convertFromDB(row[6], 'str', 'char(16)')
            vistrailId = self.convertFromDB(row[7], 'long', 'int')
            parent = self.convertFromDB(row[8], 'long', 'long')
            
            port = DBPort(type=type,
                          moduleId=moduleId,
                          moduleName=moduleName,
                          name=name,
                          spec=spec,
                          id=id)
            port.db_parentType = parentType
            port.db_vistrailId = vistrailId
            port.db_parent = parent
            port.is_dirty = False
            res[('port', id)] = port

        return res

    def from_sql_fast(self, obj, all_objects):
        if obj.db_parentType == 'connection':
            p = all_objects[('connection', obj.db_parent)]
            p.db_add_port(obj)
        elif obj.db_parentType == 'add':
            p = all_objects[('add', obj.db_parent)]
            p.db_add_data(obj)
        elif obj.db_parentType == 'change':
            p = all_objects[('change', obj.db_parent)]
            p.db_add_data(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'type', 'moduleId', 'moduleName', 'name', 'spec', 'parent_type', 'vt_id', 'parent_id']
        table = 'port'
        whereMap = {}
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        columnMap = {}
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            columnMap['id'] = \
                self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_type') and obj.db_type is not None:
            columnMap['type'] = \
                self.convertToDB(obj.db_type, 'str', 'varchar(255)')
        if hasattr(obj, 'db_moduleId') and obj.db_moduleId is not None:
            columnMap['moduleId'] = \
                self.convertToDB(obj.db_moduleId, 'long', 'int')
        if hasattr(obj, 'db_moduleName') and obj.db_moduleName is not None:
            columnMap['moduleName'] = \
                self.convertToDB(obj.db_moduleName, 'str', 'varchar(255)')
        if hasattr(obj, 'db_name') and obj.db_name is not None:
            columnMap['name'] = \
                self.convertToDB(obj.db_name, 'str', 'varchar(255)')
        if hasattr(obj, 'db_spec') and obj.db_spec is not None:
            columnMap['spec'] = \
                self.convertToDB(obj.db_spec, 'str', 'varchar(4095)')
        if hasattr(obj, 'db_parentType') and obj.db_parentType is not None:
            columnMap['parent_type'] = \
                self.convertToDB(obj.db_parentType, 'str', 'char(16)')
        if hasattr(obj, 'db_vistrailId') and obj.db_vistrailId is not None:
            columnMap['vt_id'] = \
                self.convertToDB(obj.db_vistrailId, 'long', 'int')
        if hasattr(obj, 'db_parent') and obj.db_parent is not None:
            columnMap['parent_id'] = \
                self.convertToDB(obj.db_parent, 'long', 'long')
        columnMap.update(global_props)

        if obj.is_new:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        pass
        
    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['type', 'moduleId', 'moduleName', 'name', 'spec', 'id']
        table = 'port'
        whereMap = {}
        orderBy = 'id'

        if id is not None:
            keyStr = self.convertToDB(id, 'long', 'int')
            whereMap['id'] = keyStr
        elif foreignKey is not None:
            whereMap.update(foreignKey)
        elif globalProps is None:
            print '***ERROR: need to specify id or foreign key info'
        if globalProps is not None:
            whereMap.update(globalProps)
        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        list = []
        for row in data:
            type = self.convertFromDB(row[0], 'str', 'varchar(255)')
            moduleId = self.convertFromDB(row[1], 'long', 'int')
            moduleName = self.convertFromDB(row[2], 'str', 'varchar(255)')
            name = self.convertFromDB(row[3], 'str', 'varchar(255)')
            spec = self.convertFromDB(row[4], 'str', 'varchar(4095)')
            id = self.convertFromDB(row[5], 'long', 'int')
            keyStr = self.convertToDB(id,'long','int')

            port = DBPort(id=id,
                          type=type,
                          moduleId=moduleId,
                          moduleName=moduleName,
                          name=name,
                          spec=spec)
            port.is_dirty = False
            list.append(port)

        return list

    def toSQL(self, db, obj, foreignKey=None, globalProps=None):
        keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if obj.is_dirty:
            columns = ['id']
            table = 'port'
            whereMap = {}
            columnMap = {}

            whereMap['id'] = keyStr
            if globalProps is not None:
                whereMap.update(globalProps)
            if obj.db_type is not None:
                columnMap['type'] = \
                    self.convertToDB(obj.db_type, 'str', 'varchar(255)')
            if obj.db_moduleId is not None:
                columnMap['moduleId'] = \
                    self.convertToDB(obj.db_moduleId, 'long', 'int')
            if obj.db_moduleName is not None:
                columnMap['moduleName'] = \
                    self.convertToDB(obj.db_moduleName, 'str', 'varchar(255)')
            if obj.db_name is not None:
                columnMap['name'] = \
                    self.convertToDB(obj.db_name, 'str', 'varchar(255)')
            if obj.db_spec is not None:
                columnMap['spec'] = \
                    self.convertToDB(obj.db_spec, 'str', 'varchar(4095)')
            if foreignKey is not None:
                columnMap.update(foreignKey)

            dbCommand = self.createSQLSelect(table, columns, whereMap)
            data = self.executeSQL(db, dbCommand, True)
            if len(data) <= 0:
                columnMap['id'] = keyStr
                if globalProps is not None:
                    columnMap.update(globalProps)
                dbCommand = self.createSQLInsert(table, columnMap)
            else:
                dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
            lastId = self.executeSQL(db, dbCommand, False)
        


class DBLogSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props):
        columns = ['id', 'vt_id']
        table = 'log_tbl'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            vistrailId = self.convertFromDB(row[1], 'long', 'int')
            
            log = DBLog(id=id)
            log.db_vistrailId = vistrailId
            log.is_dirty = False
            res[('log', id)] = log

        return res

    def from_sql_fast(self, obj, all_objects):
        pass
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'vt_id']
        table = 'log_tbl'
        whereMap = {}
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        columnMap = {}
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            columnMap['id'] = \
                self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_vistrailId') and obj.db_vistrailId is not None:
            columnMap['vt_id'] = \
                self.convertToDB(obj.db_vistrailId, 'long', 'int')
        columnMap.update(global_props)

        if obj.is_new:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        for child in obj.db_workflow_execs:
            child.db_log = obj.db_id
        for child in obj.db_machines:
            child.db_log = obj.db_id
        
    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['id']
        table = 'log_tbl'
        whereMap = {}
        orderBy = 'id'

        if id is not None:
            keyStr = self.convertToDB(id, 'long', 'int')
            whereMap['id'] = keyStr
        elif foreignKey is not None:
            whereMap.update(foreignKey)
        elif globalProps is None:
            print '***ERROR: need to specify id or foreign key info'
        if globalProps is not None:
            whereMap.update(globalProps)
        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        list = []
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            keyStr = self.convertToDB(id,'long','int')

            foreignKey = {'log_id': keyStr}
            res = self.getDao('workflow_exec').fromSQL(db, None, foreignKey, globalProps)
            workflow_execs = res
            
            foreignKey = {'log_id': keyStr}
            res = self.getDao('machine').fromSQL(db, None, foreignKey, globalProps)
            machines = res
            
            log = DBLog(id=id,
                        workflow_execs=workflow_execs,
                        machines=machines)
            log.is_dirty = False
            list.append(log)

        return list

    def toSQL(self, db, obj, foreignKey=None, globalProps=None):
        keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if obj.is_dirty:
            columns = ['id']
            table = 'log_tbl'
            whereMap = {}
            columnMap = {}

            whereMap['id'] = keyStr
            if globalProps is not None:
                whereMap.update(globalProps)
            if foreignKey is not None:
                columnMap.update(foreignKey)

            dbCommand = self.createSQLSelect(table, columns, whereMap)
            data = self.executeSQL(db, dbCommand, True)
            if len(data) <= 0:
                columnMap['id'] = keyStr
                if globalProps is not None:
                    columnMap.update(globalProps)
                dbCommand = self.createSQLInsert(table, columnMap)
            else:
                dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
            lastId = self.executeSQL(db, dbCommand, False)
        

        foreignKey = {'log_id': keyStr}
        for child in obj.db_workflow_execs:
            self.getDao('workflow_exec').toSQL(db, child, foreignKey, globalProps)
        
        foreignKey = {'log_id': keyStr}
        for child in obj.db_machines:
            self.getDao('machine').toSQL(db, child, foreignKey, globalProps)
        

class DBMachineSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props):
        columns = ['id', 'name', 'os', 'architecture', 'processor', 'ram', 'vt_id', 'log_id', 'module_exec_id']
        table = 'machine'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            name = self.convertFromDB(row[1], 'str', 'varchar(255)')
            os = self.convertFromDB(row[2], 'str', 'varchar(255)')
            architecture = self.convertFromDB(row[3], 'str', 'varchar(255)')
            processor = self.convertFromDB(row[4], 'str', 'varchar(255)')
            ram = self.convertFromDB(row[5], 'int', 'int')
            vistrailId = self.convertFromDB(row[6], 'long', 'int')
            log = self.convertFromDB(row[7], 'long', 'int')
            module_execs = self.convertFromDB(row[8], 'long', 'int')
            
            machine = DBMachine(name=name,
                                os=os,
                                architecture=architecture,
                                processor=processor,
                                ram=ram,
                                id=id)
            machine.db_vistrailId = vistrailId
            machine.db_log = log
            machine.is_dirty = False
            res[('machine', id)] = machine

        return res

    def from_sql_fast(self, obj, all_objects):
        p = all_objects[('log', obj.db_log)]
        p.db_add_machine(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'name', 'os', 'architecture', 'processor', 'ram', 'vt_id', 'log_id', 'module_exec_id']
        table = 'machine'
        whereMap = {}
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        columnMap = {}
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            columnMap['id'] = \
                self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_name') and obj.db_name is not None:
            columnMap['name'] = \
                self.convertToDB(obj.db_name, 'str', 'varchar(255)')
        if hasattr(obj, 'db_os') and obj.db_os is not None:
            columnMap['os'] = \
                self.convertToDB(obj.db_os, 'str', 'varchar(255)')
        if hasattr(obj, 'db_architecture') and obj.db_architecture is not None:
            columnMap['architecture'] = \
                self.convertToDB(obj.db_architecture, 'str', 'varchar(255)')
        if hasattr(obj, 'db_processor') and obj.db_processor is not None:
            columnMap['processor'] = \
                self.convertToDB(obj.db_processor, 'str', 'varchar(255)')
        if hasattr(obj, 'db_ram') and obj.db_ram is not None:
            columnMap['ram'] = \
                self.convertToDB(obj.db_ram, 'int', 'int')
        if hasattr(obj, 'db_vistrailId') and obj.db_vistrailId is not None:
            columnMap['vt_id'] = \
                self.convertToDB(obj.db_vistrailId, 'long', 'int')
        if hasattr(obj, 'db_log') and obj.db_log is not None:
            columnMap['log_id'] = \
                self.convertToDB(obj.db_log, 'long', 'int')
        if hasattr(obj, 'db_module_exec') and obj.db_module_exec is not None:
            columnMap['module_exec_id'] = \
                self.convertToDB(obj.db_module_execs, 'long', 'int')
        columnMap.update(global_props)

        if obj.is_new:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        for child in obj.db_module_execs:
            child.db_machine_id = obj.db_id
        
    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['name', 'os', 'architecture', 'processor', 'ram', 'id']
        table = 'machine'
        whereMap = {}
        orderBy = 'id'

        if id is not None:
            keyStr = self.convertToDB(id, 'long', 'int')
            whereMap['id'] = keyStr
        elif foreignKey is not None:
            whereMap.update(foreignKey)
        elif globalProps is None:
            print '***ERROR: need to specify id or foreign key info'
        if globalProps is not None:
            whereMap.update(globalProps)
        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        list = []
        for row in data:
            name = self.convertFromDB(row[0], 'str', 'varchar(255)')
            os = self.convertFromDB(row[1], 'str', 'varchar(255)')
            architecture = self.convertFromDB(row[2], 'str', 'varchar(255)')
            processor = self.convertFromDB(row[3], 'str', 'varchar(255)')
            ram = self.convertFromDB(row[4], 'int', 'int')
            id = self.convertFromDB(row[5], 'long', 'int')
            keyStr = self.convertToDB(id,'long','int')

            foreignKey = {'machine_id': keyStr}
            res = self.getDao('module_exec').fromSQL(db, None, foreignKey, globalProps)
            module_execs = res
            
            machine = DBMachine(id=id,
                                name=name,
                                os=os,
                                architecture=architecture,
                                processor=processor,
                                ram=ram,
                                module_execs=module_execs)
            machine.is_dirty = False
            list.append(machine)

        return list

    def toSQL(self, db, obj, foreignKey=None, globalProps=None):
        keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if obj.is_dirty:
            columns = ['id']
            table = 'machine'
            whereMap = {}
            columnMap = {}

            whereMap['id'] = keyStr
            if globalProps is not None:
                whereMap.update(globalProps)
            if obj.db_name is not None:
                columnMap['name'] = \
                    self.convertToDB(obj.db_name, 'str', 'varchar(255)')
            if obj.db_os is not None:
                columnMap['os'] = \
                    self.convertToDB(obj.db_os, 'str', 'varchar(255)')
            if obj.db_architecture is not None:
                columnMap['architecture'] = \
                    self.convertToDB(obj.db_architecture, 'str', 'varchar(255)')
            if obj.db_processor is not None:
                columnMap['processor'] = \
                    self.convertToDB(obj.db_processor, 'str', 'varchar(255)')
            if obj.db_ram is not None:
                columnMap['ram'] = \
                    self.convertToDB(obj.db_ram, 'int', 'int')
            if foreignKey is not None:
                columnMap.update(foreignKey)

            dbCommand = self.createSQLSelect(table, columns, whereMap)
            data = self.executeSQL(db, dbCommand, True)
            if len(data) <= 0:
                columnMap['id'] = keyStr
                if globalProps is not None:
                    columnMap.update(globalProps)
                dbCommand = self.createSQLInsert(table, columnMap)
            else:
                dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
            lastId = self.executeSQL(db, dbCommand, False)
        

        foreignKey = {'machine_id': keyStr}
        for child in obj.db_module_execs:
            self.getDao('module_exec').toSQL(db, child, foreignKey, globalProps)
        

class DBAddSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props):
        columns = ['id', 'what', 'object_id', 'par_obj_id', 'par_obj_type', 'action_id', 'vt_id']
        table = 'add_tbl'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            what = self.convertFromDB(row[1], 'str', 'varchar(255)')
            objectId = self.convertFromDB(row[2], 'long', 'int')
            parentObjId = self.convertFromDB(row[3], 'long', 'int')
            parentObjType = self.convertFromDB(row[4], 'str', 'char(16)')
            action = self.convertFromDB(row[5], 'long', 'int')
            vistrailId = self.convertFromDB(row[6], 'long', 'int')
            
            add = DBAdd(what=what,
                        objectId=objectId,
                        parentObjId=parentObjId,
                        parentObjType=parentObjType,
                        id=id)
            add.db_action = action
            add.db_vistrailId = vistrailId
            add.is_dirty = False
            res[('add', id)] = add

        return res

    def from_sql_fast(self, obj, all_objects):
        p = all_objects[('action', obj.db_action)]
        p.db_add_operation(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'what', 'object_id', 'par_obj_id', 'par_obj_type', 'action_id', 'vt_id']
        table = 'add_tbl'
        whereMap = {}
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        columnMap = {}
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            columnMap['id'] = \
                self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_what') and obj.db_what is not None:
            columnMap['what'] = \
                self.convertToDB(obj.db_what, 'str', 'varchar(255)')
        if hasattr(obj, 'db_objectId') and obj.db_objectId is not None:
            columnMap['object_id'] = \
                self.convertToDB(obj.db_objectId, 'long', 'int')
        if hasattr(obj, 'db_parentObjId') and obj.db_parentObjId is not None:
            columnMap['par_obj_id'] = \
                self.convertToDB(obj.db_parentObjId, 'long', 'int')
        if hasattr(obj, 'db_parentObjType') and obj.db_parentObjType is not None:
            columnMap['par_obj_type'] = \
                self.convertToDB(obj.db_parentObjType, 'str', 'char(16)')
        if hasattr(obj, 'db_action') and obj.db_action is not None:
            columnMap['action_id'] = \
                self.convertToDB(obj.db_action, 'long', 'int')
        if hasattr(obj, 'db_vistrailId') and obj.db_vistrailId is not None:
            columnMap['vt_id'] = \
                self.convertToDB(obj.db_vistrailId, 'long', 'int')
        columnMap.update(global_props)

        if obj.is_new:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        if obj.db_data is not None:
            child = obj.db_data
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        
    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['what', 'object_id', 'par_obj_id', 'par_obj_type', 'id']
        table = 'add_tbl'
        whereMap = {}
        orderBy = 'id'

        if id is not None:
            keyStr = self.convertToDB(id, 'long', 'int')
            whereMap['id'] = keyStr
        elif foreignKey is not None:
            whereMap.update(foreignKey)
        elif globalProps is None:
            print '***ERROR: need to specify id or foreign key info'
        if globalProps is not None:
            whereMap.update(globalProps)
        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        list = []
        for row in data:
            what = self.convertFromDB(row[0], 'str', 'varchar(255)')
            objectId = self.convertFromDB(row[1], 'long', 'int')
            parentObjId = self.convertFromDB(row[2], 'long', 'int')
            parentObjType = self.convertFromDB(row[3], 'str', 'char(16)')
            id = self.convertFromDB(row[4], 'long', 'int')
            keyStr = self.convertToDB(id,'long','int')

            data = None
            if what == 'module':
                discStr = self.convertToDB('add','str','char(16)')
                foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
                res = self.getDao('module').fromSQL(db, None, foreignKey, globalProps)
                data = res[0]
            elif what == 'location':
                discStr = self.convertToDB('add','str','char(16)')
                foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
                res = self.getDao('location').fromSQL(db, None, foreignKey, globalProps)
                data = res[0]
            elif what == 'annotation':
                discStr = self.convertToDB('add','str','char(16)')
                foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
                res = self.getDao('annotation').fromSQL(db, None, foreignKey, globalProps)
                data = res[0]
            elif what == 'function':
                discStr = self.convertToDB('add','str','char(16)')
                foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
                res = self.getDao('function').fromSQL(db, None, foreignKey, globalProps)
                data = res[0]
            elif what == 'connection':
                discStr = self.convertToDB('add','str','char(16)')
                foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
                res = self.getDao('connection').fromSQL(db, None, foreignKey, globalProps)
                data = res[0]
            elif what == 'port':
                discStr = self.convertToDB('add','str','char(16)')
                foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
                res = self.getDao('port').fromSQL(db, None, foreignKey, globalProps)
                data = res[0]
            elif what == 'parameter':
                discStr = self.convertToDB('add','str','char(16)')
                foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
                res = self.getDao('parameter').fromSQL(db, None, foreignKey, globalProps)
                data = res[0]
            elif what == 'portSpec':
                discStr = self.convertToDB('add','str','char(16)')
                foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
                res = self.getDao('portSpec').fromSQL(db, None, foreignKey, globalProps)
                data = res[0]
            elif what == 'abstractionRef':
                discStr = self.convertToDB('add','str','char(16)')
                foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
                res = self.getDao('abstractionRef').fromSQL(db, None, foreignKey, globalProps)
                data = res[0]
            elif what == 'other':
                discStr = self.convertToDB('add','str','char(16)')
                foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
                res = self.getDao('other').fromSQL(db, None, foreignKey, globalProps)
                data = res[0]
            
            add = DBAdd(id=id,
                        what=what,
                        objectId=objectId,
                        parentObjId=parentObjId,
                        parentObjType=parentObjType,
                        data=data)
            add.is_dirty = False
            list.append(add)

        return list

    def toSQL(self, db, obj, foreignKey=None, globalProps=None):
        keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if obj.is_dirty:
            columns = ['id']
            table = 'add_tbl'
            whereMap = {}
            columnMap = {}

            whereMap['id'] = keyStr
            if globalProps is not None:
                whereMap.update(globalProps)
            if obj.db_what is not None:
                columnMap['what'] = \
                    self.convertToDB(obj.db_what, 'str', 'varchar(255)')
            if obj.db_objectId is not None:
                columnMap['object_id'] = \
                    self.convertToDB(obj.db_objectId, 'long', 'int')
            if obj.db_parentObjId is not None:
                columnMap['par_obj_id'] = \
                    self.convertToDB(obj.db_parentObjId, 'long', 'int')
            if obj.db_parentObjType is not None:
                columnMap['par_obj_type'] = \
                    self.convertToDB(obj.db_parentObjType, 'str', 'char(16)')
            if foreignKey is not None:
                columnMap.update(foreignKey)

            dbCommand = self.createSQLSelect(table, columns, whereMap)
            data = self.executeSQL(db, dbCommand, True)
            if len(data) <= 0:
                columnMap['id'] = keyStr
                if globalProps is not None:
                    columnMap.update(globalProps)
                dbCommand = self.createSQLInsert(table, columnMap)
            else:
                dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
            lastId = self.executeSQL(db, dbCommand, False)
        

        child = obj.db_data
        if child.vtType == 'module':
            discStr = self.convertToDB('add','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            self.getDao('module').toSQL(db, child, foreignKey, globalProps)
        elif child.vtType == 'location':
            discStr = self.convertToDB('add','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            self.getDao('location').toSQL(db, child, foreignKey, globalProps)
        elif child.vtType == 'annotation':
            discStr = self.convertToDB('add','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            self.getDao('annotation').toSQL(db, child, foreignKey, globalProps)
        elif child.vtType == 'function':
            discStr = self.convertToDB('add','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            self.getDao('function').toSQL(db, child, foreignKey, globalProps)
        elif child.vtType == 'connection':
            discStr = self.convertToDB('add','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            self.getDao('connection').toSQL(db, child, foreignKey, globalProps)
        elif child.vtType == 'port':
            discStr = self.convertToDB('add','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            self.getDao('port').toSQL(db, child, foreignKey, globalProps)
        elif child.vtType == 'parameter':
            discStr = self.convertToDB('add','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            self.getDao('parameter').toSQL(db, child, foreignKey, globalProps)
        elif child.vtType == 'portSpec':
            discStr = self.convertToDB('add','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            self.getDao('portSpec').toSQL(db, child, foreignKey, globalProps)
        elif child.vtType == 'abstractionRef':
            discStr = self.convertToDB('add','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            self.getDao('abstractionRef').toSQL(db, child, foreignKey, globalProps)
        elif child.vtType == 'other':
            discStr = self.convertToDB('add','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            self.getDao('other').toSQL(db, child, foreignKey, globalProps)
        

class DBOtherSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props):
        columns = ['id', 'okey', 'value', 'parent_type', 'vt_id', 'parent_id']
        table = 'other'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            key = self.convertFromDB(row[1], 'str', 'varchar(255)')
            value = self.convertFromDB(row[2], 'str', 'varchar(255)')
            parentType = self.convertFromDB(row[3], 'str', 'char(16)')
            vistrailId = self.convertFromDB(row[4], 'long', 'int')
            parent = self.convertFromDB(row[5], 'long', 'long')
            
            other = DBOther(key=key,
                            value=value,
                            id=id)
            other.db_parentType = parentType
            other.db_vistrailId = vistrailId
            other.db_parent = parent
            other.is_dirty = False
            res[('other', id)] = other

        return res

    def from_sql_fast(self, obj, all_objects):
        if obj.db_parentType == 'workflow':
            p = all_objects[('workflow', obj.db_parent)]
            p.db_add_other(obj)
        elif obj.db_parentType == 'add':
            p = all_objects[('add', obj.db_parent)]
            p.db_add_data(obj)
        elif obj.db_parentType == 'change':
            p = all_objects[('change', obj.db_parent)]
            p.db_add_data(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'okey', 'value', 'parent_type', 'vt_id', 'parent_id']
        table = 'other'
        whereMap = {}
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        columnMap = {}
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            columnMap['id'] = \
                self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_key') and obj.db_key is not None:
            columnMap['okey'] = \
                self.convertToDB(obj.db_key, 'str', 'varchar(255)')
        if hasattr(obj, 'db_value') and obj.db_value is not None:
            columnMap['value'] = \
                self.convertToDB(obj.db_value, 'str', 'varchar(255)')
        if hasattr(obj, 'db_parentType') and obj.db_parentType is not None:
            columnMap['parent_type'] = \
                self.convertToDB(obj.db_parentType, 'str', 'char(16)')
        if hasattr(obj, 'db_vistrailId') and obj.db_vistrailId is not None:
            columnMap['vt_id'] = \
                self.convertToDB(obj.db_vistrailId, 'long', 'int')
        if hasattr(obj, 'db_parent') and obj.db_parent is not None:
            columnMap['parent_id'] = \
                self.convertToDB(obj.db_parent, 'long', 'long')
        columnMap.update(global_props)

        if obj.is_new:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        pass
        
    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['okey', 'value', 'id']
        table = 'other'
        whereMap = {}
        orderBy = 'id'

        if id is not None:
            keyStr = self.convertToDB(id, 'long', 'int')
            whereMap['id'] = keyStr
        elif foreignKey is not None:
            whereMap.update(foreignKey)
        elif globalProps is None:
            print '***ERROR: need to specify id or foreign key info'
        if globalProps is not None:
            whereMap.update(globalProps)
        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        list = []
        for row in data:
            key = self.convertFromDB(row[0], 'str', 'varchar(255)')
            value = self.convertFromDB(row[1], 'str', 'varchar(255)')
            id = self.convertFromDB(row[2], 'long', 'int')
            keyStr = self.convertToDB(id,'long','int')

            other = DBOther(id=id,
                            key=key,
                            value=value)
            other.is_dirty = False
            list.append(other)

        return list

    def toSQL(self, db, obj, foreignKey=None, globalProps=None):
        keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if obj.is_dirty:
            columns = ['id']
            table = 'other'
            whereMap = {}
            columnMap = {}

            whereMap['id'] = keyStr
            if globalProps is not None:
                whereMap.update(globalProps)
            if obj.db_key is not None:
                columnMap['okey'] = \
                    self.convertToDB(obj.db_key, 'str', 'varchar(255)')
            if obj.db_value is not None:
                columnMap['value'] = \
                    self.convertToDB(obj.db_value, 'str', 'varchar(255)')
            if foreignKey is not None:
                columnMap.update(foreignKey)

            dbCommand = self.createSQLSelect(table, columns, whereMap)
            data = self.executeSQL(db, dbCommand, True)
            if len(data) <= 0:
                columnMap['id'] = keyStr
                if globalProps is not None:
                    columnMap.update(globalProps)
                dbCommand = self.createSQLInsert(table, columnMap)
            else:
                dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
            lastId = self.executeSQL(db, dbCommand, False)
        


class DBLocationSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props):
        columns = ['id', 'x', 'y', 'parent_type', 'vt_id', 'parent_id']
        table = 'location'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            x = self.convertFromDB(row[1], 'float', 'DECIMAL(18,12)')
            y = self.convertFromDB(row[2], 'float', 'DECIMAL(18,12)')
            parentType = self.convertFromDB(row[3], 'str', 'char(16)')
            vistrailId = self.convertFromDB(row[4], 'long', 'int')
            parent = self.convertFromDB(row[5], 'long', 'long')
            
            location = DBLocation(x=x,
                                  y=y,
                                  id=id)
            location.db_parentType = parentType
            location.db_vistrailId = vistrailId
            location.db_parent = parent
            location.is_dirty = False
            res[('location', id)] = location

        return res

    def from_sql_fast(self, obj, all_objects):
        if obj.db_parentType == 'module':
            p = all_objects[('module', obj.db_parent)]
            p.db_add_location(obj)
        elif obj.db_parentType == 'abstractionRef':
            p = all_objects[('abstractionRef', obj.db_parent)]
            p.db_add_location(obj)
        elif obj.db_parentType == 'add':
            p = all_objects[('add', obj.db_parent)]
            p.db_add_data(obj)
        elif obj.db_parentType == 'change':
            p = all_objects[('change', obj.db_parent)]
            p.db_add_data(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'x', 'y', 'parent_type', 'vt_id', 'parent_id']
        table = 'location'
        whereMap = {}
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        columnMap = {}
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            columnMap['id'] = \
                self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_x') and obj.db_x is not None:
            columnMap['x'] = \
                self.convertToDB(obj.db_x, 'float', 'DECIMAL(18,12)')
        if hasattr(obj, 'db_y') and obj.db_y is not None:
            columnMap['y'] = \
                self.convertToDB(obj.db_y, 'float', 'DECIMAL(18,12)')
        if hasattr(obj, 'db_parentType') and obj.db_parentType is not None:
            columnMap['parent_type'] = \
                self.convertToDB(obj.db_parentType, 'str', 'char(16)')
        if hasattr(obj, 'db_vistrailId') and obj.db_vistrailId is not None:
            columnMap['vt_id'] = \
                self.convertToDB(obj.db_vistrailId, 'long', 'int')
        if hasattr(obj, 'db_parent') and obj.db_parent is not None:
            columnMap['parent_id'] = \
                self.convertToDB(obj.db_parent, 'long', 'long')
        columnMap.update(global_props)

        if obj.is_new:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        pass
        
    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['x', 'y', 'id']
        table = 'location'
        whereMap = {}
        orderBy = 'id'

        if id is not None:
            keyStr = self.convertToDB(id, 'long', 'int')
            whereMap['id'] = keyStr
        elif foreignKey is not None:
            whereMap.update(foreignKey)
        elif globalProps is None:
            print '***ERROR: need to specify id or foreign key info'
        if globalProps is not None:
            whereMap.update(globalProps)
        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        list = []
        for row in data:
            x = self.convertFromDB(row[0], 'float', 'DECIMAL(18,12)')
            y = self.convertFromDB(row[1], 'float', 'DECIMAL(18,12)')
            id = self.convertFromDB(row[2], 'long', 'int')
            keyStr = self.convertToDB(id,'long','int')

            location = DBLocation(id=id,
                                  x=x,
                                  y=y)
            location.is_dirty = False
            list.append(location)

        return list

    def toSQL(self, db, obj, foreignKey=None, globalProps=None):
        keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if obj.is_dirty:
            columns = ['id']
            table = 'location'
            whereMap = {}
            columnMap = {}

            whereMap['id'] = keyStr
            if globalProps is not None:
                whereMap.update(globalProps)
            if obj.db_x is not None:
                columnMap['x'] = \
                    self.convertToDB(obj.db_x, 'float', 'DECIMAL(18,12)')
            if obj.db_y is not None:
                columnMap['y'] = \
                    self.convertToDB(obj.db_y, 'float', 'DECIMAL(18,12)')
            if foreignKey is not None:
                columnMap.update(foreignKey)

            dbCommand = self.createSQLSelect(table, columns, whereMap)
            data = self.executeSQL(db, dbCommand, True)
            if len(data) <= 0:
                columnMap['id'] = keyStr
                if globalProps is not None:
                    columnMap.update(globalProps)
                dbCommand = self.createSQLInsert(table, columnMap)
            else:
                dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
            lastId = self.executeSQL(db, dbCommand, False)
        


class DBWorkflowExecSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props):
        columns = ['id', 'user', 'ip', 'vt_version', 'ts_start', 'ts_end', 'parent_id', 'parent_type', 'parent_version', 'name', 'log_id', 'vt_id']
        table = 'workflow_exec'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            user = self.convertFromDB(row[1], 'str', 'varchar(255)')
            ip = self.convertFromDB(row[2], 'str', 'varchar(255)')
            vt_version = self.convertFromDB(row[3], 'str', 'varchar(255)')
            ts_start = self.convertFromDB(row[4], 'datetime', 'datetime')
            ts_end = self.convertFromDB(row[5], 'datetime', 'datetime')
            parent_id = self.convertFromDB(row[6], 'long', 'int')
            parent_type = self.convertFromDB(row[7], 'str', 'varchar(255)')
            parent_version = self.convertFromDB(row[8], 'long', 'int')
            name = self.convertFromDB(row[9], 'str', 'varchar(255)')
            log = self.convertFromDB(row[10], 'long', 'int')
            vistrailId = self.convertFromDB(row[11], 'long', 'int')
            
            workflow_exec = DBWorkflowExec(user=user,
                                           ip=ip,
                                           vt_version=vt_version,
                                           ts_start=ts_start,
                                           ts_end=ts_end,
                                           parent_id=parent_id,
                                           parent_type=parent_type,
                                           parent_version=parent_version,
                                           name=name,
                                           id=id)
            workflow_exec.db_log = log
            workflow_exec.db_vistrailId = vistrailId
            workflow_exec.is_dirty = False
            res[('workflow_exec', id)] = workflow_exec

        return res

    def from_sql_fast(self, obj, all_objects):
        p = all_objects[('log', obj.db_log)]
        p.db_add_workflow_exec(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'user', 'ip', 'vt_version', 'ts_start', 'ts_end', 'parent_id', 'parent_type', 'parent_version', 'name', 'log_id', 'vt_id']
        table = 'workflow_exec'
        whereMap = {}
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        columnMap = {}
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            columnMap['id'] = \
                self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_user') and obj.db_user is not None:
            columnMap['user'] = \
                self.convertToDB(obj.db_user, 'str', 'varchar(255)')
        if hasattr(obj, 'db_ip') and obj.db_ip is not None:
            columnMap['ip'] = \
                self.convertToDB(obj.db_ip, 'str', 'varchar(255)')
        if hasattr(obj, 'db_vt_version') and obj.db_vt_version is not None:
            columnMap['vt_version'] = \
                self.convertToDB(obj.db_vt_version, 'str', 'varchar(255)')
        if hasattr(obj, 'db_ts_start') and obj.db_ts_start is not None:
            columnMap['ts_start'] = \
                self.convertToDB(obj.db_ts_start, 'datetime', 'datetime')
        if hasattr(obj, 'db_ts_end') and obj.db_ts_end is not None:
            columnMap['ts_end'] = \
                self.convertToDB(obj.db_ts_end, 'datetime', 'datetime')
        if hasattr(obj, 'db_parent_id') and obj.db_parent_id is not None:
            columnMap['parent_id'] = \
                self.convertToDB(obj.db_parent_id, 'long', 'int')
        if hasattr(obj, 'db_parent_type') and obj.db_parent_type is not None:
            columnMap['parent_type'] = \
                self.convertToDB(obj.db_parent_type, 'str', 'varchar(255)')
        if hasattr(obj, 'db_parent_version') and obj.db_parent_version is not None:
            columnMap['parent_version'] = \
                self.convertToDB(obj.db_parent_version, 'long', 'int')
        if hasattr(obj, 'db_name') and obj.db_name is not None:
            columnMap['name'] = \
                self.convertToDB(obj.db_name, 'str', 'varchar(255)')
        if hasattr(obj, 'db_log') and obj.db_log is not None:
            columnMap['log_id'] = \
                self.convertToDB(obj.db_log, 'long', 'int')
        if hasattr(obj, 'db_vistrailId') and obj.db_vistrailId is not None:
            columnMap['vt_id'] = \
                self.convertToDB(obj.db_vistrailId, 'long', 'int')
        columnMap.update(global_props)

        if obj.is_new:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        for child in obj.db_module_execs:
            child.db_workflow_exec = obj.db_id
        
    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['user', 'ip', 'vt_version', 'ts_start', 'ts_end', 'parent_id', 'parent_type', 'parent_version', 'name', 'id']
        table = 'workflow_exec'
        whereMap = {}
        orderBy = 'id'

        if id is not None:
            keyStr = self.convertToDB(id, 'long', 'int')
            whereMap['id'] = keyStr
        elif foreignKey is not None:
            whereMap.update(foreignKey)
        elif globalProps is None:
            print '***ERROR: need to specify id or foreign key info'
        if globalProps is not None:
            whereMap.update(globalProps)
        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        list = []
        for row in data:
            user = self.convertFromDB(row[0], 'str', 'varchar(255)')
            ip = self.convertFromDB(row[1], 'str', 'varchar(255)')
            vt_version = self.convertFromDB(row[2], 'str', 'varchar(255)')
            ts_start = self.convertFromDB(row[3], 'datetime', 'datetime')
            ts_end = self.convertFromDB(row[4], 'datetime', 'datetime')
            parent_id = self.convertFromDB(row[5], 'long', 'int')
            parent_type = self.convertFromDB(row[6], 'str', 'varchar(255)')
            parent_version = self.convertFromDB(row[7], 'long', 'int')
            name = self.convertFromDB(row[8], 'str', 'varchar(255)')
            id = self.convertFromDB(row[9], 'long', 'int')
            keyStr = self.convertToDB(id,'long','int')

            foreignKey = {'wf_exec_id': keyStr}
            res = self.getDao('module_exec').fromSQL(db, None, foreignKey, globalProps)
            module_execs = res
            
            workflow_exec = DBWorkflowExec(id=id,
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
            workflow_exec.is_dirty = False
            list.append(workflow_exec)

        return list

    def toSQL(self, db, obj, foreignKey=None, globalProps=None):
        keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if obj.is_dirty:
            columns = ['id']
            table = 'workflow_exec'
            whereMap = {}
            columnMap = {}

            whereMap['id'] = keyStr
            if globalProps is not None:
                whereMap.update(globalProps)
            if obj.db_user is not None:
                columnMap['user'] = \
                    self.convertToDB(obj.db_user, 'str', 'varchar(255)')
            if obj.db_ip is not None:
                columnMap['ip'] = \
                    self.convertToDB(obj.db_ip, 'str', 'varchar(255)')
            if obj.db_vt_version is not None:
                columnMap['vt_version'] = \
                    self.convertToDB(obj.db_vt_version, 'str', 'varchar(255)')
            if obj.db_ts_start is not None:
                columnMap['ts_start'] = \
                    self.convertToDB(obj.db_ts_start, 'datetime', 'datetime')
            if obj.db_ts_end is not None:
                columnMap['ts_end'] = \
                    self.convertToDB(obj.db_ts_end, 'datetime', 'datetime')
            if obj.db_parent_id is not None:
                columnMap['parent_id'] = \
                    self.convertToDB(obj.db_parent_id, 'long', 'int')
            if obj.db_parent_type is not None:
                columnMap['parent_type'] = \
                    self.convertToDB(obj.db_parent_type, 'str', 'varchar(255)')
            if obj.db_parent_version is not None:
                columnMap['parent_version'] = \
                    self.convertToDB(obj.db_parent_version, 'long', 'int')
            if obj.db_name is not None:
                columnMap['name'] = \
                    self.convertToDB(obj.db_name, 'str', 'varchar(255)')
            if foreignKey is not None:
                columnMap.update(foreignKey)

            dbCommand = self.createSQLSelect(table, columns, whereMap)
            data = self.executeSQL(db, dbCommand, True)
            if len(data) <= 0:
                columnMap['id'] = keyStr
                if globalProps is not None:
                    columnMap.update(globalProps)
                dbCommand = self.createSQLInsert(table, columnMap)
            else:
                dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
            lastId = self.executeSQL(db, dbCommand, False)
        

        foreignKey = {'wf_exec_id': keyStr}
        for child in obj.db_module_execs:
            self.getDao('module_exec').toSQL(db, child, foreignKey, globalProps)
        

class DBFunctionSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props):
        columns = ['id', 'pos', 'name', 'parent_type', 'vt_id', 'parent_id']
        table = 'function'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            pos = self.convertFromDB(row[1], 'long', 'int')
            name = self.convertFromDB(row[2], 'str', 'varchar(255)')
            parentType = self.convertFromDB(row[3], 'str', 'char(16)')
            vistrailId = self.convertFromDB(row[4], 'long', 'int')
            parent = self.convertFromDB(row[5], 'long', 'long')
            
            function = DBFunction(pos=pos,
                                  name=name,
                                  id=id)
            function.db_parentType = parentType
            function.db_vistrailId = vistrailId
            function.db_parent = parent
            function.is_dirty = False
            res[('function', id)] = function

        return res

    def from_sql_fast(self, obj, all_objects):
        if obj.db_parentType == 'module':
            p = all_objects[('module', obj.db_parent)]
            p.db_add_function(obj)
        elif obj.db_parentType == 'add':
            p = all_objects[('add', obj.db_parent)]
            p.db_add_data(obj)
        elif obj.db_parentType == 'change':
            p = all_objects[('change', obj.db_parent)]
            p.db_add_data(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'pos', 'name', 'parent_type', 'vt_id', 'parent_id']
        table = 'function'
        whereMap = {}
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        columnMap = {}
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            columnMap['id'] = \
                self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_pos') and obj.db_pos is not None:
            columnMap['pos'] = \
                self.convertToDB(obj.db_pos, 'long', 'int')
        if hasattr(obj, 'db_name') and obj.db_name is not None:
            columnMap['name'] = \
                self.convertToDB(obj.db_name, 'str', 'varchar(255)')
        if hasattr(obj, 'db_parentType') and obj.db_parentType is not None:
            columnMap['parent_type'] = \
                self.convertToDB(obj.db_parentType, 'str', 'char(16)')
        if hasattr(obj, 'db_vistrailId') and obj.db_vistrailId is not None:
            columnMap['vt_id'] = \
                self.convertToDB(obj.db_vistrailId, 'long', 'int')
        if hasattr(obj, 'db_parent') and obj.db_parent is not None:
            columnMap['parent_id'] = \
                self.convertToDB(obj.db_parent, 'long', 'long')
        columnMap.update(global_props)

        if obj.is_new:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        for child in obj.db_parameters:
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        
    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['pos', 'name', 'id']
        table = 'function'
        whereMap = {}
        orderBy = 'id'

        if id is not None:
            keyStr = self.convertToDB(id, 'long', 'int')
            whereMap['id'] = keyStr
        elif foreignKey is not None:
            whereMap.update(foreignKey)
        elif globalProps is None:
            print '***ERROR: need to specify id or foreign key info'
        if globalProps is not None:
            whereMap.update(globalProps)
        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        list = []
        for row in data:
            pos = self.convertFromDB(row[0], 'long', 'int')
            name = self.convertFromDB(row[1], 'str', 'varchar(255)')
            id = self.convertFromDB(row[2], 'long', 'int')
            keyStr = self.convertToDB(id,'long','int')

            discStr = self.convertToDB('function','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            res = self.getDao('parameter').fromSQL(db, None, foreignKey, globalProps)
            parameters = res
            
            function = DBFunction(id=id,
                                  pos=pos,
                                  name=name,
                                  parameters=parameters)
            function.is_dirty = False
            list.append(function)

        return list

    def toSQL(self, db, obj, foreignKey=None, globalProps=None):
        keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if obj.is_dirty:
            columns = ['id']
            table = 'function'
            whereMap = {}
            columnMap = {}

            whereMap['id'] = keyStr
            if globalProps is not None:
                whereMap.update(globalProps)
            if obj.db_pos is not None:
                columnMap['pos'] = \
                    self.convertToDB(obj.db_pos, 'long', 'int')
            if obj.db_name is not None:
                columnMap['name'] = \
                    self.convertToDB(obj.db_name, 'str', 'varchar(255)')
            if foreignKey is not None:
                columnMap.update(foreignKey)

            dbCommand = self.createSQLSelect(table, columns, whereMap)
            data = self.executeSQL(db, dbCommand, True)
            if len(data) <= 0:
                columnMap['id'] = keyStr
                if globalProps is not None:
                    columnMap.update(globalProps)
                dbCommand = self.createSQLInsert(table, columnMap)
            else:
                dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
            lastId = self.executeSQL(db, dbCommand, False)
        

        discStr = self.convertToDB('function','str','char(16)')
        foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
        for child in obj.db_parameters:
            self.getDao('parameter').toSQL(db, child, foreignKey, globalProps)
        

class DBAbstractionSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props):
        columns = ['id', 'name', 'vt_id']
        table = 'abstraction'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            name = self.convertFromDB(row[1], 'str', 'varchar(255)')
            vistrail = self.convertFromDB(row[2], 'long', 'int')
            
            abstraction = DBAbstraction(name=name,
                                        id=id)
            abstraction.db_vistrail = vistrail
            abstraction.is_dirty = False
            res[('abstraction', id)] = abstraction

        return res

    def from_sql_fast(self, obj, all_objects):
        p = all_objects[('vistrail', obj.db_vistrail)]
        p.db_add_abstraction(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'name', 'vt_id']
        table = 'abstraction'
        whereMap = {}
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        columnMap = {}
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            columnMap['id'] = \
                self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_name') and obj.db_name is not None:
            columnMap['name'] = \
                self.convertToDB(obj.db_name, 'str', 'varchar(255)')
        if hasattr(obj, 'db_vistrail') and obj.db_vistrail is not None:
            columnMap['vt_id'] = \
                self.convertToDB(obj.db_vistrail, 'long', 'int')
        columnMap.update(global_props)

        if obj.is_new:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        for child in obj.db_actions:
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        for child in obj.db_tags:
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        
    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['name', 'id']
        table = 'abstraction'
        whereMap = {}
        orderBy = 'id'

        if id is not None:
            keyStr = self.convertToDB(id, 'long', 'int')
            whereMap['id'] = keyStr
        elif foreignKey is not None:
            whereMap.update(foreignKey)
        elif globalProps is None:
            print '***ERROR: need to specify id or foreign key info'
        if globalProps is not None:
            whereMap.update(globalProps)
        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        list = []
        for row in data:
            name = self.convertFromDB(row[0], 'str', 'varchar(255)')
            id = self.convertFromDB(row[1], 'long', 'int')
            keyStr = self.convertToDB(id,'long','int')

            discStr = self.convertToDB('abstraction','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            res = self.getDao('action').fromSQL(db, None, foreignKey, globalProps)
            actions = res
            
            discStr = self.convertToDB('abstraction','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            res = self.getDao('tag').fromSQL(db, None, foreignKey, globalProps)
            tags = res
            
            abstraction = DBAbstraction(id=id,
                                        name=name,
                                        actions=actions,
                                        tags=tags)
            abstraction.is_dirty = False
            list.append(abstraction)

        return list

    def toSQL(self, db, obj, foreignKey=None, globalProps=None):
        keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if obj.is_dirty:
            columns = ['id']
            table = 'abstraction'
            whereMap = {}
            columnMap = {}

            whereMap['id'] = keyStr
            if globalProps is not None:
                whereMap.update(globalProps)
            if obj.db_name is not None:
                columnMap['name'] = \
                    self.convertToDB(obj.db_name, 'str', 'varchar(255)')
            if foreignKey is not None:
                columnMap.update(foreignKey)

            dbCommand = self.createSQLSelect(table, columns, whereMap)
            data = self.executeSQL(db, dbCommand, True)
            if len(data) <= 0:
                columnMap['id'] = keyStr
                if globalProps is not None:
                    columnMap.update(globalProps)
                dbCommand = self.createSQLInsert(table, columnMap)
            else:
                dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
            lastId = self.executeSQL(db, dbCommand, False)
        

        discStr = self.convertToDB('abstraction','str','char(16)')
        foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
        for child in obj.db_actions:
            self.getDao('action').toSQL(db, child, foreignKey, globalProps)
        
        discStr = self.convertToDB('abstraction','str','char(16)')
        foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
        for child in obj.db_tags:
            self.getDao('tag').toSQL(db, child, foreignKey, globalProps)
        

class DBWorkflowSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props):
        columns = ['id', 'name', 'vt_id']
        table = 'workflow'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            name = self.convertFromDB(row[1], 'str', 'varchar(255)')
            vistrailId = self.convertFromDB(row[2], 'long', 'int')
            
            workflow = DBWorkflow(name=name,
                                  id=id)
            workflow.db_vistrailId = vistrailId
            workflow.is_dirty = False
            res[('workflow', id)] = workflow

        return res

    def from_sql_fast(self, obj, all_objects):
        pass
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'name', 'vt_id']
        table = 'workflow'
        whereMap = {}
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        columnMap = {}
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            columnMap['id'] = \
                self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_name') and obj.db_name is not None:
            columnMap['name'] = \
                self.convertToDB(obj.db_name, 'str', 'varchar(255)')
        if hasattr(obj, 'db_vistrailId') and obj.db_vistrailId is not None:
            columnMap['vt_id'] = \
                self.convertToDB(obj.db_vistrailId, 'long', 'int')
        columnMap.update(global_props)

        if obj.is_new:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        for child in obj.db_modules:
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        for child in obj.db_connections:
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        for child in obj.db_annotations:
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        for child in obj.db_others:
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        for child in obj.db_abstractionRefs:
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        
    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['name', 'id']
        table = 'workflow'
        whereMap = {}
        orderBy = 'id'

        if id is not None:
            keyStr = self.convertToDB(id, 'long', 'int')
            whereMap['id'] = keyStr
        elif foreignKey is not None:
            whereMap.update(foreignKey)
        elif globalProps is None:
            print '***ERROR: need to specify id or foreign key info'
        if globalProps is not None:
            whereMap.update(globalProps)
        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        list = []
        for row in data:
            name = self.convertFromDB(row[0], 'str', 'varchar(255)')
            id = self.convertFromDB(row[1], 'long', 'int')
            keyStr = self.convertToDB(id,'long','int')

            discStr = self.convertToDB('workflow','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            res = self.getDao('module').fromSQL(db, None, foreignKey, globalProps)
            modules = res
            
            discStr = self.convertToDB('workflow','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            res = self.getDao('connection').fromSQL(db, None, foreignKey, globalProps)
            connections = res
            
            discStr = self.convertToDB('workflow','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            res = self.getDao('annotation').fromSQL(db, None, foreignKey, globalProps)
            annotations = res
            
            discStr = self.convertToDB('workflow','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            res = self.getDao('other').fromSQL(db, None, foreignKey, globalProps)
            others = res
            
            discStr = self.convertToDB('workflow','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            res = self.getDao('abstractionRef').fromSQL(db, None, foreignKey, globalProps)
            abstractionRefs = res
            
            workflow = DBWorkflow(id=id,
                                  name=name,
                                  modules=modules,
                                  connections=connections,
                                  annotations=annotations,
                                  others=others,
                                  abstractionRefs=abstractionRefs)
            workflow.is_dirty = False
            list.append(workflow)

        return list

    def toSQL(self, db, obj, foreignKey=None, globalProps=None):
        keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if obj.is_dirty:
            columns = ['id']
            table = 'workflow'
            whereMap = {}
            columnMap = {}

            whereMap['id'] = keyStr
            if globalProps is not None:
                whereMap.update(globalProps)
            if obj.db_name is not None:
                columnMap['name'] = \
                    self.convertToDB(obj.db_name, 'str', 'varchar(255)')
            if foreignKey is not None:
                columnMap.update(foreignKey)

            dbCommand = self.createSQLSelect(table, columns, whereMap)
            data = self.executeSQL(db, dbCommand, True)
            if len(data) <= 0:
                columnMap['id'] = keyStr
                if globalProps is not None:
                    columnMap.update(globalProps)
                dbCommand = self.createSQLInsert(table, columnMap)
            else:
                dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
            lastId = self.executeSQL(db, dbCommand, False)
        

        discStr = self.convertToDB('workflow','str','char(16)')
        foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
        for child in obj.db_modules:
            self.getDao('module').toSQL(db, child, foreignKey, globalProps)
        
        discStr = self.convertToDB('workflow','str','char(16)')
        foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
        for child in obj.db_connections:
            self.getDao('connection').toSQL(db, child, foreignKey, globalProps)
        
        discStr = self.convertToDB('workflow','str','char(16)')
        foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
        for child in obj.db_annotations:
            self.getDao('annotation').toSQL(db, child, foreignKey, globalProps)
        
        discStr = self.convertToDB('workflow','str','char(16)')
        foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
        for child in obj.db_others:
            self.getDao('other').toSQL(db, child, foreignKey, globalProps)
        
        discStr = self.convertToDB('workflow','str','char(16)')
        foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
        for child in obj.db_abstractionRefs:
            self.getDao('abstractionRef').toSQL(db, child, foreignKey, globalProps)
        

class DBAbstractionRefSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props):
        columns = ['id', 'abstraction_id', 'version', 'parent_type', 'vt_id', 'parent_id']
        table = 'abstraction_ref'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            abstraction_id = self.convertFromDB(row[1], 'long', 'int')
            version = self.convertFromDB(row[2], 'long', 'int')
            parentType = self.convertFromDB(row[3], 'str', 'char(16)')
            vistrailId = self.convertFromDB(row[4], 'long', 'int')
            parent = self.convertFromDB(row[5], 'long', 'long')
            
            abstractionRef = DBAbstractionRef(abstraction_id=abstraction_id,
                                              version=version,
                                              id=id)
            abstractionRef.db_parentType = parentType
            abstractionRef.db_vistrailId = vistrailId
            abstractionRef.db_parent = parent
            abstractionRef.is_dirty = False
            res[('abstractionRef', id)] = abstractionRef

        return res

    def from_sql_fast(self, obj, all_objects):
        if obj.db_parentType == 'workflow':
            p = all_objects[('workflow', obj.db_parent)]
            p.db_add_abstractionRef(obj)
        elif obj.db_parentType == 'add':
            p = all_objects[('add', obj.db_parent)]
            p.db_add_data(obj)
        elif obj.db_parentType == 'change':
            p = all_objects[('change', obj.db_parent)]
            p.db_add_data(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'abstraction_id', 'version', 'parent_type', 'vt_id', 'parent_id']
        table = 'abstraction_ref'
        whereMap = {}
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        columnMap = {}
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            columnMap['id'] = \
                self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_abstraction_id') and obj.db_abstraction_id is not None:
            columnMap['abstraction_id'] = \
                self.convertToDB(obj.db_abstraction_id, 'long', 'int')
        if hasattr(obj, 'db_version') and obj.db_version is not None:
            columnMap['version'] = \
                self.convertToDB(obj.db_version, 'long', 'int')
        if hasattr(obj, 'db_parentType') and obj.db_parentType is not None:
            columnMap['parent_type'] = \
                self.convertToDB(obj.db_parentType, 'str', 'char(16)')
        if hasattr(obj, 'db_vistrailId') and obj.db_vistrailId is not None:
            columnMap['vt_id'] = \
                self.convertToDB(obj.db_vistrailId, 'long', 'int')
        if hasattr(obj, 'db_parent') and obj.db_parent is not None:
            columnMap['parent_id'] = \
                self.convertToDB(obj.db_parent, 'long', 'long')
        columnMap.update(global_props)

        if obj.is_new:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        if obj.db_location is not None:
            child = obj.db_location
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        
    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['abstraction_id', 'version', 'id']
        table = 'abstraction_ref'
        whereMap = {}
        orderBy = 'id'

        if id is not None:
            keyStr = self.convertToDB(id, 'long', 'int')
            whereMap['id'] = keyStr
        elif foreignKey is not None:
            whereMap.update(foreignKey)
        elif globalProps is None:
            print '***ERROR: need to specify id or foreign key info'
        if globalProps is not None:
            whereMap.update(globalProps)
        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        list = []
        for row in data:
            abstraction_id = self.convertFromDB(row[0], 'long', 'int')
            version = self.convertFromDB(row[1], 'long', 'int')
            id = self.convertFromDB(row[2], 'long', 'int')
            keyStr = self.convertToDB(id,'long','int')

            discStr = self.convertToDB('abstractionRef','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            res = self.getDao('location').fromSQL(db, None, foreignKey, globalProps)
            if len(res) > 0:
                location = res[0]
            else:
                location = None
            
            abstractionRef = DBAbstractionRef(id=id,
                                              abstraction_id=abstraction_id,
                                              version=version,
                                              location=location)
            abstractionRef.is_dirty = False
            list.append(abstractionRef)

        return list

    def toSQL(self, db, obj, foreignKey=None, globalProps=None):
        keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if obj.is_dirty:
            columns = ['id']
            table = 'abstraction_ref'
            whereMap = {}
            columnMap = {}

            whereMap['id'] = keyStr
            if globalProps is not None:
                whereMap.update(globalProps)
            if obj.db_abstraction_id is not None:
                columnMap['abstraction_id'] = \
                    self.convertToDB(obj.db_abstraction_id, 'long', 'int')
            if obj.db_version is not None:
                columnMap['version'] = \
                    self.convertToDB(obj.db_version, 'long', 'int')
            if foreignKey is not None:
                columnMap.update(foreignKey)

            dbCommand = self.createSQLSelect(table, columns, whereMap)
            data = self.executeSQL(db, dbCommand, True)
            if len(data) <= 0:
                columnMap['id'] = keyStr
                if globalProps is not None:
                    columnMap.update(globalProps)
                dbCommand = self.createSQLInsert(table, columnMap)
            else:
                dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
            lastId = self.executeSQL(db, dbCommand, False)
        

        discStr = self.convertToDB('abstractionRef','str','char(16)')
        foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
        child = obj.db_location
        if child is not None:
            self.getDao('location').toSQL(db, child, foreignKey, globalProps)
        

class DBAnnotationSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props):
        columns = ['id', 'akey', 'value', 'parent_type', 'vt_id', 'parent_id']
        table = 'annotation'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            key = self.convertFromDB(row[1], 'str', 'varchar(255)')
            value = self.convertFromDB(row[2], 'str', 'varchar(8191)')
            parentType = self.convertFromDB(row[3], 'str', 'char(16)')
            vistrailId = self.convertFromDB(row[4], 'long', 'int')
            parent = self.convertFromDB(row[5], 'long', 'long')
            
            annotation = DBAnnotation(key=key,
                                      value=value,
                                      id=id)
            annotation.db_parentType = parentType
            annotation.db_vistrailId = vistrailId
            annotation.db_parent = parent
            annotation.is_dirty = False
            res[('annotation', id)] = annotation

        return res

    def from_sql_fast(self, obj, all_objects):
        if obj.db_parentType == 'workflow':
            p = all_objects[('workflow', obj.db_parent)]
            p.db_add_annotation(obj)
        elif obj.db_parentType == 'module':
            p = all_objects[('module', obj.db_parent)]
            p.db_add_annotation(obj)
        elif obj.db_parentType == 'module_exec':
            p = all_objects[('module_exec', obj.db_parent)]
            p.db_add_annotation(obj)
        elif obj.db_parentType == 'add':
            p = all_objects[('add', obj.db_parent)]
            p.db_add_data(obj)
        elif obj.db_parentType == 'change':
            p = all_objects[('change', obj.db_parent)]
            p.db_add_data(obj)
        elif obj.db_parentType == 'action':
            p = all_objects[('action', obj.db_parent)]
            p.db_add_annotation(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'akey', 'value', 'parent_type', 'vt_id', 'parent_id']
        table = 'annotation'
        whereMap = {}
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        columnMap = {}
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            columnMap['id'] = \
                self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_key') and obj.db_key is not None:
            columnMap['akey'] = \
                self.convertToDB(obj.db_key, 'str', 'varchar(255)')
        if hasattr(obj, 'db_value') and obj.db_value is not None:
            columnMap['value'] = \
                self.convertToDB(obj.db_value, 'str', 'varchar(8191)')
        if hasattr(obj, 'db_parentType') and obj.db_parentType is not None:
            columnMap['parent_type'] = \
                self.convertToDB(obj.db_parentType, 'str', 'char(16)')
        if hasattr(obj, 'db_vistrailId') and obj.db_vistrailId is not None:
            columnMap['vt_id'] = \
                self.convertToDB(obj.db_vistrailId, 'long', 'int')
        if hasattr(obj, 'db_parent') and obj.db_parent is not None:
            columnMap['parent_id'] = \
                self.convertToDB(obj.db_parent, 'long', 'long')
        columnMap.update(global_props)

        if obj.is_new:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        pass
        
    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['akey', 'value', 'id']
        table = 'annotation'
        whereMap = {}
        orderBy = 'id'

        if id is not None:
            keyStr = self.convertToDB(id, 'long', 'int')
            whereMap['id'] = keyStr
        elif foreignKey is not None:
            whereMap.update(foreignKey)
        elif globalProps is None:
            print '***ERROR: need to specify id or foreign key info'
        if globalProps is not None:
            whereMap.update(globalProps)
        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        list = []
        for row in data:
            key = self.convertFromDB(row[0], 'str', 'varchar(255)')
            value = self.convertFromDB(row[1], 'str', 'varchar(8191)')
            id = self.convertFromDB(row[2], 'long', 'int')
            keyStr = self.convertToDB(id,'long','int')

            annotation = DBAnnotation(id=id,
                                      key=key,
                                      value=value)
            annotation.is_dirty = False
            list.append(annotation)

        return list

    def toSQL(self, db, obj, foreignKey=None, globalProps=None):
        keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if obj.is_dirty:
            columns = ['id']
            table = 'annotation'
            whereMap = {}
            columnMap = {}

            whereMap['id'] = keyStr
            if globalProps is not None:
                whereMap.update(globalProps)
            if obj.db_key is not None:
                columnMap['akey'] = \
                    self.convertToDB(obj.db_key, 'str', 'varchar(255)')
            if obj.db_value is not None:
                columnMap['value'] = \
                    self.convertToDB(obj.db_value, 'str', 'varchar(8191)')
            if foreignKey is not None:
                columnMap.update(foreignKey)

            dbCommand = self.createSQLSelect(table, columns, whereMap)
            data = self.executeSQL(db, dbCommand, True)
            if len(data) <= 0:
                columnMap['id'] = keyStr
                if globalProps is not None:
                    columnMap.update(globalProps)
                dbCommand = self.createSQLInsert(table, columnMap)
            else:
                dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
            lastId = self.executeSQL(db, dbCommand, False)
        


class DBChangeSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props):
        columns = ['id', 'what', 'old_obj_id', 'new_obj_id', 'par_obj_id', 'par_obj_type', 'action_id', 'vt_id']
        table = 'change_tbl'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            what = self.convertFromDB(row[1], 'str', 'varchar(255)')
            oldObjId = self.convertFromDB(row[2], 'long', 'int')
            newObjId = self.convertFromDB(row[3], 'long', 'int')
            parentObjId = self.convertFromDB(row[4], 'long', 'int')
            parentObjType = self.convertFromDB(row[5], 'str', 'char(16)')
            action = self.convertFromDB(row[6], 'long', 'int')
            vistrailId = self.convertFromDB(row[7], 'long', 'int')
            
            change = DBChange(what=what,
                              oldObjId=oldObjId,
                              newObjId=newObjId,
                              parentObjId=parentObjId,
                              parentObjType=parentObjType,
                              id=id)
            change.db_action = action
            change.db_vistrailId = vistrailId
            change.is_dirty = False
            res[('change', id)] = change

        return res

    def from_sql_fast(self, obj, all_objects):
        p = all_objects[('action', obj.db_action)]
        p.db_add_operation(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'what', 'old_obj_id', 'new_obj_id', 'par_obj_id', 'par_obj_type', 'action_id', 'vt_id']
        table = 'change_tbl'
        whereMap = {}
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        columnMap = {}
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            columnMap['id'] = \
                self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_what') and obj.db_what is not None:
            columnMap['what'] = \
                self.convertToDB(obj.db_what, 'str', 'varchar(255)')
        if hasattr(obj, 'db_oldObjId') and obj.db_oldObjId is not None:
            columnMap['old_obj_id'] = \
                self.convertToDB(obj.db_oldObjId, 'long', 'int')
        if hasattr(obj, 'db_newObjId') and obj.db_newObjId is not None:
            columnMap['new_obj_id'] = \
                self.convertToDB(obj.db_newObjId, 'long', 'int')
        if hasattr(obj, 'db_parentObjId') and obj.db_parentObjId is not None:
            columnMap['par_obj_id'] = \
                self.convertToDB(obj.db_parentObjId, 'long', 'int')
        if hasattr(obj, 'db_parentObjType') and obj.db_parentObjType is not None:
            columnMap['par_obj_type'] = \
                self.convertToDB(obj.db_parentObjType, 'str', 'char(16)')
        if hasattr(obj, 'db_action') and obj.db_action is not None:
            columnMap['action_id'] = \
                self.convertToDB(obj.db_action, 'long', 'int')
        if hasattr(obj, 'db_vistrailId') and obj.db_vistrailId is not None:
            columnMap['vt_id'] = \
                self.convertToDB(obj.db_vistrailId, 'long', 'int')
        columnMap.update(global_props)

        if obj.is_new:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        if obj.db_data is not None:
            child = obj.db_data
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        
    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['what', 'old_obj_id', 'new_obj_id', 'par_obj_id', 'par_obj_type', 'id']
        table = 'change_tbl'
        whereMap = {}
        orderBy = 'id'

        if id is not None:
            keyStr = self.convertToDB(id, 'long', 'int')
            whereMap['id'] = keyStr
        elif foreignKey is not None:
            whereMap.update(foreignKey)
        elif globalProps is None:
            print '***ERROR: need to specify id or foreign key info'
        if globalProps is not None:
            whereMap.update(globalProps)
        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        list = []
        for row in data:
            what = self.convertFromDB(row[0], 'str', 'varchar(255)')
            oldObjId = self.convertFromDB(row[1], 'long', 'int')
            newObjId = self.convertFromDB(row[2], 'long', 'int')
            parentObjId = self.convertFromDB(row[3], 'long', 'int')
            parentObjType = self.convertFromDB(row[4], 'str', 'char(16)')
            id = self.convertFromDB(row[5], 'long', 'int')
            keyStr = self.convertToDB(id,'long','int')

            data = None
            if what == 'module':
                discStr = self.convertToDB('change','str','char(16)')
                foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
                res = self.getDao('module').fromSQL(db, None, foreignKey, globalProps)
                data = res[0]
            elif what == 'location':
                discStr = self.convertToDB('change','str','char(16)')
                foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
                res = self.getDao('location').fromSQL(db, None, foreignKey, globalProps)
                data = res[0]
            elif what == 'annotation':
                discStr = self.convertToDB('change','str','char(16)')
                foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
                res = self.getDao('annotation').fromSQL(db, None, foreignKey, globalProps)
                data = res[0]
            elif what == 'function':
                discStr = self.convertToDB('change','str','char(16)')
                foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
                res = self.getDao('function').fromSQL(db, None, foreignKey, globalProps)
                data = res[0]
            elif what == 'connection':
                discStr = self.convertToDB('change','str','char(16)')
                foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
                res = self.getDao('connection').fromSQL(db, None, foreignKey, globalProps)
                data = res[0]
            elif what == 'port':
                discStr = self.convertToDB('change','str','char(16)')
                foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
                res = self.getDao('port').fromSQL(db, None, foreignKey, globalProps)
                data = res[0]
            elif what == 'parameter':
                discStr = self.convertToDB('change','str','char(16)')
                foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
                res = self.getDao('parameter').fromSQL(db, None, foreignKey, globalProps)
                data = res[0]
            elif what == 'portSpec':
                discStr = self.convertToDB('change','str','char(16)')
                foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
                res = self.getDao('portSpec').fromSQL(db, None, foreignKey, globalProps)
                data = res[0]
            elif what == 'abstractionRef':
                discStr = self.convertToDB('change','str','char(16)')
                foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
                res = self.getDao('abstractionRef').fromSQL(db, None, foreignKey, globalProps)
                data = res[0]
            elif what == 'other':
                discStr = self.convertToDB('change','str','char(16)')
                foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
                res = self.getDao('other').fromSQL(db, None, foreignKey, globalProps)
                data = res[0]
            
            change = DBChange(id=id,
                              what=what,
                              oldObjId=oldObjId,
                              newObjId=newObjId,
                              parentObjId=parentObjId,
                              parentObjType=parentObjType,
                              data=data)
            change.is_dirty = False
            list.append(change)

        return list

    def toSQL(self, db, obj, foreignKey=None, globalProps=None):
        keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if obj.is_dirty:
            columns = ['id']
            table = 'change_tbl'
            whereMap = {}
            columnMap = {}

            whereMap['id'] = keyStr
            if globalProps is not None:
                whereMap.update(globalProps)
            if obj.db_what is not None:
                columnMap['what'] = \
                    self.convertToDB(obj.db_what, 'str', 'varchar(255)')
            if obj.db_oldObjId is not None:
                columnMap['old_obj_id'] = \
                    self.convertToDB(obj.db_oldObjId, 'long', 'int')
            if obj.db_newObjId is not None:
                columnMap['new_obj_id'] = \
                    self.convertToDB(obj.db_newObjId, 'long', 'int')
            if obj.db_parentObjId is not None:
                columnMap['par_obj_id'] = \
                    self.convertToDB(obj.db_parentObjId, 'long', 'int')
            if obj.db_parentObjType is not None:
                columnMap['par_obj_type'] = \
                    self.convertToDB(obj.db_parentObjType, 'str', 'char(16)')
            if foreignKey is not None:
                columnMap.update(foreignKey)

            dbCommand = self.createSQLSelect(table, columns, whereMap)
            data = self.executeSQL(db, dbCommand, True)
            if len(data) <= 0:
                columnMap['id'] = keyStr
                if globalProps is not None:
                    columnMap.update(globalProps)
                dbCommand = self.createSQLInsert(table, columnMap)
            else:
                dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
            lastId = self.executeSQL(db, dbCommand, False)
        

        child = obj.db_data
        if child.vtType == 'module':
            discStr = self.convertToDB('change','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            self.getDao('module').toSQL(db, child, foreignKey, globalProps)
        elif child.vtType == 'location':
            discStr = self.convertToDB('change','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            self.getDao('location').toSQL(db, child, foreignKey, globalProps)
        elif child.vtType == 'annotation':
            discStr = self.convertToDB('change','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            self.getDao('annotation').toSQL(db, child, foreignKey, globalProps)
        elif child.vtType == 'function':
            discStr = self.convertToDB('change','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            self.getDao('function').toSQL(db, child, foreignKey, globalProps)
        elif child.vtType == 'connection':
            discStr = self.convertToDB('change','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            self.getDao('connection').toSQL(db, child, foreignKey, globalProps)
        elif child.vtType == 'port':
            discStr = self.convertToDB('change','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            self.getDao('port').toSQL(db, child, foreignKey, globalProps)
        elif child.vtType == 'parameter':
            discStr = self.convertToDB('change','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            self.getDao('parameter').toSQL(db, child, foreignKey, globalProps)
        elif child.vtType == 'portSpec':
            discStr = self.convertToDB('change','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            self.getDao('portSpec').toSQL(db, child, foreignKey, globalProps)
        elif child.vtType == 'abstractionRef':
            discStr = self.convertToDB('change','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            self.getDao('abstractionRef').toSQL(db, child, foreignKey, globalProps)
        elif child.vtType == 'other':
            discStr = self.convertToDB('change','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            self.getDao('other').toSQL(db, child, foreignKey, globalProps)
        

class DBParameterSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props):
        columns = ['id', 'pos', 'name', 'type', 'val', 'alias', 'parent_type', 'vt_id', 'parent_id']
        table = 'parameter'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            pos = self.convertFromDB(row[1], 'long', 'int')
            name = self.convertFromDB(row[2], 'str', 'varchar(255)')
            type = self.convertFromDB(row[3], 'str', 'varchar(255)')
            val = self.convertFromDB(row[4], 'str', 'varchar(8191)')
            alias = self.convertFromDB(row[5], 'str', 'varchar(255)')
            parentType = self.convertFromDB(row[6], 'str', 'char(16)')
            vistrailId = self.convertFromDB(row[7], 'long', 'int')
            parent = self.convertFromDB(row[8], 'long', 'long')
            
            parameter = DBParameter(pos=pos,
                                    name=name,
                                    type=type,
                                    val=val,
                                    alias=alias,
                                    id=id)
            parameter.db_parentType = parentType
            parameter.db_vistrailId = vistrailId
            parameter.db_parent = parent
            parameter.is_dirty = False
            res[('parameter', id)] = parameter

        return res

    def from_sql_fast(self, obj, all_objects):
        if obj.db_parentType == 'function':
            p = all_objects[('function', obj.db_parent)]
            p.db_add_parameter(obj)
        elif obj.db_parentType == 'add':
            p = all_objects[('add', obj.db_parent)]
            p.db_add_data(obj)
        elif obj.db_parentType == 'change':
            p = all_objects[('change', obj.db_parent)]
            p.db_add_data(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'pos', 'name', 'type', 'val', 'alias', 'parent_type', 'vt_id', 'parent_id']
        table = 'parameter'
        whereMap = {}
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        columnMap = {}
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            columnMap['id'] = \
                self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_pos') and obj.db_pos is not None:
            columnMap['pos'] = \
                self.convertToDB(obj.db_pos, 'long', 'int')
        if hasattr(obj, 'db_name') and obj.db_name is not None:
            columnMap['name'] = \
                self.convertToDB(obj.db_name, 'str', 'varchar(255)')
        if hasattr(obj, 'db_type') and obj.db_type is not None:
            columnMap['type'] = \
                self.convertToDB(obj.db_type, 'str', 'varchar(255)')
        if hasattr(obj, 'db_val') and obj.db_val is not None:
            columnMap['val'] = \
                self.convertToDB(obj.db_val, 'str', 'varchar(8191)')
        if hasattr(obj, 'db_alias') and obj.db_alias is not None:
            columnMap['alias'] = \
                self.convertToDB(obj.db_alias, 'str', 'varchar(255)')
        if hasattr(obj, 'db_parentType') and obj.db_parentType is not None:
            columnMap['parent_type'] = \
                self.convertToDB(obj.db_parentType, 'str', 'char(16)')
        if hasattr(obj, 'db_vistrailId') and obj.db_vistrailId is not None:
            columnMap['vt_id'] = \
                self.convertToDB(obj.db_vistrailId, 'long', 'int')
        if hasattr(obj, 'db_parent') and obj.db_parent is not None:
            columnMap['parent_id'] = \
                self.convertToDB(obj.db_parent, 'long', 'long')
        columnMap.update(global_props)

        if obj.is_new:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        pass
        
    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['pos', 'name', 'type', 'val', 'alias', 'id']
        table = 'parameter'
        whereMap = {}
        orderBy = 'id'

        if id is not None:
            keyStr = self.convertToDB(id, 'long', 'int')
            whereMap['id'] = keyStr
        elif foreignKey is not None:
            whereMap.update(foreignKey)
        elif globalProps is None:
            print '***ERROR: need to specify id or foreign key info'
        if globalProps is not None:
            whereMap.update(globalProps)
        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        list = []
        for row in data:
            pos = self.convertFromDB(row[0], 'long', 'int')
            name = self.convertFromDB(row[1], 'str', 'varchar(255)')
            type = self.convertFromDB(row[2], 'str', 'varchar(255)')
            val = self.convertFromDB(row[3], 'str', 'varchar(8191)')
            alias = self.convertFromDB(row[4], 'str', 'varchar(255)')
            id = self.convertFromDB(row[5], 'long', 'int')
            keyStr = self.convertToDB(id,'long','int')

            parameter = DBParameter(id=id,
                                    pos=pos,
                                    name=name,
                                    type=type,
                                    val=val,
                                    alias=alias)
            parameter.is_dirty = False
            list.append(parameter)

        return list

    def toSQL(self, db, obj, foreignKey=None, globalProps=None):
        keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if obj.is_dirty:
            columns = ['id']
            table = 'parameter'
            whereMap = {}
            columnMap = {}

            whereMap['id'] = keyStr
            if globalProps is not None:
                whereMap.update(globalProps)
            if obj.db_pos is not None:
                columnMap['pos'] = \
                    self.convertToDB(obj.db_pos, 'long', 'int')
            if obj.db_name is not None:
                columnMap['name'] = \
                    self.convertToDB(obj.db_name, 'str', 'varchar(255)')
            if obj.db_type is not None:
                columnMap['type'] = \
                    self.convertToDB(obj.db_type, 'str', 'varchar(255)')
            if obj.db_val is not None:
                columnMap['val'] = \
                    self.convertToDB(obj.db_val, 'str', 'varchar(8191)')
            if obj.db_alias is not None:
                columnMap['alias'] = \
                    self.convertToDB(obj.db_alias, 'str', 'varchar(255)')
            if foreignKey is not None:
                columnMap.update(foreignKey)

            dbCommand = self.createSQLSelect(table, columns, whereMap)
            data = self.executeSQL(db, dbCommand, True)
            if len(data) <= 0:
                columnMap['id'] = keyStr
                if globalProps is not None:
                    columnMap.update(globalProps)
                dbCommand = self.createSQLInsert(table, columnMap)
            else:
                dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
            lastId = self.executeSQL(db, dbCommand, False)
        


class DBConnectionSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props):
        columns = ['id', 'parent_type', 'vt_id', 'parent_id']
        table = 'connection_tbl'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            parentType = self.convertFromDB(row[1], 'str', 'char(16)')
            vistrailId = self.convertFromDB(row[2], 'long', 'int')
            parent = self.convertFromDB(row[3], 'long', 'long')
            
            connection = DBConnection(id=id)
            connection.db_parentType = parentType
            connection.db_vistrailId = vistrailId
            connection.db_parent = parent
            connection.is_dirty = False
            res[('connection', id)] = connection

        return res

    def from_sql_fast(self, obj, all_objects):
        if obj.db_parentType == 'workflow':
            p = all_objects[('workflow', obj.db_parent)]
            p.db_add_connection(obj)
        elif obj.db_parentType == 'add':
            p = all_objects[('add', obj.db_parent)]
            p.db_add_data(obj)
        elif obj.db_parentType == 'change':
            p = all_objects[('change', obj.db_parent)]
            p.db_add_data(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'parent_type', 'vt_id', 'parent_id']
        table = 'connection_tbl'
        whereMap = {}
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        columnMap = {}
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            columnMap['id'] = \
                self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_parentType') and obj.db_parentType is not None:
            columnMap['parent_type'] = \
                self.convertToDB(obj.db_parentType, 'str', 'char(16)')
        if hasattr(obj, 'db_vistrailId') and obj.db_vistrailId is not None:
            columnMap['vt_id'] = \
                self.convertToDB(obj.db_vistrailId, 'long', 'int')
        if hasattr(obj, 'db_parent') and obj.db_parent is not None:
            columnMap['parent_id'] = \
                self.convertToDB(obj.db_parent, 'long', 'long')
        columnMap.update(global_props)

        if obj.is_new:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        for child in obj.db_ports:
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        
    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['id']
        table = 'connection_tbl'
        whereMap = {}
        orderBy = 'id'

        if id is not None:
            keyStr = self.convertToDB(id, 'long', 'int')
            whereMap['id'] = keyStr
        elif foreignKey is not None:
            whereMap.update(foreignKey)
        elif globalProps is None:
            print '***ERROR: need to specify id or foreign key info'
        if globalProps is not None:
            whereMap.update(globalProps)
        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        list = []
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            keyStr = self.convertToDB(id,'long','int')

            discStr = self.convertToDB('connection','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            res = self.getDao('port').fromSQL(db, None, foreignKey, globalProps)
            ports = res
            
            connection = DBConnection(id=id,
                                      ports=ports)
            connection.is_dirty = False
            list.append(connection)

        return list

    def toSQL(self, db, obj, foreignKey=None, globalProps=None):
        keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if obj.is_dirty:
            columns = ['id']
            table = 'connection_tbl'
            whereMap = {}
            columnMap = {}

            whereMap['id'] = keyStr
            if globalProps is not None:
                whereMap.update(globalProps)
            if foreignKey is not None:
                columnMap.update(foreignKey)

            dbCommand = self.createSQLSelect(table, columns, whereMap)
            data = self.executeSQL(db, dbCommand, True)
            if len(data) <= 0:
                columnMap['id'] = keyStr
                if globalProps is not None:
                    columnMap.update(globalProps)
                dbCommand = self.createSQLInsert(table, columnMap)
            else:
                dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
            lastId = self.executeSQL(db, dbCommand, False)
        

        discStr = self.convertToDB('connection','str','char(16)')
        foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
        for child in obj.db_ports:
            self.getDao('port').toSQL(db, child, foreignKey, globalProps)
        

class DBActionSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props):
        columns = ['id', 'prev_id', 'date', 'session', 'user', 'prune', 'parent_type', 'vt_id', 'parent_id']
        table = 'action'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            prevId = self.convertFromDB(row[1], 'long', 'int')
            date = self.convertFromDB(row[2], 'datetime', 'datetime')
            session = self.convertFromDB(row[3], 'str', 'varchar(1023)')
            user = self.convertFromDB(row[4], 'str', 'varchar(255)')
            prune = self.convertFromDB(row[5], 'int', 'int')
            parentType = self.convertFromDB(row[6], 'str', 'char(16)')
            vistrailId = self.convertFromDB(row[7], 'long', 'int')
            parent = self.convertFromDB(row[8], 'long', 'long')
            
            action = DBAction(prevId=prevId,
                              date=date,
                              session=session,
                              user=user,
                              prune=prune,
                              id=id)
            action.db_parentType = parentType
            action.db_vistrailId = vistrailId
            action.db_parent = parent
            action.is_dirty = False
            res[('action', id)] = action

        return res

    def from_sql_fast(self, obj, all_objects):
        if obj.db_parentType == 'vistrail':
            p = all_objects[('vistrail', obj.db_parent)]
            p.db_add_action(obj)
        elif obj.db_parentType == 'abstraction':
            p = all_objects[('abstraction', obj.db_parent)]
            p.db_add_action(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'prev_id', 'date', 'session', 'user', 'prune', 'parent_type', 'vt_id', 'parent_id']
        table = 'action'
        whereMap = {}
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        columnMap = {}
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            columnMap['id'] = \
                self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_prevId') and obj.db_prevId is not None:
            columnMap['prev_id'] = \
                self.convertToDB(obj.db_prevId, 'long', 'int')
        if hasattr(obj, 'db_date') and obj.db_date is not None:
            columnMap['date'] = \
                self.convertToDB(obj.db_date, 'datetime', 'datetime')
        if hasattr(obj, 'db_session') and obj.db_session is not None:
            columnMap['session'] = \
                self.convertToDB(obj.db_session, 'str', 'varchar(1023)')
        if hasattr(obj, 'db_user') and obj.db_user is not None:
            columnMap['user'] = \
                self.convertToDB(obj.db_user, 'str', 'varchar(255)')
        if hasattr(obj, 'db_prune') and obj.db_prune is not None:
            columnMap['prune'] = \
                self.convertToDB(obj.db_prune, 'int', 'int')
        if hasattr(obj, 'db_parentType') and obj.db_parentType is not None:
            columnMap['parent_type'] = \
                self.convertToDB(obj.db_parentType, 'str', 'char(16)')
        if hasattr(obj, 'db_vistrailId') and obj.db_vistrailId is not None:
            columnMap['vt_id'] = \
                self.convertToDB(obj.db_vistrailId, 'long', 'int')
        if hasattr(obj, 'db_parent') and obj.db_parent is not None:
            columnMap['parent_id'] = \
                self.convertToDB(obj.db_parent, 'long', 'long')
        columnMap.update(global_props)

        if obj.is_new:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        for child in obj.db_annotations:
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        for child in obj.db_operations:
            child.db_action = obj.db_id
        
    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['prev_id', 'date', 'session', 'user', 'prune', 'id']
        table = 'action'
        whereMap = {}
        orderBy = 'id'

        if id is not None:
            keyStr = self.convertToDB(id, 'long', 'int')
            whereMap['id'] = keyStr
        elif foreignKey is not None:
            whereMap.update(foreignKey)
        elif globalProps is None:
            print '***ERROR: need to specify id or foreign key info'
        if globalProps is not None:
            whereMap.update(globalProps)
        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        list = []
        for row in data:
            prevId = self.convertFromDB(row[0], 'long', 'int')
            date = self.convertFromDB(row[1], 'datetime', 'datetime')
            session = self.convertFromDB(row[2], 'str', 'varchar(1023)')
            user = self.convertFromDB(row[3], 'str', 'varchar(255)')
            prune = self.convertFromDB(row[4], 'int', 'int')
            id = self.convertFromDB(row[5], 'long', 'int')
            keyStr = self.convertToDB(id,'long','int')

            discStr = self.convertToDB('action','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            res = self.getDao('annotation').fromSQL(db, None, foreignKey, globalProps)
            annotations = res
            
            operations = []
            
            foreignKey = {'action_id': keyStr}
            res = self.getDao('add').fromSQL(db, None, foreignKey, globalProps)
            operations.extend(res)
            
            foreignKey = {'action_id': keyStr}
            res = self.getDao('delete').fromSQL(db, None, foreignKey, globalProps)
            operations.extend(res)
            
            foreignKey = {'action_id': keyStr}
            res = self.getDao('change').fromSQL(db, None, foreignKey, globalProps)
            operations.extend(res)
            
            action = DBAction(id=id,
                              prevId=prevId,
                              date=date,
                              session=session,
                              user=user,
                              prune=prune,
                              annotations=annotations,
                              operations=operations)
            action.is_dirty = False
            list.append(action)

        return list

    def toSQL(self, db, obj, foreignKey=None, globalProps=None):
        keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if obj.is_dirty:
            columns = ['id']
            table = 'action'
            whereMap = {}
            columnMap = {}

            whereMap['id'] = keyStr
            if globalProps is not None:
                whereMap.update(globalProps)
            if obj.db_prevId is not None:
                columnMap['prev_id'] = \
                    self.convertToDB(obj.db_prevId, 'long', 'int')
            if obj.db_date is not None:
                columnMap['date'] = \
                    self.convertToDB(obj.db_date, 'datetime', 'datetime')
            if obj.db_session is not None:
                columnMap['session'] = \
                    self.convertToDB(obj.db_session, 'str', 'varchar(1023)')
            if obj.db_user is not None:
                columnMap['user'] = \
                    self.convertToDB(obj.db_user, 'str', 'varchar(255)')
            if obj.db_prune is not None:
                columnMap['prune'] = \
                    self.convertToDB(obj.db_prune, 'int', 'int')
            if foreignKey is not None:
                columnMap.update(foreignKey)

            dbCommand = self.createSQLSelect(table, columns, whereMap)
            data = self.executeSQL(db, dbCommand, True)
            if len(data) <= 0:
                columnMap['id'] = keyStr
                if globalProps is not None:
                    columnMap.update(globalProps)
                dbCommand = self.createSQLInsert(table, columnMap)
            else:
                dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
            lastId = self.executeSQL(db, dbCommand, False)
        

        discStr = self.convertToDB('action','str','char(16)')
        foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
        for child in obj.db_annotations:
            self.getDao('annotation').toSQL(db, child, foreignKey, globalProps)
        
        for child in obj.db_operations:
            if child.vtType == 'add':
                foreignKey = {'action_id' : keyStr}
                self.getDao('add').toSQL(db, child, foreignKey, globalProps)
            elif child.vtType == 'delete':
                foreignKey = {'action_id' : keyStr}
                self.getDao('delete').toSQL(db, child, foreignKey, globalProps)
            elif child.vtType == 'change':
                foreignKey = {'action_id' : keyStr}
                self.getDao('change').toSQL(db, child, foreignKey, globalProps)
        

class DBDeleteSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props):
        columns = ['id', 'what', 'object_id', 'par_obj_id', 'par_obj_type', 'action_id', 'vt_id']
        table = 'delete_tbl'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            what = self.convertFromDB(row[1], 'str', 'varchar(255)')
            objectId = self.convertFromDB(row[2], 'long', 'int')
            parentObjId = self.convertFromDB(row[3], 'long', 'int')
            parentObjType = self.convertFromDB(row[4], 'str', 'char(16)')
            action = self.convertFromDB(row[5], 'long', 'int')
            vistrailId = self.convertFromDB(row[6], 'long', 'int')
            
            delete = DBDelete(what=what,
                              objectId=objectId,
                              parentObjId=parentObjId,
                              parentObjType=parentObjType,
                              id=id)
            delete.db_action = action
            delete.db_vistrailId = vistrailId
            delete.is_dirty = False
            res[('delete', id)] = delete

        return res

    def from_sql_fast(self, obj, all_objects):
        p = all_objects[('action', obj.db_action)]
        p.db_add_operation(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'what', 'object_id', 'par_obj_id', 'par_obj_type', 'action_id', 'vt_id']
        table = 'delete_tbl'
        whereMap = {}
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        columnMap = {}
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            columnMap['id'] = \
                self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_what') and obj.db_what is not None:
            columnMap['what'] = \
                self.convertToDB(obj.db_what, 'str', 'varchar(255)')
        if hasattr(obj, 'db_objectId') and obj.db_objectId is not None:
            columnMap['object_id'] = \
                self.convertToDB(obj.db_objectId, 'long', 'int')
        if hasattr(obj, 'db_parentObjId') and obj.db_parentObjId is not None:
            columnMap['par_obj_id'] = \
                self.convertToDB(obj.db_parentObjId, 'long', 'int')
        if hasattr(obj, 'db_parentObjType') and obj.db_parentObjType is not None:
            columnMap['par_obj_type'] = \
                self.convertToDB(obj.db_parentObjType, 'str', 'char(16)')
        if hasattr(obj, 'db_action') and obj.db_action is not None:
            columnMap['action_id'] = \
                self.convertToDB(obj.db_action, 'long', 'int')
        if hasattr(obj, 'db_vistrailId') and obj.db_vistrailId is not None:
            columnMap['vt_id'] = \
                self.convertToDB(obj.db_vistrailId, 'long', 'int')
        columnMap.update(global_props)

        if obj.is_new:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        pass
        
    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['what', 'object_id', 'par_obj_id', 'par_obj_type', 'id']
        table = 'delete_tbl'
        whereMap = {}
        orderBy = 'id'

        if id is not None:
            keyStr = self.convertToDB(id, 'long', 'int')
            whereMap['id'] = keyStr
        elif foreignKey is not None:
            whereMap.update(foreignKey)
        elif globalProps is None:
            print '***ERROR: need to specify id or foreign key info'
        if globalProps is not None:
            whereMap.update(globalProps)
        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        list = []
        for row in data:
            what = self.convertFromDB(row[0], 'str', 'varchar(255)')
            objectId = self.convertFromDB(row[1], 'long', 'int')
            parentObjId = self.convertFromDB(row[2], 'long', 'int')
            parentObjType = self.convertFromDB(row[3], 'str', 'char(16)')
            id = self.convertFromDB(row[4], 'long', 'int')
            keyStr = self.convertToDB(id,'long','int')

            delete = DBDelete(id=id,
                              what=what,
                              objectId=objectId,
                              parentObjId=parentObjId,
                              parentObjType=parentObjType)
            delete.is_dirty = False
            list.append(delete)

        return list

    def toSQL(self, db, obj, foreignKey=None, globalProps=None):
        keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if obj.is_dirty:
            columns = ['id']
            table = 'delete_tbl'
            whereMap = {}
            columnMap = {}

            whereMap['id'] = keyStr
            if globalProps is not None:
                whereMap.update(globalProps)
            if obj.db_what is not None:
                columnMap['what'] = \
                    self.convertToDB(obj.db_what, 'str', 'varchar(255)')
            if obj.db_objectId is not None:
                columnMap['object_id'] = \
                    self.convertToDB(obj.db_objectId, 'long', 'int')
            if obj.db_parentObjId is not None:
                columnMap['par_obj_id'] = \
                    self.convertToDB(obj.db_parentObjId, 'long', 'int')
            if obj.db_parentObjType is not None:
                columnMap['par_obj_type'] = \
                    self.convertToDB(obj.db_parentObjType, 'str', 'char(16)')
            if foreignKey is not None:
                columnMap.update(foreignKey)

            dbCommand = self.createSQLSelect(table, columns, whereMap)
            data = self.executeSQL(db, dbCommand, True)
            if len(data) <= 0:
                columnMap['id'] = keyStr
                if globalProps is not None:
                    columnMap.update(globalProps)
                dbCommand = self.createSQLInsert(table, columnMap)
            else:
                dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
            lastId = self.executeSQL(db, dbCommand, False)
        


class DBVistrailSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props):
        columns = ['id', 'version', 'name']
        table = 'vistrail'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            global_props['vt_id'] = self.convertToDB(id, 'long', 'int')
            version = self.convertFromDB(row[1], 'str', 'char(16)')
            name = self.convertFromDB(row[2], 'str', 'varchar(255)')
            
            vistrail = DBVistrail(version=version,
                                  name=name,
                                  id=id)
            vistrail.is_dirty = False
            res[('vistrail', id)] = vistrail

        return res

    def from_sql_fast(self, obj, all_objects):
        pass
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'version', 'name']
        table = 'vistrail'
        whereMap = {}
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        columnMap = {}
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            columnMap['id'] = \
                self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_version') and obj.db_version is not None:
            columnMap['version'] = \
                self.convertToDB(obj.db_version, 'str', 'char(16)')
        if hasattr(obj, 'db_name') and obj.db_name is not None:
            columnMap['name'] = \
                self.convertToDB(obj.db_name, 'str', 'varchar(255)')
        columnMap.update(global_props)

        if obj.is_new:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        if obj.db_id is None:
            obj.db_id = lastId
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        global_props['vt_id'] = self.convertToDB(obj.db_id, 'long', 'int')
        
    def to_sql_fast(self, obj, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        for child in obj.db_actions:
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        for child in obj.db_tags:
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        for child in obj.db_abstractions:
            child.db_vistrail = obj.db_id
        
    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['version', 'name', 'id']
        table = 'vistrail'
        whereMap = {}
        orderBy = 'id'

        if id is not None:
            keyStr = self.convertToDB(id, 'long', 'int')
            whereMap['id'] = keyStr
        elif foreignKey is not None:
            whereMap.update(foreignKey)
        elif globalProps is None:
            print '***ERROR: need to specify id or foreign key info'
        if globalProps is not None:
            whereMap.update(globalProps)
        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        list = []
        for row in data:
            version = self.convertFromDB(row[0], 'str', 'char(16)')
            name = self.convertFromDB(row[1], 'str', 'varchar(255)')
            id = self.convertFromDB(row[2], 'long', 'int')
            if globalProps is None:
                globalProps = {}
            globalProps['vt_id'] = self.convertToDB(id, 'long', 'int')
            keyStr = self.convertToDB(id,'long','int')

            discStr = self.convertToDB('vistrail','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            res = self.getDao('action').fromSQL(db, None, foreignKey, globalProps)
            actions = res
            
            discStr = self.convertToDB('vistrail','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            res = self.getDao('tag').fromSQL(db, None, foreignKey, globalProps)
            tags = res
            
            foreignKey = {'vt_id': keyStr}
            res = self.getDao('abstraction').fromSQL(db, None, foreignKey, globalProps)
            abstractions = res
            
            vistrail = DBVistrail(id=id,
                                  version=version,
                                  name=name,
                                  actions=actions,
                                  tags=tags,
                                  abstractions=abstractions)
            vistrail.is_dirty = False
            list.append(vistrail)

        return list

    def toSQL(self, db, obj, foreignKey=None, globalProps=None):
        keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if obj.is_dirty:
            columns = ['id']
            table = 'vistrail'
            whereMap = {}
            columnMap = {}

            whereMap['id'] = keyStr
            if globalProps is not None:
                whereMap.update(globalProps)
            if obj.db_version is not None:
                columnMap['version'] = \
                    self.convertToDB(obj.db_version, 'str', 'char(16)')
            if obj.db_name is not None:
                columnMap['name'] = \
                    self.convertToDB(obj.db_name, 'str', 'varchar(255)')
            if foreignKey is not None:
                columnMap.update(foreignKey)

            dbCommand = self.createSQLSelect(table, columns, whereMap)
            data = self.executeSQL(db, dbCommand, True)
            if len(data) <= 0:
                if obj.db_id is not None:
                    columnMap['id'] = keyStr
                if globalProps is not None:
                    columnMap.update(globalProps)
                dbCommand = self.createSQLInsert(table, columnMap)
            else:
                dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
            lastId = self.executeSQL(db, dbCommand, False)
            if obj.db_id is None:
                obj.db_id = lastId
                keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            if globalProps is None:
                globalProps = {}
            globalProps['vt_id'] = self.convertToDB(obj.db_id, 'long', 'int')
        

        discStr = self.convertToDB('vistrail','str','char(16)')
        foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
        for child in obj.db_actions:
            self.getDao('action').toSQL(db, child, foreignKey, globalProps)
        
        discStr = self.convertToDB('vistrail','str','char(16)')
        foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
        for child in obj.db_tags:
            self.getDao('tag').toSQL(db, child, foreignKey, globalProps)
        
        foreignKey = {'vt_id': keyStr}
        for child in obj.db_abstractions:
            self.getDao('abstraction').toSQL(db, child, foreignKey, globalProps)
        

class DBModuleExecSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props):
        columns = ['id', 'ts_start', 'ts_end', 'module_id', 'module_name', 'machine_id', 'wf_exec_id', 'vt_id']
        table = 'module_exec'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            ts_start = self.convertFromDB(row[1], 'datetime', 'datetime')
            ts_end = self.convertFromDB(row[2], 'datetime', 'datetime')
            module_id = self.convertFromDB(row[3], 'long', 'int')
            module_name = self.convertFromDB(row[4], 'str', 'varchar(255)')
            machine_id = self.convertFromDB(row[5], 'long', 'int')
            workflow_exec = self.convertFromDB(row[6], 'long', 'int')
            vistrailId = self.convertFromDB(row[7], 'long', 'int')
            
            module_exec = DBModuleExec(ts_start=ts_start,
                                       ts_end=ts_end,
                                       module_id=module_id,
                                       module_name=module_name,
                                       id=id)
            module_exec.db_machine_id = machine_id
            module_exec.db_workflow_exec = workflow_exec
            module_exec.db_vistrailId = vistrailId
            module_exec.is_dirty = False
            res[('module_exec', id)] = module_exec

        return res

    def from_sql_fast(self, obj, all_objects):
        p = all_objects[('machine', obj.db_machine_id)]
        p.db_add_module_exec(obj)
        p = all_objects[('workflow_exec', obj.db_workflow_exec)]
        p.db_add_module_exec(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'ts_start', 'ts_end', 'module_id', 'module_name', 'machine_id', 'wf_exec_id', 'vt_id']
        table = 'module_exec'
        whereMap = {}
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        columnMap = {}
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            columnMap['id'] = \
                self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_ts_start') and obj.db_ts_start is not None:
            columnMap['ts_start'] = \
                self.convertToDB(obj.db_ts_start, 'datetime', 'datetime')
        if hasattr(obj, 'db_ts_end') and obj.db_ts_end is not None:
            columnMap['ts_end'] = \
                self.convertToDB(obj.db_ts_end, 'datetime', 'datetime')
        if hasattr(obj, 'db_module_id') and obj.db_module_id is not None:
            columnMap['module_id'] = \
                self.convertToDB(obj.db_module_id, 'long', 'int')
        if hasattr(obj, 'db_module_name') and obj.db_module_name is not None:
            columnMap['module_name'] = \
                self.convertToDB(obj.db_module_name, 'str', 'varchar(255)')
        if hasattr(obj, 'db_machine_id') and obj.db_machine_id is not None:
            columnMap['machine_id'] = \
                self.convertToDB(obj.db_machine_id, 'long', 'int')
        if hasattr(obj, 'db_workflow_exec') and obj.db_workflow_exec is not None:
            columnMap['wf_exec_id'] = \
                self.convertToDB(obj.db_workflow_exec, 'long', 'int')
        if hasattr(obj, 'db_vistrailId') and obj.db_vistrailId is not None:
            columnMap['vt_id'] = \
                self.convertToDB(obj.db_vistrailId, 'long', 'int')
        columnMap.update(global_props)

        if obj.is_new:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        for child in obj.db_annotations:
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        
    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['ts_start', 'ts_end', 'module_id', 'module_name', 'id']
        table = 'module_exec'
        whereMap = {}
        orderBy = 'id'

        if id is not None:
            keyStr = self.convertToDB(id, 'long', 'int')
            whereMap['id'] = keyStr
        elif foreignKey is not None:
            whereMap.update(foreignKey)
        elif globalProps is None:
            print '***ERROR: need to specify id or foreign key info'
        if globalProps is not None:
            whereMap.update(globalProps)
        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy)
        data = self.executeSQL(db, dbCommand, True)
        list = []
        for row in data:
            ts_start = self.convertFromDB(row[0], 'datetime', 'datetime')
            ts_end = self.convertFromDB(row[1], 'datetime', 'datetime')
            module_id = self.convertFromDB(row[2], 'long', 'int')
            module_name = self.convertFromDB(row[3], 'str', 'varchar(255)')
            id = self.convertFromDB(row[4], 'long', 'int')
            keyStr = self.convertToDB(id,'long','int')

            discStr = self.convertToDB('module_exec','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            res = self.getDao('annotation').fromSQL(db, None, foreignKey, globalProps)
            annotations = res
            
            module_exec = DBModuleExec(id=id,
                                       ts_start=ts_start,
                                       ts_end=ts_end,
                                       module_id=module_id,
                                       module_name=module_name,
                                       annotations=annotations)
            module_exec.is_dirty = False
            list.append(module_exec)

        return list

    def toSQL(self, db, obj, foreignKey=None, globalProps=None):
        keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if obj.is_dirty:
            columns = ['id']
            table = 'module_exec'
            whereMap = {}
            columnMap = {}

            whereMap['id'] = keyStr
            if globalProps is not None:
                whereMap.update(globalProps)
            if obj.db_ts_start is not None:
                columnMap['ts_start'] = \
                    self.convertToDB(obj.db_ts_start, 'datetime', 'datetime')
            if obj.db_ts_end is not None:
                columnMap['ts_end'] = \
                    self.convertToDB(obj.db_ts_end, 'datetime', 'datetime')
            if obj.db_module_id is not None:
                columnMap['module_id'] = \
                    self.convertToDB(obj.db_module_id, 'long', 'int')
            if obj.db_module_name is not None:
                columnMap['module_name'] = \
                    self.convertToDB(obj.db_module_name, 'str', 'varchar(255)')
            if foreignKey is not None:
                columnMap.update(foreignKey)

            dbCommand = self.createSQLSelect(table, columns, whereMap)
            data = self.executeSQL(db, dbCommand, True)
            if len(data) <= 0:
                columnMap['id'] = keyStr
                if globalProps is not None:
                    columnMap.update(globalProps)
                dbCommand = self.createSQLInsert(table, columnMap)
            else:
                dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
            lastId = self.executeSQL(db, dbCommand, False)
        

        discStr = self.convertToDB('module_exec','str','char(16)')
        foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
        for child in obj.db_annotations:
            self.getDao('annotation').toSQL(db, child, foreignKey, globalProps)
        

"""generated automatically by auto_dao.py"""

class SQLDAOListBase(dict):

    def __init__(self, daos=None):
        if daos is not None:
            dict.update(self, daos)

        if 'portSpec' not in self:
            self['portSpec'] = DBPortSpecSQLDAOBase(self)
        if 'module' not in self:
            self['module'] = DBModuleSQLDAOBase(self)
        if 'tag' not in self:
            self['tag'] = DBTagSQLDAOBase(self)
        if 'port' not in self:
            self['port'] = DBPortSQLDAOBase(self)
        if 'log' not in self:
            self['log'] = DBLogSQLDAOBase(self)
        if 'machine' not in self:
            self['machine'] = DBMachineSQLDAOBase(self)
        if 'add' not in self:
            self['add'] = DBAddSQLDAOBase(self)
        if 'other' not in self:
            self['other'] = DBOtherSQLDAOBase(self)
        if 'location' not in self:
            self['location'] = DBLocationSQLDAOBase(self)
        if 'workflow_exec' not in self:
            self['workflow_exec'] = DBWorkflowExecSQLDAOBase(self)
        if 'function' not in self:
            self['function'] = DBFunctionSQLDAOBase(self)
        if 'abstraction' not in self:
            self['abstraction'] = DBAbstractionSQLDAOBase(self)
        if 'workflow' not in self:
            self['workflow'] = DBWorkflowSQLDAOBase(self)
        if 'abstractionRef' not in self:
            self['abstractionRef'] = DBAbstractionRefSQLDAOBase(self)
        if 'annotation' not in self:
            self['annotation'] = DBAnnotationSQLDAOBase(self)
        if 'change' not in self:
            self['change'] = DBChangeSQLDAOBase(self)
        if 'parameter' not in self:
            self['parameter'] = DBParameterSQLDAOBase(self)
        if 'connection' not in self:
            self['connection'] = DBConnectionSQLDAOBase(self)
        if 'action' not in self:
            self['action'] = DBActionSQLDAOBase(self)
        if 'delete' not in self:
            self['delete'] = DBDeleteSQLDAOBase(self)
        if 'vistrail' not in self:
            self['vistrail'] = DBVistrailSQLDAOBase(self)
        if 'module_exec' not in self:
            self['module_exec'] = DBModuleExecSQLDAOBase(self)
