
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
from db.versions.v0_9_3.domain import *

class DBPortSpecSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props,lock=False):
        columns = ['id', 'name', 'type', 'spec', 'parent_type', 'entity_id', 'entity_type', 'parent_id']
        table = 'port_spec'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy, lock)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            name = self.convertFromDB(row[1], 'str', 'varchar(22)')
            type = self.convertFromDB(row[2], 'str', 'varchar(255)')
            spec = self.convertFromDB(row[3], 'str', 'varchar(255)')
            parentType = self.convertFromDB(row[4], 'str', 'char(16)')
            entity_id = self.convertFromDB(row[5], 'long', 'int')
            entity_type = self.convertFromDB(row[6], 'str', 'char(16)')
            parent = self.convertFromDB(row[7], 'long', 'long')
            
            portSpec = DBPortSpec(name=name,
                                  type=type,
                                  spec=spec,
                                  id=id)
            portSpec.db_parentType = parentType
            portSpec.db_entity_id = entity_id
            portSpec.db_entity_type = entity_type
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
        columns = ['id', 'name', 'type', 'spec', 'parent_type', 'entity_id', 'entity_type', 'parent_id']
        table = 'port_spec'
        whereMap = {}
        whereMap.update(global_props)
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
        if hasattr(obj, 'db_entity_id') and obj.db_entity_id is not None:
            columnMap['entity_id'] = \
                self.convertToDB(obj.db_entity_id, 'long', 'int')
        if hasattr(obj, 'db_entity_type') and obj.db_entity_type is not None:
            columnMap['entity_type'] = \
                self.convertToDB(obj.db_entity_type, 'str', 'char(16)')
        if hasattr(obj, 'db_parent') and obj.db_parent is not None:
            columnMap['parent_id'] = \
                self.convertToDB(obj.db_parent, 'long', 'long')
        columnMap.update(global_props)

        if obj.is_new or do_copy:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        pass
        
    def delete_sql_column(self, db, obj, global_props):
        table = 'port_spec'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        dbCommand = self.createSQLDelete(table, whereMap)
        self.executeSQL(db, dbCommand, False)

class DBModuleSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props,lock=False):
        columns = ['id', 'cache', 'name', 'namespace', 'package', 'version', 'tag', 'parent_type', 'entity_id', 'entity_type', 'parent_id']
        table = 'module'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy, lock)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            cache = self.convertFromDB(row[1], 'int', 'int')
            name = self.convertFromDB(row[2], 'str', 'varchar(255)')
            namespace = self.convertFromDB(row[3], 'str', 'varchar(255)')
            package = self.convertFromDB(row[4], 'str', 'varchar(511)')
            version = self.convertFromDB(row[5], 'str', 'varchar(255)')
            tag = self.convertFromDB(row[6], 'str', 'varchar(255)')
            parentType = self.convertFromDB(row[7], 'str', 'char(16)')
            entity_id = self.convertFromDB(row[8], 'long', 'int')
            entity_type = self.convertFromDB(row[9], 'str', 'char(16)')
            parent = self.convertFromDB(row[10], 'long', 'long')
            
            module = DBModule(cache=cache,
                              name=name,
                              namespace=namespace,
                              package=package,
                              version=version,
                              tag=tag,
                              id=id)
            module.db_parentType = parentType
            module.db_entity_id = entity_id
            module.db_entity_type = entity_type
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
        columns = ['id', 'cache', 'name', 'namespace', 'package', 'version', 'tag', 'parent_type', 'entity_id', 'entity_type', 'parent_id']
        table = 'module'
        whereMap = {}
        whereMap.update(global_props)
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
        if hasattr(obj, 'db_name') and obj.db_name is not None:
            columnMap['name'] = \
                self.convertToDB(obj.db_name, 'str', 'varchar(255)')
        if hasattr(obj, 'db_namespace') and obj.db_namespace is not None:
            columnMap['namespace'] = \
                self.convertToDB(obj.db_namespace, 'str', 'varchar(255)')
        if hasattr(obj, 'db_package') and obj.db_package is not None:
            columnMap['package'] = \
                self.convertToDB(obj.db_package, 'str', 'varchar(511)')
        if hasattr(obj, 'db_version') and obj.db_version is not None:
            columnMap['version'] = \
                self.convertToDB(obj.db_version, 'str', 'varchar(255)')
        if hasattr(obj, 'db_tag') and obj.db_tag is not None:
            columnMap['tag'] = \
                self.convertToDB(obj.db_tag, 'str', 'varchar(255)')
        if hasattr(obj, 'db_parentType') and obj.db_parentType is not None:
            columnMap['parent_type'] = \
                self.convertToDB(obj.db_parentType, 'str', 'char(16)')
        if hasattr(obj, 'db_entity_id') and obj.db_entity_id is not None:
            columnMap['entity_id'] = \
                self.convertToDB(obj.db_entity_id, 'long', 'int')
        if hasattr(obj, 'db_entity_type') and obj.db_entity_type is not None:
            columnMap['entity_type'] = \
                self.convertToDB(obj.db_entity_type, 'str', 'char(16)')
        if hasattr(obj, 'db_parent') and obj.db_parent is not None:
            columnMap['parent_id'] = \
                self.convertToDB(obj.db_parent, 'long', 'long')
        columnMap.update(global_props)

        if obj.is_new or do_copy:
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
        
    def delete_sql_column(self, db, obj, global_props):
        table = 'module'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        dbCommand = self.createSQLDelete(table, whereMap)
        self.executeSQL(db, dbCommand, False)

class DBTagSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props,lock=False):
        columns = ['id', 'name', 'parent_type', 'entity_id', 'entity_type', 'parent_id']
        table = 'tag'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy, lock)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            name = self.convertFromDB(row[1], 'str', 'varchar(255)')
            parentType = self.convertFromDB(row[2], 'str', 'char(16)')
            entity_id = self.convertFromDB(row[3], 'long', 'int')
            entity_type = self.convertFromDB(row[4], 'str', 'char(16)')
            parent = self.convertFromDB(row[5], 'long', 'long')
            
            tag = DBTag(name=name,
                        id=id)
            tag.db_parentType = parentType
            tag.db_entity_id = entity_id
            tag.db_entity_type = entity_type
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
        columns = ['id', 'name', 'parent_type', 'entity_id', 'entity_type', 'parent_id']
        table = 'tag'
        whereMap = {}
        whereMap.update(global_props)
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
        if hasattr(obj, 'db_entity_id') and obj.db_entity_id is not None:
            columnMap['entity_id'] = \
                self.convertToDB(obj.db_entity_id, 'long', 'int')
        if hasattr(obj, 'db_entity_type') and obj.db_entity_type is not None:
            columnMap['entity_type'] = \
                self.convertToDB(obj.db_entity_type, 'str', 'char(16)')
        if hasattr(obj, 'db_parent') and obj.db_parent is not None:
            columnMap['parent_id'] = \
                self.convertToDB(obj.db_parent, 'long', 'long')
        columnMap.update(global_props)

        if obj.is_new or do_copy:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        pass
        
    def delete_sql_column(self, db, obj, global_props):
        table = 'tag'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        dbCommand = self.createSQLDelete(table, whereMap)
        self.executeSQL(db, dbCommand, False)

class DBPortSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props,lock=False):
        columns = ['id', 'type', 'moduleId', 'moduleName', 'name', 'spec', 'parent_type', 'entity_id', 'entity_type', 'parent_id']
        table = 'port'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy, lock)
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
            entity_id = self.convertFromDB(row[7], 'long', 'int')
            entity_type = self.convertFromDB(row[8], 'str', 'char(16)')
            parent = self.convertFromDB(row[9], 'long', 'long')
            
            port = DBPort(type=type,
                          moduleId=moduleId,
                          moduleName=moduleName,
                          name=name,
                          spec=spec,
                          id=id)
            port.db_parentType = parentType
            port.db_entity_id = entity_id
            port.db_entity_type = entity_type
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
        columns = ['id', 'type', 'moduleId', 'moduleName', 'name', 'spec', 'parent_type', 'entity_id', 'entity_type', 'parent_id']
        table = 'port'
        whereMap = {}
        whereMap.update(global_props)
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
        if hasattr(obj, 'db_entity_id') and obj.db_entity_id is not None:
            columnMap['entity_id'] = \
                self.convertToDB(obj.db_entity_id, 'long', 'int')
        if hasattr(obj, 'db_entity_type') and obj.db_entity_type is not None:
            columnMap['entity_type'] = \
                self.convertToDB(obj.db_entity_type, 'str', 'char(16)')
        if hasattr(obj, 'db_parent') and obj.db_parent is not None:
            columnMap['parent_id'] = \
                self.convertToDB(obj.db_parent, 'long', 'long')
        columnMap.update(global_props)

        if obj.is_new or do_copy:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        pass
        
    def delete_sql_column(self, db, obj, global_props):
        table = 'port'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        dbCommand = self.createSQLDelete(table, whereMap)
        self.executeSQL(db, dbCommand, False)

class DBGroupSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props,lock=False):
        columns = ['id', 'cache', 'name', 'namespace', 'package', 'version', 'tag', 'parent_type', 'entity_id', 'entity_type', 'parent_id']
        table = 'group_tbl'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy, lock)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            cache = self.convertFromDB(row[1], 'int', 'int')
            name = self.convertFromDB(row[2], 'str', 'varchar(255)')
            namespace = self.convertFromDB(row[3], 'str', 'varchar(255)')
            package = self.convertFromDB(row[4], 'str', 'varchar(511)')
            version = self.convertFromDB(row[5], 'str', 'varchar(255)')
            tag = self.convertFromDB(row[6], 'str', 'varchar(255)')
            parentType = self.convertFromDB(row[7], 'str', 'char(16)')
            entity_id = self.convertFromDB(row[8], 'long', 'int')
            entity_type = self.convertFromDB(row[9], 'str', 'char(16)')
            parent = self.convertFromDB(row[10], 'long', 'long')
            
            group = DBGroup(cache=cache,
                            name=name,
                            namespace=namespace,
                            package=package,
                            version=version,
                            tag=tag,
                            id=id)
            group.db_parentType = parentType
            group.db_entity_id = entity_id
            group.db_entity_type = entity_type
            group.db_parent = parent
            group.is_dirty = False
            res[('group', id)] = group

        return res

    def from_sql_fast(self, obj, all_objects):
        if obj.db_parentType == 'workflow':
            p = all_objects[('workflow', obj.db_parent)]
            p.db_add_parent(obj)
        elif obj.db_parentType == 'add':
            p = all_objects[('add', obj.db_parent)]
            p.db_add_data(obj)
        elif obj.db_parentType == 'change':
            p = all_objects[('change', obj.db_parent)]
            p.db_add_data(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'cache', 'name', 'namespace', 'package', 'version', 'tag', 'parent_type', 'entity_id', 'entity_type', 'parent_id']
        table = 'group_tbl'
        whereMap = {}
        whereMap.update(global_props)
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
        if hasattr(obj, 'db_name') and obj.db_name is not None:
            columnMap['name'] = \
                self.convertToDB(obj.db_name, 'str', 'varchar(255)')
        if hasattr(obj, 'db_namespace') and obj.db_namespace is not None:
            columnMap['namespace'] = \
                self.convertToDB(obj.db_namespace, 'str', 'varchar(255)')
        if hasattr(obj, 'db_package') and obj.db_package is not None:
            columnMap['package'] = \
                self.convertToDB(obj.db_package, 'str', 'varchar(511)')
        if hasattr(obj, 'db_version') and obj.db_version is not None:
            columnMap['version'] = \
                self.convertToDB(obj.db_version, 'str', 'varchar(255)')
        if hasattr(obj, 'db_tag') and obj.db_tag is not None:
            columnMap['tag'] = \
                self.convertToDB(obj.db_tag, 'str', 'varchar(255)')
        if hasattr(obj, 'db_parentType') and obj.db_parentType is not None:
            columnMap['parent_type'] = \
                self.convertToDB(obj.db_parentType, 'str', 'char(16)')
        if hasattr(obj, 'db_entity_id') and obj.db_entity_id is not None:
            columnMap['entity_id'] = \
                self.convertToDB(obj.db_entity_id, 'long', 'int')
        if hasattr(obj, 'db_entity_type') and obj.db_entity_type is not None:
            columnMap['entity_type'] = \
                self.convertToDB(obj.db_entity_type, 'str', 'char(16)')
        if hasattr(obj, 'db_parent') and obj.db_parent is not None:
            columnMap['parent_id'] = \
                self.convertToDB(obj.db_parent, 'long', 'long')
        columnMap.update(global_props)

        if obj.is_new or do_copy:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        if obj.db_workflow is not None:
            child = obj.db_workflow
            child.db_parent = obj.db_id
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
        
    def delete_sql_column(self, db, obj, global_props):
        table = 'group_tbl'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        dbCommand = self.createSQLDelete(table, whereMap)
        self.executeSQL(db, dbCommand, False)

class DBLogSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props,lock=False):
        columns = ['id', 'entity_type', 'version', 'name', 'last_modified', 'vistrail_id']
        table = 'log_tbl'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy, lock)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            if not global_props.has_key('entity_id'):
                global_props['entity_id'] = self.convertToDB(id, 'long', 'int')
            entity_type = self.convertFromDB(row[1], 'str', 'char(16)')
            if not global_props.has_key('entity_type'):
                global_props['entity_type'] = self.convertToDB(entity_type, 'str', 'char(16)')
            version = self.convertFromDB(row[2], 'str', 'char(16)')
            name = self.convertFromDB(row[3], 'str', 'varchar(255)')
            last_modified = self.convertFromDB(row[4], 'datetime', 'datetime')
            vistrail_id = self.convertFromDB(row[5], 'long', 'int')
            
            log = DBLog(entity_type=entity_type,
                        version=version,
                        name=name,
                        last_modified=last_modified,
                        vistrail_id=vistrail_id,
                        id=id)
            log.is_dirty = False
            res[('log', id)] = log

        return res

    def from_sql_fast(self, obj, all_objects):
        pass
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'entity_type', 'version', 'name', 'last_modified', 'vistrail_id']
        table = 'log_tbl'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        columnMap = {}
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            columnMap['id'] = \
                self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_entity_type') and obj.db_entity_type is not None:
            columnMap['entity_type'] = \
                self.convertToDB(obj.db_entity_type, 'str', 'char(16)')
        if hasattr(obj, 'db_version') and obj.db_version is not None:
            columnMap['version'] = \
                self.convertToDB(obj.db_version, 'str', 'char(16)')
        if hasattr(obj, 'db_name') and obj.db_name is not None:
            columnMap['name'] = \
                self.convertToDB(obj.db_name, 'str', 'varchar(255)')
        if hasattr(obj, 'db_last_modified') and obj.db_last_modified is not None:
            columnMap['last_modified'] = \
                self.convertToDB(obj.db_last_modified, 'datetime', 'datetime')
        if hasattr(obj, 'db_vistrail_id') and obj.db_vistrail_id is not None:
            columnMap['vistrail_id'] = \
                self.convertToDB(obj.db_vistrail_id, 'long', 'int')
        columnMap.update(global_props)

        if obj.is_new or do_copy:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        if obj.db_id is None:
            obj.db_id = lastId
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_entity_type') and obj.db_entity_type is not None:
            global_props['entity_type'] = self.convertToDB(obj.db_entity_type, 'str', 'char(16)')
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            global_props['entity_id'] = self.convertToDB(obj.db_id, 'long', 'int')
        
    def to_sql_fast(self, obj, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        for child in obj.db_workflow_execs:
            child.db_log = obj.db_id
        for child in obj.db_machines:
            child.db_log = obj.db_id
        
    def delete_sql_column(self, db, obj, global_props):
        table = 'log_tbl'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        dbCommand = self.createSQLDelete(table, whereMap)
        self.executeSQL(db, dbCommand, False)

class DBMachineSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props,lock=False):
        columns = ['id', 'name', 'os', 'architecture', 'processor', 'ram', 'vt_id', 'log_id', 'entity_id', 'entity_type']
        table = 'machine'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy, lock)
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
            entity_id = self.convertFromDB(row[8], 'long', 'int')
            entity_type = self.convertFromDB(row[9], 'str', 'char(16)')
            
            machine = DBMachine(name=name,
                                os=os,
                                architecture=architecture,
                                processor=processor,
                                ram=ram,
                                id=id)
            machine.db_vistrailId = vistrailId
            machine.db_log = log
            machine.db_entity_id = entity_id
            machine.db_entity_type = entity_type
            machine.is_dirty = False
            res[('machine', id)] = machine

        return res

    def from_sql_fast(self, obj, all_objects):
        p = all_objects[('log', obj.db_log)]
        p.db_add_machine(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'name', 'os', 'architecture', 'processor', 'ram', 'vt_id', 'log_id', 'entity_id', 'entity_type']
        table = 'machine'
        whereMap = {}
        whereMap.update(global_props)
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
        if hasattr(obj, 'db_entity_id') and obj.db_entity_id is not None:
            columnMap['entity_id'] = \
                self.convertToDB(obj.db_entity_id, 'long', 'int')
        if hasattr(obj, 'db_entity_type') and obj.db_entity_type is not None:
            columnMap['entity_type'] = \
                self.convertToDB(obj.db_entity_type, 'str', 'char(16)')
        columnMap.update(global_props)

        if obj.is_new or do_copy:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        pass
        
    def delete_sql_column(self, db, obj, global_props):
        table = 'machine'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        dbCommand = self.createSQLDelete(table, whereMap)
        self.executeSQL(db, dbCommand, False)

class DBAddSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props,lock=False):
        columns = ['id', 'what', 'object_id', 'par_obj_id', 'par_obj_type', 'action_id', 'entity_id', 'entity_type']
        table = 'add_tbl'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy, lock)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            what = self.convertFromDB(row[1], 'str', 'varchar(255)')
            objectId = self.convertFromDB(row[2], 'long', 'int')
            parentObjId = self.convertFromDB(row[3], 'long', 'int')
            parentObjType = self.convertFromDB(row[4], 'str', 'char(16)')
            action = self.convertFromDB(row[5], 'long', 'int')
            entity_id = self.convertFromDB(row[6], 'long', 'int')
            entity_type = self.convertFromDB(row[7], 'str', 'char(16)')
            
            add = DBAdd(what=what,
                        objectId=objectId,
                        parentObjId=parentObjId,
                        parentObjType=parentObjType,
                        id=id)
            add.db_action = action
            add.db_entity_id = entity_id
            add.db_entity_type = entity_type
            add.is_dirty = False
            res[('add', id)] = add

        return res

    def from_sql_fast(self, obj, all_objects):
        p = all_objects[('action', obj.db_action)]
        p.db_add_operation(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'what', 'object_id', 'par_obj_id', 'par_obj_type', 'action_id', 'entity_id', 'entity_type']
        table = 'add_tbl'
        whereMap = {}
        whereMap.update(global_props)
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
        if hasattr(obj, 'db_entity_id') and obj.db_entity_id is not None:
            columnMap['entity_id'] = \
                self.convertToDB(obj.db_entity_id, 'long', 'int')
        if hasattr(obj, 'db_entity_type') and obj.db_entity_type is not None:
            columnMap['entity_type'] = \
                self.convertToDB(obj.db_entity_type, 'str', 'char(16)')
        columnMap.update(global_props)

        if obj.is_new or do_copy:
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
        
    def delete_sql_column(self, db, obj, global_props):
        table = 'add_tbl'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        dbCommand = self.createSQLDelete(table, whereMap)
        self.executeSQL(db, dbCommand, False)

class DBOtherSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props,lock=False):
        columns = ['id', 'okey', 'value', 'parent_type', 'entity_id', 'entity_type', 'parent_id']
        table = 'other'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy, lock)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            key = self.convertFromDB(row[1], 'str', 'varchar(255)')
            value = self.convertFromDB(row[2], 'str', 'varchar(255)')
            parentType = self.convertFromDB(row[3], 'str', 'char(16)')
            entity_id = self.convertFromDB(row[4], 'long', 'int')
            entity_type = self.convertFromDB(row[5], 'str', 'char(16)')
            parent = self.convertFromDB(row[6], 'long', 'long')
            
            other = DBOther(key=key,
                            value=value,
                            id=id)
            other.db_parentType = parentType
            other.db_entity_id = entity_id
            other.db_entity_type = entity_type
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
        columns = ['id', 'okey', 'value', 'parent_type', 'entity_id', 'entity_type', 'parent_id']
        table = 'other'
        whereMap = {}
        whereMap.update(global_props)
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
        if hasattr(obj, 'db_entity_id') and obj.db_entity_id is not None:
            columnMap['entity_id'] = \
                self.convertToDB(obj.db_entity_id, 'long', 'int')
        if hasattr(obj, 'db_entity_type') and obj.db_entity_type is not None:
            columnMap['entity_type'] = \
                self.convertToDB(obj.db_entity_type, 'str', 'char(16)')
        if hasattr(obj, 'db_parent') and obj.db_parent is not None:
            columnMap['parent_id'] = \
                self.convertToDB(obj.db_parent, 'long', 'long')
        columnMap.update(global_props)

        if obj.is_new or do_copy:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        pass
        
    def delete_sql_column(self, db, obj, global_props):
        table = 'other'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        dbCommand = self.createSQLDelete(table, whereMap)
        self.executeSQL(db, dbCommand, False)

class DBLocationSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props,lock=False):
        columns = ['id', 'x', 'y', 'parent_type', 'entity_id', 'entity_type', 'parent_id']
        table = 'location'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy, lock)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            x = self.convertFromDB(row[1], 'float', 'DECIMAL(18,12)')
            y = self.convertFromDB(row[2], 'float', 'DECIMAL(18,12)')
            parentType = self.convertFromDB(row[3], 'str', 'char(16)')
            entity_id = self.convertFromDB(row[4], 'long', 'int')
            entity_type = self.convertFromDB(row[5], 'str', 'char(16)')
            parent = self.convertFromDB(row[6], 'long', 'long')
            
            location = DBLocation(x=x,
                                  y=y,
                                  id=id)
            location.db_parentType = parentType
            location.db_entity_id = entity_id
            location.db_entity_type = entity_type
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
        elif obj.db_parentType == 'group':
            p = all_objects[('group', obj.db_parent)]
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
        columns = ['id', 'x', 'y', 'parent_type', 'entity_id', 'entity_type', 'parent_id']
        table = 'location'
        whereMap = {}
        whereMap.update(global_props)
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
        if hasattr(obj, 'db_entity_id') and obj.db_entity_id is not None:
            columnMap['entity_id'] = \
                self.convertToDB(obj.db_entity_id, 'long', 'int')
        if hasattr(obj, 'db_entity_type') and obj.db_entity_type is not None:
            columnMap['entity_type'] = \
                self.convertToDB(obj.db_entity_type, 'str', 'char(16)')
        if hasattr(obj, 'db_parent') and obj.db_parent is not None:
            columnMap['parent_id'] = \
                self.convertToDB(obj.db_parent, 'long', 'long')
        columnMap.update(global_props)

        if obj.is_new or do_copy:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        pass
        
    def delete_sql_column(self, db, obj, global_props):
        table = 'location'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        dbCommand = self.createSQLDelete(table, whereMap)
        self.executeSQL(db, dbCommand, False)

class DBParameterSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props,lock=False):
        columns = ['id', 'pos', 'name', 'type', 'val', 'alias', 'parent_type', 'entity_id', 'entity_type', 'parent_id']
        table = 'parameter'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy, lock)
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
            entity_id = self.convertFromDB(row[7], 'long', 'int')
            entity_type = self.convertFromDB(row[8], 'str', 'char(16)')
            parent = self.convertFromDB(row[9], 'long', 'long')
            
            parameter = DBParameter(pos=pos,
                                    name=name,
                                    type=type,
                                    val=val,
                                    alias=alias,
                                    id=id)
            parameter.db_parentType = parentType
            parameter.db_entity_id = entity_id
            parameter.db_entity_type = entity_type
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
        columns = ['id', 'pos', 'name', 'type', 'val', 'alias', 'parent_type', 'entity_id', 'entity_type', 'parent_id']
        table = 'parameter'
        whereMap = {}
        whereMap.update(global_props)
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
        if hasattr(obj, 'db_entity_id') and obj.db_entity_id is not None:
            columnMap['entity_id'] = \
                self.convertToDB(obj.db_entity_id, 'long', 'int')
        if hasattr(obj, 'db_entity_type') and obj.db_entity_type is not None:
            columnMap['entity_type'] = \
                self.convertToDB(obj.db_entity_type, 'str', 'char(16)')
        if hasattr(obj, 'db_parent') and obj.db_parent is not None:
            columnMap['parent_id'] = \
                self.convertToDB(obj.db_parent, 'long', 'long')
        columnMap.update(global_props)

        if obj.is_new or do_copy:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        pass
        
    def delete_sql_column(self, db, obj, global_props):
        table = 'parameter'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        dbCommand = self.createSQLDelete(table, whereMap)
        self.executeSQL(db, dbCommand, False)

class DBPluginDataSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props,lock=False):
        columns = ['id', 'data', 'parent_type', 'entity_id', 'entity_type', 'parent_id']
        table = 'plugin_data'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy, lock)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            data = self.convertFromDB(row[1], 'str', 'varchar(8191)')
            parentType = self.convertFromDB(row[2], 'str', 'char(16)')
            entity_id = self.convertFromDB(row[3], 'long', 'int')
            entity_type = self.convertFromDB(row[4], 'str', 'char(16)')
            parent = self.convertFromDB(row[5], 'long', 'long')
            
            plugin_data = DBPluginData(data=data,
                                       id=id)
            plugin_data.db_parentType = parentType
            plugin_data.db_entity_id = entity_id
            plugin_data.db_entity_type = entity_type
            plugin_data.db_parent = parent
            plugin_data.is_dirty = False
            res[('plugin_data', id)] = plugin_data

        return res

    def from_sql_fast(self, obj, all_objects):
        if obj.db_parentType == 'workflow':
            p = all_objects[('workflow', obj.db_parent)]
            p.db_add_plugin_data(obj)
        elif obj.db_parentType == 'add':
            p = all_objects[('add', obj.db_parent)]
            p.db_add_data(obj)
        elif obj.db_parentType == 'change':
            p = all_objects[('change', obj.db_parent)]
            p.db_add_data(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'data', 'parent_type', 'entity_id', 'entity_type', 'parent_id']
        table = 'plugin_data'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        columnMap = {}
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            columnMap['id'] = \
                self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_data') and obj.db_data is not None:
            columnMap['data'] = \
                self.convertToDB(obj.db_data, 'str', 'varchar(8191)')
        if hasattr(obj, 'db_parentType') and obj.db_parentType is not None:
            columnMap['parent_type'] = \
                self.convertToDB(obj.db_parentType, 'str', 'char(16)')
        if hasattr(obj, 'db_entity_id') and obj.db_entity_id is not None:
            columnMap['entity_id'] = \
                self.convertToDB(obj.db_entity_id, 'long', 'int')
        if hasattr(obj, 'db_entity_type') and obj.db_entity_type is not None:
            columnMap['entity_type'] = \
                self.convertToDB(obj.db_entity_type, 'str', 'char(16)')
        if hasattr(obj, 'db_parent') and obj.db_parent is not None:
            columnMap['parent_id'] = \
                self.convertToDB(obj.db_parent, 'long', 'long')
        columnMap.update(global_props)

        if obj.is_new or do_copy:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        pass
        
    def delete_sql_column(self, db, obj, global_props):
        table = 'plugin_data'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        dbCommand = self.createSQLDelete(table, whereMap)
        self.executeSQL(db, dbCommand, False)

class DBFunctionSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props,lock=False):
        columns = ['id', 'pos', 'name', 'parent_type', 'entity_id', 'entity_type', 'parent_id']
        table = 'function'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy, lock)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            pos = self.convertFromDB(row[1], 'long', 'int')
            name = self.convertFromDB(row[2], 'str', 'varchar(255)')
            parentType = self.convertFromDB(row[3], 'str', 'char(16)')
            entity_id = self.convertFromDB(row[4], 'long', 'int')
            entity_type = self.convertFromDB(row[5], 'str', 'char(16)')
            parent = self.convertFromDB(row[6], 'long', 'long')
            
            function = DBFunction(pos=pos,
                                  name=name,
                                  id=id)
            function.db_parentType = parentType
            function.db_entity_id = entity_id
            function.db_entity_type = entity_type
            function.db_parent = parent
            function.is_dirty = False
            res[('function', id)] = function

        return res

    def from_sql_fast(self, obj, all_objects):
        if obj.db_parentType == 'module':
            p = all_objects[('module', obj.db_parent)]
            p.db_add_function(obj)
        elif obj.db_parentType == 'abstractionRef':
            p = all_objects[('abstractionRef', obj.db_parent)]
            p.db_add_function(obj)
        elif obj.db_parentType == 'group':
            p = all_objects[('group', obj.db_parent)]
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
        columns = ['id', 'pos', 'name', 'parent_type', 'entity_id', 'entity_type', 'parent_id']
        table = 'function'
        whereMap = {}
        whereMap.update(global_props)
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
        if hasattr(obj, 'db_entity_id') and obj.db_entity_id is not None:
            columnMap['entity_id'] = \
                self.convertToDB(obj.db_entity_id, 'long', 'int')
        if hasattr(obj, 'db_entity_type') and obj.db_entity_type is not None:
            columnMap['entity_type'] = \
                self.convertToDB(obj.db_entity_type, 'str', 'char(16)')
        if hasattr(obj, 'db_parent') and obj.db_parent is not None:
            columnMap['parent_id'] = \
                self.convertToDB(obj.db_parent, 'long', 'long')
        columnMap.update(global_props)

        if obj.is_new or do_copy:
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
        
    def delete_sql_column(self, db, obj, global_props):
        table = 'function'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        dbCommand = self.createSQLDelete(table, whereMap)
        self.executeSQL(db, dbCommand, False)

class DBAbstractionSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props,lock=False):
        columns = ['id', 'entity_type', 'name', 'last_modified']
        table = 'abstraction'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy, lock)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            if not global_props.has_key('entity_id'):
                global_props['entity_id'] = self.convertToDB(id, 'long', 'int')
            entity_type = self.convertFromDB(row[1], 'str', 'char(16)')
            if not global_props.has_key('entity_type'):
                global_props['entity_type'] = self.convertToDB(entity_type, 'str', 'char(16)')
            name = self.convertFromDB(row[2], 'str', 'varchar(255)')
            last_modified = self.convertFromDB(row[3], 'datetime', 'datetime')
            
            abstraction = DBAbstraction(entity_type=entity_type,
                                        name=name,
                                        last_modified=last_modified,
                                        id=id)
            abstraction.is_dirty = False
            res[('abstraction', id)] = abstraction

        return res

    def from_sql_fast(self, obj, all_objects):
        pass
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'entity_type', 'name', 'last_modified']
        table = 'abstraction'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        columnMap = {}
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            columnMap['id'] = \
                self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_entity_type') and obj.db_entity_type is not None:
            columnMap['entity_type'] = \
                self.convertToDB(obj.db_entity_type, 'str', 'char(16)')
        if hasattr(obj, 'db_name') and obj.db_name is not None:
            columnMap['name'] = \
                self.convertToDB(obj.db_name, 'str', 'varchar(255)')
        if hasattr(obj, 'db_last_modified') and obj.db_last_modified is not None:
            columnMap['last_modified'] = \
                self.convertToDB(obj.db_last_modified, 'datetime', 'datetime')
        columnMap.update(global_props)

        if obj.is_new or do_copy:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        if obj.db_id is None:
            obj.db_id = lastId
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_entity_type') and obj.db_entity_type is not None:
            global_props['entity_type'] = self.convertToDB(obj.db_entity_type, 'str', 'char(16)')
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            global_props['entity_id'] = self.convertToDB(obj.db_id, 'long', 'int')
        
    def to_sql_fast(self, obj, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        for child in obj.db_actions:
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        for child in obj.db_tags:
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        
    def delete_sql_column(self, db, obj, global_props):
        table = 'abstraction'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        dbCommand = self.createSQLDelete(table, whereMap)
        self.executeSQL(db, dbCommand, False)

class DBWorkflowSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props,lock=False):
        columns = ['id', 'entity_id', 'entity_type', 'name', 'version', 'last_modified', 'vistrail_id', 'parent_id', 'parent_type']
        table = 'workflow'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy, lock)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            if not global_props.has_key('entity_id'):
                global_props['entity_id'] = self.convertToDB(id, 'long', 'int')
            entity_id = self.convertFromDB(row[1], 'long', 'int')
            entity_type = self.convertFromDB(row[2], 'str', 'char(16)')
            if not global_props.has_key('entity_type'):
                global_props['entity_type'] = self.convertToDB(entity_type, 'str', 'char(16)')
            name = self.convertFromDB(row[3], 'str', 'varchar(255)')
            version = self.convertFromDB(row[4], 'str', 'char(16)')
            last_modified = self.convertFromDB(row[5], 'datetime', 'datetime')
            vistrail_id = self.convertFromDB(row[6], 'long', 'int')
            parent = self.convertFromDB(row[7], 'long', 'int')
            parentType = self.convertFromDB(row[8], 'str', 'char(16)')
            
            workflow = DBWorkflow(entity_type=entity_type,
                                  name=name,
                                  version=version,
                                  last_modified=last_modified,
                                  vistrail_id=vistrail_id,
                                  id=id)
            workflow.db_entity_id = entity_id
            workflow.db_parent = parent
            workflow.db_parentType = parentType
            workflow.is_dirty = False
            res[('workflow', id)] = workflow

        return res

    def from_sql_fast(self, obj, all_objects):
        p = all_objects[('group', obj.db_parent)]
        p.db_add_workflow(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'entity_id', 'entity_type', 'name', 'version', 'last_modified', 'vistrail_id', 'parent_id', 'parent_type']
        table = 'workflow'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        columnMap = {}
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            columnMap['id'] = \
                self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_entity_id') and obj.db_entity_id is not None:
            columnMap['entity_id'] = \
                self.convertToDB(obj.db_entity_id, 'long', 'int')
        if hasattr(obj, 'db_entity_type') and obj.db_entity_type is not None:
            columnMap['entity_type'] = \
                self.convertToDB(obj.db_entity_type, 'str', 'char(16)')
        if hasattr(obj, 'db_name') and obj.db_name is not None:
            columnMap['name'] = \
                self.convertToDB(obj.db_name, 'str', 'varchar(255)')
        if hasattr(obj, 'db_version') and obj.db_version is not None:
            columnMap['version'] = \
                self.convertToDB(obj.db_version, 'str', 'char(16)')
        if hasattr(obj, 'db_last_modified') and obj.db_last_modified is not None:
            columnMap['last_modified'] = \
                self.convertToDB(obj.db_last_modified, 'datetime', 'datetime')
        if hasattr(obj, 'db_vistrail_id') and obj.db_vistrail_id is not None:
            columnMap['vistrail_id'] = \
                self.convertToDB(obj.db_vistrail_id, 'long', 'int')
        if hasattr(obj, 'db_parent') and obj.db_parent is not None:
            columnMap['parent_id'] = \
                self.convertToDB(obj.db_parent, 'long', 'int')
        if hasattr(obj, 'db_parentType') and obj.db_parentType is not None:
            columnMap['parent_type'] = \
                self.convertToDB(obj.db_parentType, 'str', 'char(16)')
        columnMap.update(global_props)

        if obj.is_new or do_copy:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        if obj.db_id is None:
            obj.db_id = lastId
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_entity_type') and obj.db_entity_type is not None:
            global_props['entity_type'] = self.convertToDB(obj.db_entity_type, 'str', 'char(16)')
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            global_props['entity_id'] = self.convertToDB(obj.db_id, 'long', 'int')
        
    def to_sql_fast(self, obj, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        for child in obj.db_connections:
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        for child in obj.db_annotations:
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        for child in obj.db_plugin_datas:
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        for child in obj.db_others:
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        for child in obj.db_modules:
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        
    def delete_sql_column(self, db, obj, global_props):
        table = 'workflow'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        dbCommand = self.createSQLDelete(table, whereMap)
        self.executeSQL(db, dbCommand, False)

class DBAbstractionRefSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props,lock=False):
        columns = ['id', 'name', 'cache', 'abstraction_id', 'version', 'parent_type', 'entity_id', 'entity_type', 'parent_id']
        table = 'abstraction_ref'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy, lock)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            name = self.convertFromDB(row[1], 'str', 'varchar(1023)')
            cache = self.convertFromDB(row[2], 'int', 'int')
            abstraction_id = self.convertFromDB(row[3], 'long', 'int')
            version = self.convertFromDB(row[4], 'long', 'int')
            parentType = self.convertFromDB(row[5], 'str', 'char(16)')
            entity_id = self.convertFromDB(row[6], 'long', 'int')
            entity_type = self.convertFromDB(row[7], 'str', 'char(16)')
            parent = self.convertFromDB(row[8], 'long', 'long')
            
            abstractionRef = DBAbstractionRef(name=name,
                                              cache=cache,
                                              abstraction_id=abstraction_id,
                                              version=version,
                                              id=id)
            abstractionRef.db_parentType = parentType
            abstractionRef.db_entity_id = entity_id
            abstractionRef.db_entity_type = entity_type
            abstractionRef.db_parent = parent
            abstractionRef.is_dirty = False
            res[('abstractionRef', id)] = abstractionRef

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
        columns = ['id', 'name', 'cache', 'abstraction_id', 'version', 'parent_type', 'entity_id', 'entity_type', 'parent_id']
        table = 'abstraction_ref'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        columnMap = {}
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            columnMap['id'] = \
                self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_name') and obj.db_name is not None:
            columnMap['name'] = \
                self.convertToDB(obj.db_name, 'str', 'varchar(1023)')
        if hasattr(obj, 'db_cache') and obj.db_cache is not None:
            columnMap['cache'] = \
                self.convertToDB(obj.db_cache, 'int', 'int')
        if hasattr(obj, 'db_abstraction_id') and obj.db_abstraction_id is not None:
            columnMap['abstraction_id'] = \
                self.convertToDB(obj.db_abstraction_id, 'long', 'int')
        if hasattr(obj, 'db_version') and obj.db_version is not None:
            columnMap['version'] = \
                self.convertToDB(obj.db_version, 'long', 'int')
        if hasattr(obj, 'db_parentType') and obj.db_parentType is not None:
            columnMap['parent_type'] = \
                self.convertToDB(obj.db_parentType, 'str', 'char(16)')
        if hasattr(obj, 'db_entity_id') and obj.db_entity_id is not None:
            columnMap['entity_id'] = \
                self.convertToDB(obj.db_entity_id, 'long', 'int')
        if hasattr(obj, 'db_entity_type') and obj.db_entity_type is not None:
            columnMap['entity_type'] = \
                self.convertToDB(obj.db_entity_type, 'str', 'char(16)')
        if hasattr(obj, 'db_parent') and obj.db_parent is not None:
            columnMap['parent_id'] = \
                self.convertToDB(obj.db_parent, 'long', 'long')
        columnMap.update(global_props)

        if obj.is_new or do_copy:
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
        
    def delete_sql_column(self, db, obj, global_props):
        table = 'abstraction_ref'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        dbCommand = self.createSQLDelete(table, whereMap)
        self.executeSQL(db, dbCommand, False)

class DBAnnotationSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props,lock=False):
        columns = ['id', 'akey', 'value', 'parent_type', 'entity_id', 'entity_type', 'parent_id']
        table = 'annotation'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy, lock)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            key = self.convertFromDB(row[1], 'str', 'varchar(255)')
            value = self.convertFromDB(row[2], 'str', 'varchar(8191)')
            parentType = self.convertFromDB(row[3], 'str', 'char(16)')
            entity_id = self.convertFromDB(row[4], 'long', 'int')
            entity_type = self.convertFromDB(row[5], 'str', 'char(16)')
            parent = self.convertFromDB(row[6], 'long', 'long')
            
            annotation = DBAnnotation(key=key,
                                      value=value,
                                      id=id)
            annotation.db_parentType = parentType
            annotation.db_entity_id = entity_id
            annotation.db_entity_type = entity_type
            annotation.db_parent = parent
            annotation.is_dirty = False
            res[('annotation', id)] = annotation

        return res

    def from_sql_fast(self, obj, all_objects):
        if obj.db_parentType == 'vistrail':
            p = all_objects[('vistrail', obj.db_parent)]
            p.db_add_annotation(obj)
        elif obj.db_parentType == 'workflow':
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
        elif obj.db_parentType == 'abstractionRef':
            p = all_objects[('abstractionRef', obj.db_parent)]
            p.db_add_annotation(obj)
        elif obj.db_parentType == 'group':
            p = all_objects[('group', obj.db_parent)]
            p.db_add_annotation(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'akey', 'value', 'parent_type', 'entity_id', 'entity_type', 'parent_id']
        table = 'annotation'
        whereMap = {}
        whereMap.update(global_props)
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
        if hasattr(obj, 'db_entity_id') and obj.db_entity_id is not None:
            columnMap['entity_id'] = \
                self.convertToDB(obj.db_entity_id, 'long', 'int')
        if hasattr(obj, 'db_entity_type') and obj.db_entity_type is not None:
            columnMap['entity_type'] = \
                self.convertToDB(obj.db_entity_type, 'str', 'char(16)')
        if hasattr(obj, 'db_parent') and obj.db_parent is not None:
            columnMap['parent_id'] = \
                self.convertToDB(obj.db_parent, 'long', 'long')
        columnMap.update(global_props)

        if obj.is_new or do_copy:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        pass
        
    def delete_sql_column(self, db, obj, global_props):
        table = 'annotation'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        dbCommand = self.createSQLDelete(table, whereMap)
        self.executeSQL(db, dbCommand, False)

class DBChangeSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props,lock=False):
        columns = ['id', 'what', 'old_obj_id', 'new_obj_id', 'par_obj_id', 'par_obj_type', 'action_id', 'entity_id', 'entity_type']
        table = 'change_tbl'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy, lock)
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
            entity_id = self.convertFromDB(row[7], 'long', 'int')
            entity_type = self.convertFromDB(row[8], 'str', 'char(16)')
            
            change = DBChange(what=what,
                              oldObjId=oldObjId,
                              newObjId=newObjId,
                              parentObjId=parentObjId,
                              parentObjType=parentObjType,
                              id=id)
            change.db_action = action
            change.db_entity_id = entity_id
            change.db_entity_type = entity_type
            change.is_dirty = False
            res[('change', id)] = change

        return res

    def from_sql_fast(self, obj, all_objects):
        p = all_objects[('action', obj.db_action)]
        p.db_add_operation(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'what', 'old_obj_id', 'new_obj_id', 'par_obj_id', 'par_obj_type', 'action_id', 'entity_id', 'entity_type']
        table = 'change_tbl'
        whereMap = {}
        whereMap.update(global_props)
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
        if hasattr(obj, 'db_entity_id') and obj.db_entity_id is not None:
            columnMap['entity_id'] = \
                self.convertToDB(obj.db_entity_id, 'long', 'int')
        if hasattr(obj, 'db_entity_type') and obj.db_entity_type is not None:
            columnMap['entity_type'] = \
                self.convertToDB(obj.db_entity_type, 'str', 'char(16)')
        columnMap.update(global_props)

        if obj.is_new or do_copy:
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
        
    def delete_sql_column(self, db, obj, global_props):
        table = 'change_tbl'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        dbCommand = self.createSQLDelete(table, whereMap)
        self.executeSQL(db, dbCommand, False)

class DBWorkflowExecSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props,lock=False):
        columns = ['id', 'user', 'ip', 'session', 'vt_version', 'ts_start', 'ts_end', 'parent_id', 'parent_type', 'parent_version', 'completed', 'name', 'log_id', 'entity_id', 'entity_type']
        table = 'workflow_exec'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy, lock)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            user = self.convertFromDB(row[1], 'str', 'varchar(255)')
            ip = self.convertFromDB(row[2], 'str', 'varchar(255)')
            session = self.convertFromDB(row[3], 'long', 'int')
            vt_version = self.convertFromDB(row[4], 'str', 'varchar(255)')
            ts_start = self.convertFromDB(row[5], 'datetime', 'datetime')
            ts_end = self.convertFromDB(row[6], 'datetime', 'datetime')
            parent_id = self.convertFromDB(row[7], 'long', 'int')
            parent_type = self.convertFromDB(row[8], 'str', 'varchar(255)')
            parent_version = self.convertFromDB(row[9], 'long', 'int')
            completed = self.convertFromDB(row[10], 'int', 'int')
            name = self.convertFromDB(row[11], 'str', 'varchar(255)')
            log = self.convertFromDB(row[12], 'long', 'int')
            entity_id = self.convertFromDB(row[13], 'long', 'int')
            entity_type = self.convertFromDB(row[14], 'str', 'char(16)')
            
            workflow_exec = DBWorkflowExec(user=user,
                                           ip=ip,
                                           session=session,
                                           vt_version=vt_version,
                                           ts_start=ts_start,
                                           ts_end=ts_end,
                                           parent_id=parent_id,
                                           parent_type=parent_type,
                                           parent_version=parent_version,
                                           completed=completed,
                                           name=name,
                                           id=id)
            workflow_exec.db_log = log
            workflow_exec.db_entity_id = entity_id
            workflow_exec.db_entity_type = entity_type
            workflow_exec.is_dirty = False
            res[('workflow_exec', id)] = workflow_exec

        return res

    def from_sql_fast(self, obj, all_objects):
        p = all_objects[('log', obj.db_log)]
        p.db_add_workflow_exec(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'user', 'ip', 'session', 'vt_version', 'ts_start', 'ts_end', 'parent_id', 'parent_type', 'parent_version', 'completed', 'name', 'log_id', 'entity_id', 'entity_type']
        table = 'workflow_exec'
        whereMap = {}
        whereMap.update(global_props)
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
        if hasattr(obj, 'db_session') and obj.db_session is not None:
            columnMap['session'] = \
                self.convertToDB(obj.db_session, 'long', 'int')
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
        if hasattr(obj, 'db_completed') and obj.db_completed is not None:
            columnMap['completed'] = \
                self.convertToDB(obj.db_completed, 'int', 'int')
        if hasattr(obj, 'db_name') and obj.db_name is not None:
            columnMap['name'] = \
                self.convertToDB(obj.db_name, 'str', 'varchar(255)')
        if hasattr(obj, 'db_log') and obj.db_log is not None:
            columnMap['log_id'] = \
                self.convertToDB(obj.db_log, 'long', 'int')
        if hasattr(obj, 'db_entity_id') and obj.db_entity_id is not None:
            columnMap['entity_id'] = \
                self.convertToDB(obj.db_entity_id, 'long', 'int')
        if hasattr(obj, 'db_entity_type') and obj.db_entity_type is not None:
            columnMap['entity_type'] = \
                self.convertToDB(obj.db_entity_type, 'str', 'char(16)')
        columnMap.update(global_props)

        if obj.is_new or do_copy:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        for child in obj.db_module_execs:
            child.db_workflow_exec = obj.db_id
        
    def delete_sql_column(self, db, obj, global_props):
        table = 'workflow_exec'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        dbCommand = self.createSQLDelete(table, whereMap)
        self.executeSQL(db, dbCommand, False)

class DBConnectionSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props,lock=False):
        columns = ['id', 'parent_type', 'entity_id', 'entity_type', 'parent_id']
        table = 'connection_tbl'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy, lock)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            parentType = self.convertFromDB(row[1], 'str', 'char(16)')
            entity_id = self.convertFromDB(row[2], 'long', 'int')
            entity_type = self.convertFromDB(row[3], 'str', 'char(16)')
            parent = self.convertFromDB(row[4], 'long', 'long')
            
            connection = DBConnection(id=id)
            connection.db_parentType = parentType
            connection.db_entity_id = entity_id
            connection.db_entity_type = entity_type
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
        columns = ['id', 'parent_type', 'entity_id', 'entity_type', 'parent_id']
        table = 'connection_tbl'
        whereMap = {}
        whereMap.update(global_props)
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
        if hasattr(obj, 'db_entity_id') and obj.db_entity_id is not None:
            columnMap['entity_id'] = \
                self.convertToDB(obj.db_entity_id, 'long', 'int')
        if hasattr(obj, 'db_entity_type') and obj.db_entity_type is not None:
            columnMap['entity_type'] = \
                self.convertToDB(obj.db_entity_type, 'str', 'char(16)')
        if hasattr(obj, 'db_parent') and obj.db_parent is not None:
            columnMap['parent_id'] = \
                self.convertToDB(obj.db_parent, 'long', 'long')
        columnMap.update(global_props)

        if obj.is_new or do_copy:
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
        
    def delete_sql_column(self, db, obj, global_props):
        table = 'connection_tbl'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        dbCommand = self.createSQLDelete(table, whereMap)
        self.executeSQL(db, dbCommand, False)

class DBActionSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props,lock=False):
        columns = ['id', 'prev_id', 'date', 'session', 'user', 'prune', 'parent_type', 'entity_id', 'entity_type', 'parent_id']
        table = 'action'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy, lock)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            prevId = self.convertFromDB(row[1], 'long', 'int')
            date = self.convertFromDB(row[2], 'datetime', 'datetime')
            session = self.convertFromDB(row[3], 'long', 'int')
            user = self.convertFromDB(row[4], 'str', 'varchar(255)')
            prune = self.convertFromDB(row[5], 'int', 'int')
            parentType = self.convertFromDB(row[6], 'str', 'char(16)')
            entity_id = self.convertFromDB(row[7], 'long', 'int')
            entity_type = self.convertFromDB(row[8], 'str', 'char(16)')
            parent = self.convertFromDB(row[9], 'long', 'long')
            
            action = DBAction(prevId=prevId,
                              date=date,
                              session=session,
                              user=user,
                              prune=prune,
                              id=id)
            action.db_parentType = parentType
            action.db_entity_id = entity_id
            action.db_entity_type = entity_type
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
        columns = ['id', 'prev_id', 'date', 'session', 'user', 'prune', 'parent_type', 'entity_id', 'entity_type', 'parent_id']
        table = 'action'
        whereMap = {}
        whereMap.update(global_props)
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
                self.convertToDB(obj.db_session, 'long', 'int')
        if hasattr(obj, 'db_user') and obj.db_user is not None:
            columnMap['user'] = \
                self.convertToDB(obj.db_user, 'str', 'varchar(255)')
        if hasattr(obj, 'db_prune') and obj.db_prune is not None:
            columnMap['prune'] = \
                self.convertToDB(obj.db_prune, 'int', 'int')
        if hasattr(obj, 'db_parentType') and obj.db_parentType is not None:
            columnMap['parent_type'] = \
                self.convertToDB(obj.db_parentType, 'str', 'char(16)')
        if hasattr(obj, 'db_entity_id') and obj.db_entity_id is not None:
            columnMap['entity_id'] = \
                self.convertToDB(obj.db_entity_id, 'long', 'int')
        if hasattr(obj, 'db_entity_type') and obj.db_entity_type is not None:
            columnMap['entity_type'] = \
                self.convertToDB(obj.db_entity_type, 'str', 'char(16)')
        if hasattr(obj, 'db_parent') and obj.db_parent is not None:
            columnMap['parent_id'] = \
                self.convertToDB(obj.db_parent, 'long', 'long')
        columnMap.update(global_props)

        if obj.is_new or do_copy:
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
        
    def delete_sql_column(self, db, obj, global_props):
        table = 'action'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        dbCommand = self.createSQLDelete(table, whereMap)
        self.executeSQL(db, dbCommand, False)

class DBDeleteSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props,lock=False):
        columns = ['id', 'what', 'object_id', 'par_obj_id', 'par_obj_type', 'action_id', 'entity_id', 'entity_type']
        table = 'delete_tbl'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy, lock)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            what = self.convertFromDB(row[1], 'str', 'varchar(255)')
            objectId = self.convertFromDB(row[2], 'long', 'int')
            parentObjId = self.convertFromDB(row[3], 'long', 'int')
            parentObjType = self.convertFromDB(row[4], 'str', 'char(16)')
            action = self.convertFromDB(row[5], 'long', 'int')
            entity_id = self.convertFromDB(row[6], 'long', 'int')
            entity_type = self.convertFromDB(row[7], 'str', 'char(16)')
            
            delete = DBDelete(what=what,
                              objectId=objectId,
                              parentObjId=parentObjId,
                              parentObjType=parentObjType,
                              id=id)
            delete.db_action = action
            delete.db_entity_id = entity_id
            delete.db_entity_type = entity_type
            delete.is_dirty = False
            res[('delete', id)] = delete

        return res

    def from_sql_fast(self, obj, all_objects):
        p = all_objects[('action', obj.db_action)]
        p.db_add_operation(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'what', 'object_id', 'par_obj_id', 'par_obj_type', 'action_id', 'entity_id', 'entity_type']
        table = 'delete_tbl'
        whereMap = {}
        whereMap.update(global_props)
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
        if hasattr(obj, 'db_entity_id') and obj.db_entity_id is not None:
            columnMap['entity_id'] = \
                self.convertToDB(obj.db_entity_id, 'long', 'int')
        if hasattr(obj, 'db_entity_type') and obj.db_entity_type is not None:
            columnMap['entity_type'] = \
                self.convertToDB(obj.db_entity_type, 'str', 'char(16)')
        columnMap.update(global_props)

        if obj.is_new or do_copy:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        
    def to_sql_fast(self, obj, do_copy=True):
        pass
        
    def delete_sql_column(self, db, obj, global_props):
        table = 'delete_tbl'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        dbCommand = self.createSQLDelete(table, whereMap)
        self.executeSQL(db, dbCommand, False)

class DBVistrailSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props,lock=False):
        columns = ['id', 'entity_type', 'version', 'name', 'last_modified']
        table = 'vistrail'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy, lock)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            if not global_props.has_key('entity_id'):
                global_props['entity_id'] = self.convertToDB(id, 'long', 'int')
            entity_type = self.convertFromDB(row[1], 'str', 'char(16)')
            if not global_props.has_key('entity_type'):
                global_props['entity_type'] = self.convertToDB(entity_type, 'str', 'char(16)')
            version = self.convertFromDB(row[2], 'str', 'char(16)')
            name = self.convertFromDB(row[3], 'str', 'varchar(255)')
            last_modified = self.convertFromDB(row[4], 'datetime', 'datetime')
            
            vistrail = DBVistrail(entity_type=entity_type,
                                  version=version,
                                  name=name,
                                  last_modified=last_modified,
                                  id=id)
            vistrail.is_dirty = False
            res[('vistrail', id)] = vistrail

        return res

    def from_sql_fast(self, obj, all_objects):
        pass
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'entity_type', 'version', 'name', 'last_modified']
        table = 'vistrail'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        columnMap = {}
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            columnMap['id'] = \
                self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_entity_type') and obj.db_entity_type is not None:
            columnMap['entity_type'] = \
                self.convertToDB(obj.db_entity_type, 'str', 'char(16)')
        if hasattr(obj, 'db_version') and obj.db_version is not None:
            columnMap['version'] = \
                self.convertToDB(obj.db_version, 'str', 'char(16)')
        if hasattr(obj, 'db_name') and obj.db_name is not None:
            columnMap['name'] = \
                self.convertToDB(obj.db_name, 'str', 'varchar(255)')
        if hasattr(obj, 'db_last_modified') and obj.db_last_modified is not None:
            columnMap['last_modified'] = \
                self.convertToDB(obj.db_last_modified, 'datetime', 'datetime')
        columnMap.update(global_props)

        if obj.is_new or do_copy:
            dbCommand = self.createSQLInsert(table, columnMap)
        else:
            dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
        lastId = self.executeSQL(db, dbCommand, False)
        if obj.db_id is None:
            obj.db_id = lastId
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if hasattr(obj, 'db_entity_type') and obj.db_entity_type is not None:
            global_props['entity_type'] = self.convertToDB(obj.db_entity_type, 'str', 'char(16)')
        if hasattr(obj, 'db_id') and obj.db_id is not None:
            global_props['entity_id'] = self.convertToDB(obj.db_id, 'long', 'int')
        
    def to_sql_fast(self, obj, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        for child in obj.db_actions:
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        for child in obj.db_tags:
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        for child in obj.db_annotations:
            child.db_parentType = obj.vtType
            child.db_parent = obj.db_id
        
    def delete_sql_column(self, db, obj, global_props):
        table = 'vistrail'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        dbCommand = self.createSQLDelete(table, whereMap)
        self.executeSQL(db, dbCommand, False)

class DBModuleExecSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def get_sql_columns(self, db, global_props,lock=False):
        columns = ['id', 'ts_start', 'ts_end', 'cached', 'module_id', 'module_name', 'completed', 'error', 'abstraction_id', 'abstraction_version', 'machine_id', 'wf_exec_id', 'entity_id', 'entity_type']
        table = 'module_exec'
        whereMap = global_props
        orderBy = 'id'

        dbCommand = self.createSQLSelect(table, columns, whereMap, orderBy, lock)
        data = self.executeSQL(db, dbCommand, True)
        res = {}
        for row in data:
            id = self.convertFromDB(row[0], 'long', 'int')
            ts_start = self.convertFromDB(row[1], 'datetime', 'datetime')
            ts_end = self.convertFromDB(row[2], 'datetime', 'datetime')
            cached = self.convertFromDB(row[3], 'int', 'int')
            module_id = self.convertFromDB(row[4], 'long', 'int')
            module_name = self.convertFromDB(row[5], 'str', 'varchar(255)')
            completed = self.convertFromDB(row[6], 'int', 'int')
            error = self.convertFromDB(row[7], 'str', 'varchar(1023)')
            abstraction_id = self.convertFromDB(row[8], 'long', 'int')
            abstraction_version = self.convertFromDB(row[9], 'long', 'int')
            machine_id = self.convertFromDB(row[10], 'long', 'int')
            workflow_exec = self.convertFromDB(row[11], 'long', 'int')
            entity_id = self.convertFromDB(row[12], 'long', 'int')
            entity_type = self.convertFromDB(row[13], 'str', 'char(16)')
            
            module_exec = DBModuleExec(ts_start=ts_start,
                                       ts_end=ts_end,
                                       cached=cached,
                                       module_id=module_id,
                                       module_name=module_name,
                                       completed=completed,
                                       error=error,
                                       abstraction_id=abstraction_id,
                                       abstraction_version=abstraction_version,
                                       machine_id=machine_id,
                                       id=id)
            module_exec.db_workflow_exec = workflow_exec
            module_exec.db_entity_id = entity_id
            module_exec.db_entity_type = entity_type
            module_exec.is_dirty = False
            res[('module_exec', id)] = module_exec

        return res

    def from_sql_fast(self, obj, all_objects):
        p = all_objects[('workflow_exec', obj.db_workflow_exec)]
        p.db_add_module_exec(obj)
        
    def set_sql_columns(self, db, obj, global_props, do_copy=True):
        if not do_copy and not obj.is_dirty:
            return
        columns = ['id', 'ts_start', 'ts_end', 'cached', 'module_id', 'module_name', 'completed', 'error', 'abstraction_id', 'abstraction_version', 'machine_id', 'wf_exec_id', 'entity_id', 'entity_type']
        table = 'module_exec'
        whereMap = {}
        whereMap.update(global_props)
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
        if hasattr(obj, 'db_cached') and obj.db_cached is not None:
            columnMap['cached'] = \
                self.convertToDB(obj.db_cached, 'int', 'int')
        if hasattr(obj, 'db_module_id') and obj.db_module_id is not None:
            columnMap['module_id'] = \
                self.convertToDB(obj.db_module_id, 'long', 'int')
        if hasattr(obj, 'db_module_name') and obj.db_module_name is not None:
            columnMap['module_name'] = \
                self.convertToDB(obj.db_module_name, 'str', 'varchar(255)')
        if hasattr(obj, 'db_completed') and obj.db_completed is not None:
            columnMap['completed'] = \
                self.convertToDB(obj.db_completed, 'int', 'int')
        if hasattr(obj, 'db_error') and obj.db_error is not None:
            columnMap['error'] = \
                self.convertToDB(obj.db_error, 'str', 'varchar(1023)')
        if hasattr(obj, 'db_abstraction_id') and obj.db_abstraction_id is not None:
            columnMap['abstraction_id'] = \
                self.convertToDB(obj.db_abstraction_id, 'long', 'int')
        if hasattr(obj, 'db_abstraction_version') and obj.db_abstraction_version is not None:
            columnMap['abstraction_version'] = \
                self.convertToDB(obj.db_abstraction_version, 'long', 'int')
        if hasattr(obj, 'db_machine_id') and obj.db_machine_id is not None:
            columnMap['machine_id'] = \
                self.convertToDB(obj.db_machine_id, 'long', 'int')
        if hasattr(obj, 'db_workflow_exec') and obj.db_workflow_exec is not None:
            columnMap['wf_exec_id'] = \
                self.convertToDB(obj.db_workflow_exec, 'long', 'int')
        if hasattr(obj, 'db_entity_id') and obj.db_entity_id is not None:
            columnMap['entity_id'] = \
                self.convertToDB(obj.db_entity_id, 'long', 'int')
        if hasattr(obj, 'db_entity_type') and obj.db_entity_type is not None:
            columnMap['entity_type'] = \
                self.convertToDB(obj.db_entity_type, 'str', 'char(16)')
        columnMap.update(global_props)

        if obj.is_new or do_copy:
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
        
    def delete_sql_column(self, db, obj, global_props):
        table = 'module_exec'
        whereMap = {}
        whereMap.update(global_props)
        if obj.db_id is not None:
            keyStr = self.convertToDB(obj.db_id, 'long', 'int')
            whereMap['id'] = keyStr
        dbCommand = self.createSQLDelete(table, whereMap)
        self.executeSQL(db, dbCommand, False)

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
        if 'group' not in self:
            self['group'] = DBGroupSQLDAOBase(self)
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
        if 'parameter' not in self:
            self['parameter'] = DBParameterSQLDAOBase(self)
        if 'plugin_data' not in self:
            self['plugin_data'] = DBPluginDataSQLDAOBase(self)
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
        if 'workflow_exec' not in self:
            self['workflow_exec'] = DBWorkflowExecSQLDAOBase(self)
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
