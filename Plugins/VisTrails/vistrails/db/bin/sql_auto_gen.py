
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

from auto_gen_objects import Object, Property, Choice
from auto_gen import AutoGen
import string

SQL_TYPE = 'sql'
PYTHON_SPACES = 4
SQL_SPACES = 4

class SQLObject (Object):
    def getName(self):
        try:
      return self.layouts[SQL_TYPE]['table']
  except KeyError:
      pass
  return Object.getName(self)
      
class SQLProperty (Property):
    def hasSpec(self):
  try:
      sqlSpec = self.specs[SQL_TYPE]
      return True
  except KeyError:
      pass
  return False

    def getName(self):
  try:
      return self.specs[SQL_TYPE]['name']
  except KeyError:
      pass
  return Property.getName(self)

    def getColumn(self):
  try:
      return self.specs[SQL_TYPE]['column']
  except KeyError:
      pass
  return self.getName()
    
    def getType(self):
  try:
      return self.specs[SQL_TYPE]['type']
  except KeyError:
      pass
  return 'int'

    def getGlobalName(self):
        try:
            return self.specs[SQL_TYPE]['globalName']
        except KeyError:
            pass
        return ''

    def isText(self):
  if string.find(self.getType().upper(), 'CHAR') != -1 or \
    string.find(self.getType().upper(), 'DATE') != -1:
      return True
  return False

    def isAutoInc(self):
        try:
            # FIXME include "and isPrimaryKey()" ?
            return self.specs[SQL_TYPE]['autoInc'] == 'true' and \
                self.isPrimaryKey()
        except KeyError:
            pass
        return False

    def isGlobal(self):
        try:
            return self.specs[SQL_TYPE]['global'] == 'true'
        except KeyError:
            pass
        return False

class SQLChoice(Choice):
    def hasSpec(self):
        if self.properties[0].hasSpec():
            return True
        return False

    def getColumn(self):
        for property in self.properties:
            if property.hasSpec():
                break
  try:
      return property.specs[SQL_TYPE]['column']
  except KeyError:
      pass
  return self.getName()
      
    def isGlobal(self):
        try:
            return self.properties[0].specs[SQL_TYPE]['global'] == 'true'
        except KeyError:
            pass
        return False

    def getGlobalName(self):
        try:
            return self.properties[0].specs[SQL_TYPE]['globalName']
        except KeyError:
            pass
        return ''
  
class SQLAutoGen(AutoGen):
    def __init__(self, objects):
  AutoGen.__init__(self, objects)
  for obj in self.objects.values():
      obj.__class__ = SQLObject
      for property in obj.properties:
    property.__class__ = SQLProperty
            for choice in obj.choices:
                choice.__class__ = SQLChoice
                for property in choice.properties:
                    property.__class__ = SQLProperty

    def reset(self, spaces = SQL_SPACES):
  AutoGen.reset(self, spaces)

    def getSQLPropertiesForChoice(self, choice):
        properties = []
        for property in choice.properties:
            if property.hasSpec():
                properties.append(property)
        return properties

    def getSQLFields(self, object):
        return self.getSQLProperties(object) + self.getSQLChoices(object)

    def getSQLProperties(self, object):
        properties = []
        for property in object.properties:
            if property.hasSpec():
                properties.append(property)
        return properties

    def getSQLChoices(self, object):
        choices = []
        for choice in object.choices:
            if choice.hasSpec():
                choices.append(choice)
        return choices

    def getSQLInverses(self, object):
        fields = []
        for field in self.getSQLFields(object):
            if field.isInverse():
                fields.append(field)
        return fields

    def getSQLInverseRefs(self, object):
        fields = []
        for field in self.getSQLFields(object):
            if field.isInverse() and field.isReference():
                fields.append(field)
        return fields

    def getSQLColumns(self, object):
        columns = []
        for field in self.getSQLFields(object):
            columns.append(field.getColumn())
        return columns

    def getSQLReferences(self, object):
        refs = []
        for property in object.properties:
            if property.isReference() and not property.isInverse():
                refs.append(property)
        for choice in object.choices:
            if choice.isReference() and not choice.isInverse():
                refs.append(choice)
        return refs

    def getNormalSQLColumns(self, object):
        columns = []
        for property in object.properties:
            if not property.isInverse() and not property.isPrimaryKey() and \
               not property.isReference() and property.hasSpec():
                columns.append(property)
        return columns

    def getNormalSQLColumnsAndKey(self, object):
        columns = self.getNormalSQLColumns(object)
  columns.append(object.getKey())
        return columns

    def getSQLColumnsAndKey(self, object):
  columns = []
  for property in self.getNormalSQLColumnsAndKey(object):
      columns.append(property.getColumn())
  return columns

    def getSQLReferenceProperties(self, object):
  refs = []
  for property in object.properties:
      if not property.isInverse() and property.isReference():
    refs.append(property)
  return refs

    def getSQLForeignKeys(self, object):
        keys = []
        for property in object.properties:
            if property.isInverse() and property.isForeignKey():
                keys.append(property)
        return keys
    
    def getSQLReferencedField(self, refObj, object):
  if refObj is not None:
      # find inverse
      for refProp in refObj.properties:
    if refProp.isReference() and \
      refProp.isInverse() and \
      refProp.getReference() == object.getRegularName():
        return (refProp, False)
      for choice in refObj.choices:
    for refProp in self.getSQLPropertiesForChoice(choice):
        if refProp.isReference() and \
          refProp.getReference() == object.getRegularName():
      return (choice, True)
  return (None, False)

    def get_sql_referenced(self, ref_obj, obj):
        if ref_obj is not None:
            for ref_prop in ref_obj.properties:
                if ref_prop.isReference() and \
                        ref_prop.getReference() == obj.getRegularName():
                    return (ref_prop, False)
            for choice in ref_obj.choices:
                for ref_prop in choice.properties:
                    if ref_prop.isReference() and \
                            ref_prop.getReference() == obj.getRegularName():
                        return (choice, True)
        raise Exception("didn't work")

    def generateSchema(self):
  self.reset(SQL_SPACES)
        self.printLine('-- generated automatically by auto_dao.py\n\n')
        for obj in self.objects.values():
            self.generateTable(obj)
        return self.getOutput()

    def generateTable(self, object):
        self.printLine('CREATE TABLE %s(\n' % object.getName())

        comma = ''
        self.indent();
        for property in object.properties:
            if property.hasSpec():
    self.write(comma)
    self.printLine('%s %s' % \
             (property.getColumn(), property.getType()))
                if property.isAutoInc():
                    self.write(' not null auto_increment primary key')
    comma = ',\n'
        for choice in object.choices:
            if choice.isInverse():
                for property in choice.properties:
                    if property.hasSpec():
                        break
                self.write(comma)
                self.printLine('%s %s' % \
                               (property.getColumn(), property.getType()))
                comma = ',\n'
  self.unindentLine('\n) engine=InnoDB;\n\n')

    def generateDeleteSchema(self):
  self.reset(SQL_SPACES)
  self.printLine('-- genereated automatically by generate.py\n\n')
        self.printLine('DROP TABLE IF EXISTS')
        comma = ''
  for obj in self.objects.values():
            self.write('%s %s' % (comma, obj.getName()))
            comma = ','
        self.write('\n')
  return self.getOutput()

    def generateDAOList(self):
  self.reset(PYTHON_SPACES)
  self.printLine('"""generated automatically by auto_dao.py"""\n\n')

  self.printLine('class SQLDAOListBase(dict):\n\n')
  self.indentLine('def __init__(self, daos=None):\n')
  self.indentLine('if daos is not None:\n')
  self.indentLine('dict.update(self, daos)\n\n')
  for obj in self.objects.values():
      self.unindentLine('if \'%s\' not in self:\n' % \
         obj.getRegularName())
      self.indentLine('self[\'%s\'] = %sSQLDAOBase(self)\n' % \
         (obj.getRegularName(), 
          obj.getClassName()))
  return self.getOutput()

    def generateDAO(self, version):
  self.reset(SQL_SPACES)
  self.printLine('"""generated automatically by auto_dao.py"""\n\n')
  self.printLine('from sql_dao import SQLDAO\n')
  self.printLine('from db.versions.%s.domain import *\n\n' % version)
  for obj in self.objects.values():
      self.generateDAOClass(obj)
  return self.getOutput()

    def generateDAOClass(self, object):
        print 'generating sql: %s' % object.getRegularName()
  self.printLine('class %sSQLDAOBase(SQLDAO):\n\n' % \
           object.getClassName())
  self.indentLine('def __init__(self, daoList):\n')
  self.indentLine('self.daoList = daoList\n\n')
  self.unindentLine('def getDao(self, dao):\n')
  self.indentLine('return self.daoList[dao]\n\n')

  refs = self.getSQLReferenceProperties(object)
  varPairs = []
  for field in self.getPythonFields(object):
            if field.__class__ == SQLChoice or field.isReference() or \
                    field.hasSpec():
                varPairs.append('%s=%s' % (field.getRegularName(),
                                           field.getRegularName()))

  key = object.getKey()
  
  choiceRefs = []
  for choice in object.choices:
      if not choice.isInverse() and choice.properties[0].isReference():
    choiceRefs.append(choice)

        # get_sql_columns
        self.unindentLine('def get_sql_columns(self, db, global_props,' + \
                              'lock=False):\n')
        self.indentLine("columns = ['%s']\n" % \
                            "', '".join(self.getSQLColumns(object)))
        self.printLine("table = '%s'\n" % object.getName())
        self.printLine("whereMap = global_props\n")
        self.printLine("orderBy = '%s'\n\n" % key.getName())
        self.printLine('dbCommand = self.createSQLSelect(' +
                       'table, columns, whereMap, orderBy, lock)\n')

        self.printLine('data = self.executeSQL(db, dbCommand, True)\n')
        self.printLine('res = {}\n')
        self.printLine('for row in data:\n')
        self.indent()
  count = 0
  for field in self.getSQLFields(object):
      self.printLine("%s = self.convertFromDB(row[%d], '%s', '%s')\n" % \
         (field.getRegularName(), count,
                            field.getPythonType(), field.getType()))
            count += 1
            if field.isGlobal():
                self.printLine("if not global_props.has_key('%s'):\n" % \
                               field.getGlobalName())
                self.indentLine("global_props['%s'] = " % \
                                    field.getGlobalName() +
                                "self.convertToDB(%s, '%s', '%s')\n" % \
                                    (field.getRegularName(),
                                     field.getPythonType(),
                                     field.getType()))
                self.unindent()

        attrPairs = []
        for field in self.getNormalSQLColumnsAndKey(object):
            attrPairs.append('%s=%s' % (field.getRegularName(),
                                        field.getRegularName()))

        self.printLine('\n')
        assignStr = '%s = %s(' % (object.getRegularName(),
                                  object.getClassName())
        sep = ',\n' + (' ' * (len(assignStr) + 12))
  self.printLine('%s = %s(%s)\n' % (object.getRegularName(), 
                                          object.getClassName(),
                                          sep.join(attrPairs)))
        for field in self.getSQLInverses(object):
            self.printLine('%s.%s = %s\n' % (object.getRegularName(),
                                             field.getFieldName(),
                                             field.getRegularName()))
        self.printLine('%s.is_dirty = False\n' % object.getRegularName())
  self.printLine("res[('%s', %s)] = %s\n\n" % \
                           (object.getRegularName(), key.getRegularName(),
                            object.getRegularName()))
  self.unindentLine('return res\n\n')

        # from_sql_fast
        self.unindentLine('def from_sql_fast(self, obj, all_objects):\n')
        
        # identify the fields needed to tie the child to the parent
        self.indent()
        inverse_refs = self.getSQLInverseRefs(object)
        if len(inverse_refs) < 1:
            self.printLine('pass\n')
        for backRef in inverse_refs:
            cond = "if"
            if backRef.isChoice():
                # need discriminator
                disc = backRef.getDiscriminator()
                for prop in backRef.properties:
                    ref_obj = \
                        self.getReferencedObject(prop.getReference())
                    disc_prop = self.getDiscriminatorProperty(object,
                                                              disc)
                    (ref_prop, _) = self.get_sql_referenced(ref_obj,
                                                            object)
                    self.printLine("%s obj.%s == '%s':\n" % \
                                       (cond, disc_prop.getFieldName(),
                                        ref_obj.getRegularName()))

                    self.indentLine("p = all_objects[('%s', obj.%s)]\n" % \
                                        (ref_obj.getRegularName(),
                                         backRef.getFieldName()))
                    self.printLine('p.%s(obj)\n' % ref_prop.getAppender())
                    self.unindent()
                    cond = "elif"
            else:
                ref_obj = self.getReferencedObject(backRef.getReference())
                (ref_field, _) = self.get_sql_referenced(ref_obj,
                                                         object)
                self.printLine("p = all_objects[('%s', obj.%s)]\n" % \
                                   (ref_obj.getRegularName(),
                                    backRef.getFieldName()))
                self.printLine('p.%s(obj)\n' % ref_field.getAppender())
        self.printLine('\n')

        # set_sql_columns
        self.unindentLine('def set_sql_columns(self, db, obj, global_props, do_copy=True):\n')
        self.indentLine('if not do_copy and not obj.is_dirty:\n')
        self.indentLine('return\n')
        self.unindentLine("columns = ['%s']\n" % \
                              "', '".join(self.getSQLColumns(object)))
        self.printLine("table = '%s'\n" % object.getName())
        self.printLine("whereMap = {}\n")
        self.printLine("whereMap.update(global_props)\n")
  self.printLine('if obj.%s is not None:\n' % key.getFieldName())
        self.indentLine("keyStr = self.convertToDB(obj.%s, '%s', '%s')\n" % \
                            (key.getFieldName(), key.getPythonType(), 
                             key.getType()))
        self.printLine("whereMap['%s'] = keyStr\n" % key.getColumn())
        self.unindent()
#         for field in self.getSQLInverses(object):
#             self.printLine('%s = %s.%s\n' % (field.getRegularName(),
#                                              object.getRegularName(),
#                                              field.getPythonName()))
        self.printLine('columnMap = {}\n')
        for field in self.getSQLFields(object):
      self.printLine("if hasattr(obj, '%s') and obj.%s is not None:\n" % \
                               (field.getPythonName(), field.getPythonName()))
            self.indentLine("columnMap['%s'] = \\\n" % field.getColumn())
            self.indentLine("self.convertToDB(obj.%s, '%s', '%s')\n" % \
                                (field.getFieldName(),
                                 field.getPythonType(),
                                 field.getType()))
            self.unindent(2)
        self.printLine('columnMap.update(global_props)\n\n')

        self.printLine('if obj.is_new or do_copy:\n')
        self.indentLine('dbCommand = self.createSQLInsert(table, columnMap)\n')
        self.unindentLine('else:\n')
        self.indentLine('dbCommand = ' +
                        'self.createSQLUpdate(table, columnMap, whereMap)\n')
        self.unindentLine('lastId = self.executeSQL(db, dbCommand, False)\n')
        if key.isAutoInc():
            self.printLine('if obj.%s is None:\n' % key.getPythonName())
            self.indentLine('obj.%s = lastId\n' % key.getPythonName())
            self.printLine("keyStr = self.convertToDB(obj.%s, '%s', '%s')\n" % \
                               (key.getPythonName(), key.getPythonType(),
                                key.getType()))
            self.unindent()
        for property in self.getNormalSQLColumnsAndKey(object):
            if property.isGlobal():
#                 self.printLine("if not global_props.has_key('%s'):\n" % \
#                                property.getGlobalName())
                self.printLine("if hasattr(obj, '%s') and obj.%s " % \
                                   (property.getPythonName(), 
                                    property.getPythonName()) + \
                                   "is not None:\n")
                self.indentLine("global_props['%s'] = " % \
                                    property.getGlobalName() +
                                "self.convertToDB(obj.%s, '%s', '%s')\n" % \
                                    (property.getPythonName(),
                                     property.getPythonType(),
                                     property.getType()))
                self.unindent()

        self.printLine('\n')

        # to_sql_fast
        self.unindentLine('def to_sql_fast(self, obj, do_copy=True):\n')
        self.indent()
        references = self.getSQLReferences(object)
        if len(references) < 1:
            self.printLine('pass\n')
        else:
            self.printLine('if not do_copy and not obj.is_dirty:\n')
            self.indentLine('return\n')
            self.unindent()
        for ref in references:
            ref_obj = self.getReferencedObject(ref.getReference())
            try:
                (ref_field, is_choice) = \
                    self.get_sql_referenced(ref_obj, object)
            except Exception:
                print "can't find tie between %s and %s" % (ref_obj.getName(),
                                                            object.getName())
                continue
            if ref.isPlural():
                self.printLine('for child in obj.%s:\n' % ref.getIterator())
                self.indent()
            else:
                self.printLine('if obj.%s is not None:\n' % ref.getFieldName())
                self.indentLine('child = obj.%s\n' % ref.getFieldName())
            if is_choice:
                # need to set discriminator and foreign key
                disc = ref_field.getDiscriminator()
                disc_prop = self.getDiscriminatorProperty(ref_obj,
                                                          disc)
                self.printLine("child.%s = obj.vtType\n" % \
                                   disc_prop.getFieldName())
                self.printLine("child.%s = obj.db_id\n" % \
                                   ref_field.getFieldName())
            else:
                # need to set foreign key
                self.printLine("child.%s = obj.db_id\n" % \
                                   ref_field.getFieldName())
            self.unindent()
        self.printLine('\n')

        # delete_sql_column
        self.unindentLine('def delete_sql_column(self, db, obj, ' + \
                              'global_props):\n')
        self.indentLine("table = '%s'\n" % object.getName())
        self.printLine('whereMap = {}\n')
        self.printLine('whereMap.update(global_props)\n')
  self.printLine('if obj.%s is not None:\n' % key.getFieldName())
        self.indentLine("keyStr = self.convertToDB(obj.%s, '%s', '%s')\n" % \
                            (key.getFieldName(), key.getPythonType(), 
                             key.getType()))
        self.printLine("whereMap['%s'] = keyStr\n" % key.getColumn())
        self.unindentLine('dbCommand = self.createSQLDelete(table, whereMap)\n')
        self.printLine('self.executeSQL(db, dbCommand, False)\n\n')
        
        self.unindent(2)
