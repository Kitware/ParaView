
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
from db.versions.v0_5_0.domain import *

class DBPortSpecSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

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

    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['cache', 'name', 'id']
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
            name = self.convertFromDB(row[1], 'str', 'varchar(255)')
            id = self.convertFromDB(row[2], 'long', 'int')
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
            annotations = {}
            for obj in res:
                annotations[obj.db_id] = obj
            
            discStr = self.convertToDB('module','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            res = self.getDao('portSpec').fromSQL(db, None, foreignKey, globalProps)
            portSpecs = res
            
            module = DBModule(id=id,
                              cache=cache,
                              name=name,
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
        for child in obj.db_annotations.itervalues():
            self.getDao('annotation').toSQL(db, child, foreignKey, globalProps)
        
        discStr = self.convertToDB('module','str','char(16)')
        foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
        for child in obj.db_portSpecs:
            self.getDao('portSpec').toSQL(db, child, foreignKey, globalProps)
        

class DBSessionSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['user', 'ip', 'vis_ver', 'ts_start', 'tsEnd', 'machine_id', 'id']
        table = 'session'
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
            visVersion = self.convertFromDB(row[2], 'str', 'varchar(255)')
            tsStart = self.convertFromDB(row[3], 'datetime', 'datetime')
            tsEnd = self.convertFromDB(row[4], 'datetime', 'datetime')
            machineId = self.convertFromDB(row[5], 'long', 'int')
            id = self.convertFromDB(row[6], 'long', 'int')
            keyStr = self.convertToDB(id,'long','int')

            foreignKey = {'session_id': keyStr}
            res = self.getDao('wfExec').fromSQL(db, None, foreignKey, globalProps)
            wfExecs = {}
            for obj in res:
                wfExecs[obj.db_id] = obj
            
            session = DBSession(id=id,
                                user=user,
                                ip=ip,
                                visVersion=visVersion,
                                tsStart=tsStart,
                                tsEnd=tsEnd,
                                machineId=machineId,
                                wfExecs=wfExecs)
            session.is_dirty = False
            list.append(session)

        return list

    def toSQL(self, db, obj, foreignKey=None, globalProps=None):
        keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if obj.is_dirty:
            columns = ['id']
            table = 'session'
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
            if obj.db_visVersion is not None:
                columnMap['vis_ver'] = \
                    self.convertToDB(obj.db_visVersion, 'str', 'varchar(255)')
            if obj.db_tsStart is not None:
                columnMap['ts_start'] = \
                    self.convertToDB(obj.db_tsStart, 'datetime', 'datetime')
            if obj.db_tsEnd is not None:
                columnMap['tsEnd'] = \
                    self.convertToDB(obj.db_tsEnd, 'datetime', 'datetime')
            if obj.db_machineId is not None:
                columnMap['machine_id'] = \
                    self.convertToDB(obj.db_machineId, 'long', 'int')
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
        

        foreignKey = {'session_id': keyStr}
        for child in obj.db_wfExecs.itervalues():
            self.getDao('wfExec').toSQL(db, child, foreignKey, globalProps)
        

class DBPortSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['type', 'moduleId', 'moduleName', 'sig', 'id']
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
            sig = self.convertFromDB(row[3], 'str', 'varchar(255)')
            id = self.convertFromDB(row[4], 'long', 'int')
            keyStr = self.convertToDB(id,'long','int')

            port = DBPort(id=id,
                          type=type,
                          moduleId=moduleId,
                          moduleName=moduleName,
                          sig=sig)
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
            if obj.db_sig is not None:
                columnMap['sig'] = \
                    self.convertToDB(obj.db_sig, 'str', 'varchar(255)')
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
            res = self.getDao('session').fromSQL(db, None, foreignKey, globalProps)
            sessions = {}
            for obj in res:
                sessions[obj.db_id] = obj
            
            foreignKey = {'log_id': keyStr}
            res = self.getDao('machine').fromSQL(db, None, foreignKey, globalProps)
            machines = {}
            for obj in res:
                machines[obj.db_id] = obj
            
            log = DBLog(id=id,
                        sessions=sessions,
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
        for child in obj.db_sessions.itervalues():
            self.getDao('session').toSQL(db, child, foreignKey, globalProps)
        
        foreignKey = {'log_id': keyStr}
        for child in obj.db_machines.itervalues():
            self.getDao('machine').toSQL(db, child, foreignKey, globalProps)
        

class DBMachineSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

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

            machine = DBMachine(id=id,
                                name=name,
                                os=os,
                                architecture=architecture,
                                processor=processor,
                                ram=ram)
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
        


class DBAddSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

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
        elif child.vtType == 'other':
            discStr = self.convertToDB('add','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            self.getDao('other').toSQL(db, child, foreignKey, globalProps)
        

class DBOtherSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

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
        


class DBWfExecSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['ts_start', 'ts_end', 'wfVersion', 'vistrail_id', 'vistrail_name', 'id']
        table = 'wf_exec'
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
            tsStart = self.convertFromDB(row[0], 'datetime', 'datetime')
            tsEnd = self.convertFromDB(row[1], 'datetime', 'datetime')
            wfVersion = self.convertFromDB(row[2], 'int', 'int')
            vistrailId = self.convertFromDB(row[3], 'long', 'int')
            vistrailName = self.convertFromDB(row[4], 'str', 'varchar(255)')
            id = self.convertFromDB(row[5], 'long', 'int')
            keyStr = self.convertToDB(id,'long','int')

            foreignKey = {'wf_exec_id': keyStr}
            res = self.getDao('execRec').fromSQL(db, None, foreignKey, globalProps)
            execRecs = {}
            for obj in res:
                execRecs[obj.db_id] = obj
            
            wfExec = DBWfExec(id=id,
                              tsStart=tsStart,
                              tsEnd=tsEnd,
                              wfVersion=wfVersion,
                              vistrailId=vistrailId,
                              vistrailName=vistrailName,
                              execRecs=execRecs)
            wfExec.is_dirty = False
            list.append(wfExec)

        return list

    def toSQL(self, db, obj, foreignKey=None, globalProps=None):
        keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if obj.is_dirty:
            columns = ['id']
            table = 'wf_exec'
            whereMap = {}
            columnMap = {}

            whereMap['id'] = keyStr
            if globalProps is not None:
                whereMap.update(globalProps)
            if obj.db_tsStart is not None:
                columnMap['ts_start'] = \
                    self.convertToDB(obj.db_tsStart, 'datetime', 'datetime')
            if obj.db_tsEnd is not None:
                columnMap['ts_end'] = \
                    self.convertToDB(obj.db_tsEnd, 'datetime', 'datetime')
            if obj.db_wfVersion is not None:
                columnMap['wfVersion'] = \
                    self.convertToDB(obj.db_wfVersion, 'int', 'int')
            if obj.db_vistrailId is not None:
                columnMap['vistrail_id'] = \
                    self.convertToDB(obj.db_vistrailId, 'long', 'int')
            if obj.db_vistrailName is not None:
                columnMap['vistrail_name'] = \
                    self.convertToDB(obj.db_vistrailName, 'str', 'varchar(255)')
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
        for child in obj.db_execRecs.itervalues():
            self.getDao('execRec').toSQL(db, child, foreignKey, globalProps)
        

class DBParameterSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

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
            val = self.convertFromDB(row[3], 'str', 'varchar(8192)')
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
                    self.convertToDB(obj.db_val, 'str', 'varchar(8192)')
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
        


class DBFunctionSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

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
        

class DBWorkflowSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

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
            modules = {}
            for obj in res:
                modules[obj.db_id] = obj
            
            discStr = self.convertToDB('workflow','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            res = self.getDao('connection').fromSQL(db, None, foreignKey, globalProps)
            connections = {}
            for obj in res:
                connections[obj.db_id] = obj
            
            discStr = self.convertToDB('workflow','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            res = self.getDao('annotation').fromSQL(db, None, foreignKey, globalProps)
            annotations = res
            
            discStr = self.convertToDB('workflow','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            res = self.getDao('other').fromSQL(db, None, foreignKey, globalProps)
            others = res
            
            workflow = DBWorkflow(id=id,
                                  name=name,
                                  modules=modules,
                                  connections=connections,
                                  annotations=annotations,
                                  others=others)
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
        for child in obj.db_modules.itervalues():
            self.getDao('module').toSQL(db, child, foreignKey, globalProps)
        
        discStr = self.convertToDB('workflow','str','char(16)')
        foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
        for child in obj.db_connections.itervalues():
            self.getDao('connection').toSQL(db, child, foreignKey, globalProps)
        
        discStr = self.convertToDB('workflow','str','char(16)')
        foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
        for child in obj.db_annotations:
            self.getDao('annotation').toSQL(db, child, foreignKey, globalProps)
        
        discStr = self.convertToDB('workflow','str','char(16)')
        foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
        for child in obj.db_others:
            self.getDao('other').toSQL(db, child, foreignKey, globalProps)
        

class DBActionSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['prev_id', 'date', 'user', 'id']
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
            user = self.convertFromDB(row[2], 'str', 'varchar(255)')
            id = self.convertFromDB(row[3], 'long', 'int')
            keyStr = self.convertToDB(id,'long','int')

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
                              user=user,
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
            if obj.db_user is not None:
                columnMap['user'] = \
                    self.convertToDB(obj.db_user, 'str', 'varchar(255)')
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
        

class DBAnnotationSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

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
            value = self.convertFromDB(row[1], 'str', 'varchar(255)')
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
        


class DBChangeSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

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
        elif child.vtType == 'other':
            discStr = self.convertToDB('change','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            self.getDao('other').toSQL(db, child, foreignKey, globalProps)
        

class DBMacroSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['name', 'descrptn', 'id']
        table = 'macro'
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
            descrptn = self.convertFromDB(row[1], 'str', 'varchar(255)')
            id = self.convertFromDB(row[2], 'long', 'int')
            keyStr = self.convertToDB(id,'long','int')

            discStr = self.convertToDB('macro','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            res = self.getDao('action').fromSQL(db, None, foreignKey, globalProps)
            actions = {}
            for obj in res:
                actions[obj.db_id] = obj
            
            macro = DBMacro(id=id,
                            name=name,
                            descrptn=descrptn,
                            actions=actions)
            macro.is_dirty = False
            list.append(macro)

        return list

    def toSQL(self, db, obj, foreignKey=None, globalProps=None):
        keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if obj.is_dirty:
            columns = ['id']
            table = 'macro'
            whereMap = {}
            columnMap = {}

            whereMap['id'] = keyStr
            if globalProps is not None:
                whereMap.update(globalProps)
            if obj.db_name is not None:
                columnMap['name'] = \
                    self.convertToDB(obj.db_name, 'str', 'varchar(255)')
            if obj.db_descrptn is not None:
                columnMap['descrptn'] = \
                    self.convertToDB(obj.db_descrptn, 'str', 'varchar(255)')
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
        

        discStr = self.convertToDB('macro','str','char(16)')
        foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
        for child in obj.db_actions.itervalues():
            self.getDao('action').toSQL(db, child, foreignKey, globalProps)
        

class DBConnectionSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

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
        

class DBTagSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['time', 'name']
        table = 'tag'
        whereMap = {}
        orderBy = 'name'

        if id is not None:
            keyStr = self.convertToDB(id, 'str', 'varchar(255)')
            whereMap['name'] = keyStr
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
            time = self.convertFromDB(row[0], 'long', 'int')
            name = self.convertFromDB(row[1], 'str', 'varchar(255)')
            keyStr = self.convertToDB(name,'str','varchar(255)')

            tag = DBTag(name=name,
                        time=time)
            tag.is_dirty = False
            list.append(tag)

        return list

    def toSQL(self, db, obj, foreignKey=None, globalProps=None):
        keyStr = self.convertToDB(obj.db_name, 'str', 'varchar(255)')
        if obj.is_dirty:
            columns = ['name']
            table = 'tag'
            whereMap = {}
            columnMap = {}

            whereMap['name'] = keyStr
            if globalProps is not None:
                whereMap.update(globalProps)
            if obj.db_time is not None:
                columnMap['time'] = \
                    self.convertToDB(obj.db_time, 'long', 'int')
            if foreignKey is not None:
                columnMap.update(foreignKey)

            dbCommand = self.createSQLSelect(table, columns, whereMap)
            data = self.executeSQL(db, dbCommand, True)
            if len(data) <= 0:
                columnMap['name'] = keyStr
                if globalProps is not None:
                    columnMap.update(globalProps)
                dbCommand = self.createSQLInsert(table, columnMap)
            else:
                dbCommand = self.createSQLUpdate(table, columnMap, whereMap)
            lastId = self.executeSQL(db, dbCommand, False)
        


class DBExecRecSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

    def fromSQL(self, db, id=None, foreignKey=None, globalProps=None):
        columns = ['ts_start', 'ts_end', 'module_id', 'module_name', 'id']
        table = 'exec'
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
            tsStart = self.convertFromDB(row[0], 'datetime', 'datetime')
            tsEnd = self.convertFromDB(row[1], 'datetime', 'datetime')
            moduleId = self.convertFromDB(row[2], 'long', 'int')
            moduleName = self.convertFromDB(row[3], 'str', 'varchar(255)')
            id = self.convertFromDB(row[4], 'long', 'int')
            keyStr = self.convertToDB(id,'long','int')

            discStr = self.convertToDB('execRec','str','char(16)')
            foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
            res = self.getDao('annotation').fromSQL(db, None, foreignKey, globalProps)
            annotations = res
            
            execRec = DBExecRec(id=id,
                                tsStart=tsStart,
                                tsEnd=tsEnd,
                                moduleId=moduleId,
                                moduleName=moduleName,
                                annotations=annotations)
            execRec.is_dirty = False
            list.append(execRec)

        return list

    def toSQL(self, db, obj, foreignKey=None, globalProps=None):
        keyStr = self.convertToDB(obj.db_id, 'long', 'int')
        if obj.is_dirty:
            columns = ['id']
            table = 'exec'
            whereMap = {}
            columnMap = {}

            whereMap['id'] = keyStr
            if globalProps is not None:
                whereMap.update(globalProps)
            if obj.db_tsStart is not None:
                columnMap['ts_start'] = \
                    self.convertToDB(obj.db_tsStart, 'datetime', 'datetime')
            if obj.db_tsEnd is not None:
                columnMap['ts_end'] = \
                    self.convertToDB(obj.db_tsEnd, 'datetime', 'datetime')
            if obj.db_moduleId is not None:
                columnMap['module_id'] = \
                    self.convertToDB(obj.db_moduleId, 'long', 'int')
            if obj.db_moduleName is not None:
                columnMap['module_name'] = \
                    self.convertToDB(obj.db_moduleName, 'str', 'varchar(255)')
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
        

        discStr = self.convertToDB('execRec','str','char(16)')
        foreignKey = {'parent_id' : keyStr, 'parent_type': discStr}
        for child in obj.db_annotations:
            self.getDao('annotation').toSQL(db, child, foreignKey, globalProps)
        

class DBVistrailSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

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
            actions = {}
            for obj in res:
                actions[obj.db_id] = obj
            
            foreignKey = None
            res = self.getDao('tag').fromSQL(db, None, foreignKey, globalProps)
            tags = {}
            for obj in res:
                tags[obj.db_name] = obj
            
            foreignKey = None
            res = self.getDao('macro').fromSQL(db, None, foreignKey, globalProps)
            macros = {}
            for obj in res:
                macros[obj.db_id] = obj
            
            vistrail = DBVistrail(id=id,
                                  version=version,
                                  name=name,
                                  actions=actions,
                                  tags=tags,
                                  macros=macros)
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
        for child in obj.db_actions.itervalues():
            self.getDao('action').toSQL(db, child, foreignKey, globalProps)
        
        foreignKey = None
        for child in obj.db_tags.itervalues():
            self.getDao('tag').toSQL(db, child, foreignKey, globalProps)
        
        foreignKey = None
        for child in obj.db_macros.itervalues():
            self.getDao('macro').toSQL(db, child, foreignKey, globalProps)
        

class DBDeleteSQLDAOBase(SQLDAO):

    def __init__(self, daoList):
        self.daoList = daoList

    def getDao(self, dao):
        return self.daoList[dao]

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
        


"""generated automatically by auto_dao.py"""

class SQLDAOListBase(dict):

    def __init__(self, daos=None):
        if daos is not None:
            dict.update(self, daos)

        if 'portSpec' not in self:
            self['portSpec'] = DBPortSpecSQLDAOBase(self)
        if 'module' not in self:
            self['module'] = DBModuleSQLDAOBase(self)
        if 'session' not in self:
            self['session'] = DBSessionSQLDAOBase(self)
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
        if 'wfExec' not in self:
            self['wfExec'] = DBWfExecSQLDAOBase(self)
        if 'parameter' not in self:
            self['parameter'] = DBParameterSQLDAOBase(self)
        if 'function' not in self:
            self['function'] = DBFunctionSQLDAOBase(self)
        if 'workflow' not in self:
            self['workflow'] = DBWorkflowSQLDAOBase(self)
        if 'action' not in self:
            self['action'] = DBActionSQLDAOBase(self)
        if 'annotation' not in self:
            self['annotation'] = DBAnnotationSQLDAOBase(self)
        if 'change' not in self:
            self['change'] = DBChangeSQLDAOBase(self)
        if 'macro' not in self:
            self['macro'] = DBMacroSQLDAOBase(self)
        if 'connection' not in self:
            self['connection'] = DBConnectionSQLDAOBase(self)
        if 'tag' not in self:
            self['tag'] = DBTagSQLDAOBase(self)
        if 'execRec' not in self:
            self['execRec'] = DBExecRecSQLDAOBase(self)
        if 'vistrail' not in self:
            self['vistrail'] = DBVistrailSQLDAOBase(self)
        if 'delete' not in self:
            self['delete'] = DBDeleteSQLDAOBase(self)
