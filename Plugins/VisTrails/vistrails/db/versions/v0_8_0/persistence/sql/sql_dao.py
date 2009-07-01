
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

class SQLDAO:
    def __init__(self):
  pass

    def convertFromDB(self, value, type, db_type):
  if value is not None:
      if type == 'str':
    return str(value)
            elif type == 'long':
                return long(value)
            elif type == 'float':
                return float(value)
            elif type == 'int':
                return int(value)
            elif type == 'date':
                if db_type == 'date':
                    return value
                else:
                    return date(*strptime(str(value), '%Y-%m-%d')[0:3])
            elif type == 'datetime':
                if db_type == 'datetime':
                    return value
                else:
                    return datetime(*strptime(str(value), 
                                              '%Y-%m-%d %H:%M:%S')[0:6])
  return None

    def convertToDB(self, value, type, db_type):
        if value is not None:
            if type == 'str':
                return "'" + str(value).replace("'", "''") + "'"
            elif type == 'long':
                return str(value)
            elif type == 'float':
                return str(value)
            elif type == 'int':
                return str(value)
            elif type == 'date':
                return "'" + value.isoformat() + "'"
            elif type == 'datetime':
                return "'" + value.strftime('%Y-%m-%d %H:%M:%S') + "'"
            else:
                return str(value)

        return "''"

    def createSQLSelect(self, table, columns, whereMap, orderBy=None, 
                        forUpdate=False):
        columnStr = ', '.join(columns)
        whereStr = ''
        whereClause = ''
        for column, value in whereMap.iteritems():
            whereStr += '%s %s = %s' % \
                        (whereClause, column, value)
            whereClause = ' AND'
        dbCommand = """SELECT %s FROM %s WHERE %s""" % \
                    (columnStr, table, whereStr)
        if orderBy is not None:
            dbCommand += " ORDER BY " + orderBy
        if forUpdate:
            dbCommand += " FOR UPDATE"
        dbCommand += ";"
        return dbCommand

    def createSQLInsert(self, table, columnMap):
        columns = []
        values = []
        for column, value in columnMap.iteritems():
      if value is None:
    value = 'NULL'
      columns.append(column)
      values.append(value)
        columnStr = ', '.join(columns)
        valueStr = ', '.join(values)
        dbCommand = """INSERT INTO %s(%s) VALUES (%s);""" % \
                    (table, columnStr, valueStr)
        return dbCommand

    def createSQLUpdate(self, table, columnMap, whereMap):
        setStr = ''
        comma = ''
        for column, value in columnMap.iteritems():
      if value is None:
    value = 'NULL'
      setStr += '%s %s = %s' % (comma, column, value)
      comma = ','
        whereStr = ''
        whereClause = ''
        for column, value in whereMap.iteritems():
            whereStr += '%s %s = %s' % \
                        (whereClause, column, value)
            whereClause = ' AND'
        dbCommand = """UPDATE %s SET %s WHERE %s;""" % \
                    (table, setStr, whereStr)
        return dbCommand

    def createSQLDelete(self, table, whereMap):
        whereStr = ''
        whereClause = ''
        for column, value in whereMap.iteritems():
            whereStr += '%s%s = %s' % \
                (whereClause, column, value)
            whereClause = ' AND '
        dbCommand = """DELETE FROM %s WHERE %s;""" % \
            (table, whereStr)
        return dbCommand

    def executeSQL(self, db, dbCommand, isFetch):
        # print 'db: %s' % dbCommand
        data = None
        cursor = db.cursor()
        cursor.execute(dbCommand)
        if isFetch:
            data = cursor.fetchall()
        else:
            data = cursor.lastrowid
        cursor.close()
        return data

    def start_transaction(self, db):
        db.begin()

    def commit_transaction(self, db):
        db.commit()

    def rollback_transaction(self, db):
        db.rollback()
