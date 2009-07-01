
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

"""Document This"""

from auto_gen_objects import Object, Property

PYTHON_SPACES = 4

class AutoGen:
    def __init__(self, objects):
  self.objects = objects
  self.reset()

    def reset(self, spaces = PYTHON_SPACES):
  self.level = 0
  self.output = ''
  self.refObjects = []
  self.numSpaces = spaces

    def indent(self, indent = 1):
  self.level += indent

    def unindent(self, indent = 1):
  self.level -= indent

    def write(self, string):
  self.output += string

    def printLine(self, string):
  spaces = ''
  for idx in xrange(self.level):
      for jdx in xrange(self.numSpaces):
    spaces +=  ' '
  self.output += spaces + string

    def indentLine(self, string):
  self.indent();
  self.printLine(string)

    def unindentLine(self, string):
  self.unindent()
  self.printLine(string)

    def getOutput(self):
  return self.output

    def getSingleProperties(self, object):
  singleProperties = []
  for property in object.properties:
      if not property.isPlural():
    singleProperties.append(property)
  return singleProperties

    def getPluralProperties(self, object):
  pluralProperties = []
  for property in object.properties:
      if property.isPlural() and not property.isInverse():
    pluralProperties.append(property)
  return pluralProperties

    def getForeignKeys(self, object):
        properties = []
        for property in object.properties:
            if property.isForeignKey():
                properties.append(property)
        return properties

    def getReferences(self, object):
        return self.getReferenceProperties(object) + \
            self.getReferenceChoices(object)

    def getNonInverseReferences(self, object):
        return [ref for ref in self.getReferences(object) \
                    if not ref.isInverse()]

    def getReferenceProperties(self, object):
  refProperties = []
  for property in object.properties:
      if property.isReference():
    refProperties.append(property)
  return refProperties

    def getReferenceChoices(self, object):
        refChoices = []
        for choice in object.choices:
            if choice.isReference():
                refChoices.append(choice)
        return refChoices

    def getNonReferenceProperties(self, object):
  noRefProperties = []
  for property in object.properties:
      if not property.isReference():
    noRefProperties.append(property)
  return noRefProperties

    def getInverseProperties(self, object):
  inverseProperties = []
  for property in object.properties:
      if property.isInverse():
    inverseProperties.append(property)
  return inverseProperties

    def getNonInverseProperties(self, object):
  noInverseProperties = []
  for property in object.properties:
      if not property.isInverse():
    noInverseProperties.append(property)
  return noInverseProperties
    
    def getPythonVarNames(self, object):
  varNames = []
  for field in self.getPythonFields(object):
      varNames.append(field.getRegularName())
  return varNames

    def getPythonFields(self, object):
  fields = []
  for choice in object.choices:
      if not choice.isInverse():
    fields.append(choice)
  for property in object.properties:
      if not property.isInverse():
    fields.append(property)
  return fields

    def getPythonPluralFields(self, object):
        fields = []
        for field in self.getPythonFields(object):
            if field.isPlural():
                fields.append(field)
  return fields
    
    def getPythonLists(self, object):
  fields = []
  for field in self.getPythonPluralFields(object):
      if field.getPythonType() != 'hash':
    fields.append(field)
  return fields

    def getPythonHashes(self, object):
  fields = []
  for field in self.getPythonPluralFields(object):
      if field.getPythonType() == 'hash':
    fields.append(field)
  return fields

    def getReferencedObject(self, refName):
  try:
      return self.objects[refName]
  except KeyError:
      pass
  return None

    def getDiscriminatorProperty(self, object, dName):
        try:
            for property in object.properties:
                if property.getName() == dName:
                    return property
        except KeyError:
            pass
        return None

    def generatePythonCode(self):
  self.reset()
  self.printLine('"""generated automatically by auto_dao.py"""\n\n')
        self.printLine('import copy\n\n')
  for obj in self.objects.itervalues():
      self.generatePythonClass(obj)
  return self.getOutput()

    def getAllIndices(self, field):
        indices = []
        if field.isReference():
            if field.isPlural():
                ref_obj = self.getReferencedObject(field.getReference())
                key = ref_obj.getKey()
                if key is not None:
                    indices.append(key.getRegularName())
            for index in field.getIndices():
                if type(index) == type([]):
                    index_field = []
                    for piece in index:
                        ref_field = ref_obj.getField(piece)
                        if ref_field is not None:
                            index_field.append(ref_field.getRegularName())
                    if len(index_field) > 1:
                        indices.append(index_field)
                    elif len(index_field) > 0:
                        indices.append(index_field[0])
                else:
                    index_field = ref_obj.getField(index)
                    if index_field is not None:
                        indices.append(index_field.getRegularName())
        return indices

    def getIndexName(self, index):
        if type(index) == type([]):
            # return '_'.join(index)
            return index[0]
        else:
            return index

    def getIndexKey(self, field_str, index):
        if type(index) == type([]):
            return '(' + field_str + '.db_' + \
                (',' + field_str + '.db_').join(index) + ')'
        else:
            return field_str + '.db_' + index

    def generatePythonClass(self, object):
  self.printLine('class %s(object):\n\n' % object.getClassName())

        vars = self.getPythonVarNames(object)
  # create constructor
  varInit = []
  for field in self.getPythonFields(object):
      varInit.append('%s=None' % field.getRegularName())


  self.indentLine("vtType = '%s'\n\n" % object.getRegularName())
        self.printLine('def __init__(self, %s):\n' % ', '.join(varInit))
  self.indent()

  for field in self.getPythonFields(object):
            if field.isReference() and not field.isInverse():
                self.printLine('self.db_deleted_%s = []\n' % \
                                   field.getRegularName())
      if field.isPlural():
                for index in self.getAllIndices(field):
                    self.printLine('self.db_%s_%s_index = {}\n' % \
                                       (field.getRegularName(), 
                                        self.getIndexName(index)))
    self.printLine('if %s is None:\n' % field.getRegularName())
    if field.getPythonType() == 'hash':
            self.indentLine('self.%s = {}\n' % field.getPrivateName())
    else:
        self.indentLine('self.%s = []\n' % field.getPrivateName())
    self.unindentLine('else:\n')
    self.indentLine('self.%s = %s\n' % (field.getPrivateName(),
                field.getRegularName()))
                if len(self.getAllIndices(field)) > 0:
                    if field.getPythonType() == 'hash':
                        self.printLine('for v in self.%s.itervalues():\n' % \
                                           field.getPrivateName())
                    else:
                        self.printLine('for v in self.%s:\n' % \
                                           field.getPrivateName())
                    self.indent()
                    for index in self.getAllIndices(field):
                        self.printLine('self.db_%s_%s_index[%s] = v\n' % \
                                           (field.getRegularName(), 
                                            self.getIndexName(index), 
                                            self.getIndexKey('v', index)))
                    self.unindent()
                self.unindent()
      else:
    self.printLine('self.%s = %s\n' % (field.getPrivateName(),
               field.getRegularName()))
        self.printLine('self.is_dirty = True\n')
        self.printLine('self.is_new = True\n')
  self.unindentLine('\n')

        # create copy constructor
        self.printLine('def __copy__(self):\n')
        self.indentLine('return %s.do_copy(self)\n\n' % object.getClassName())

        # create copy w/ new ids
        self.unindentLine('def do_copy(self, new_ids=False, ' +
                          'id_scope=None, id_remap=None):\n')
        # self.indentLine('cp = %s()\n' % object.getClassName())
        
        constructor_pairs = []
        for field in self.getPythonFields(object):
            if not field.isPlural() and not field.isReference():
                constructor_pairs.append("%s=self.%s" % \
                                             (field.getRegularName(),
                                              field.getPrivateName()))
        self.indent()
        returnStr = 'cp = %s' % object.getClassName()
        sep = ',\n' + (' ' * (len(returnStr) + self.level * self.numSpaces + 1))
  self.printLine('%s(%s)\n' % (returnStr, sep.join(constructor_pairs)))

        for field in self.getPythonFields(object):
            if field.isPlural():
                self.printLine('if self.%s is None:\n' % field.getPrivateName())
                if field.getPythonType() == 'hash':
                    self.indentLine('cp.%s = {}\n' % field.getPrivateName())
                else:
                    self.indentLine('cp.%s = []\n' % field.getPrivateName())
                self.unindentLine('else:\n')
                if field.getPythonType() == 'hash':
                    self.indentLine('cp.%s = dict([(k,v.do_copy(new_ids, id_scope, id_remap)) for (k,v) in self.%s.iteritems()])\n' % (field.getPrivateName(), field.getPrivateName()))
                else:
                    self.indentLine('cp.%s = [v.do_copy(new_ids, id_scope, id_remap) for v in self.%s]\n' % (field.getPrivateName(), field.getPrivateName()))
                self.unindent()
            else:
                if field.isReference():
                    self.printLine('if self.%s is not None:\n' % \
                                       field.getPrivateName())
                    self.indentLine('cp.%s = self.%s.do_copy(new_ids, id_scope, id_remap)\n' % (field.getPrivateName(), field.getPrivateName()))
                    self.unindent()
#                 else:
#                     self.printLine('cp.%s = self.%s\n' % (field.getFieldName(),
#                                                           field.getFieldName()))
        self.printLine('\n')
        self.printLine('# set new ids\n')
        self.printLine('if new_ids:\n')
        self.indentLine('new_id = id_scope.getNewId(self.vtType)\n')
        self.printLine('if self.vtType in id_scope.remap:\n')
        self.indentLine('id_remap[(id_scope.remap[self.vtType], self.db_id)] = new_id\n')
        self.unindentLine('else:\n')
        self.indentLine('id_remap[(self.vtType, self.db_id)] = new_id\n')
        self.unindentLine('cp.db_id = new_id\n')

        foreignKeys = self.getForeignKeys(object)
        if len(foreignKeys) > 0:
            for field in foreignKeys:
                if field.hasDiscriminator():
                    disc_prop = \
                        self.getDiscriminatorProperty(object, 
                                                      field.getDiscriminator())
                    lookup_str = "self.%s" % disc_prop.getPrivateName()
                else:
                    ref_obj = self.getReferencedObject(field.getReference())
                    lookup_str = "'%s'" % ref_obj.getRegularName()
                self.printLine("if hasattr(self, '%s') and (%s, self.%s) in id_remap:\n" % \
                                    (field.getFieldName(),
                                     lookup_str,
                                     field.getPrivateName()))
                self.indentLine("cp.%s = id_remap[(%s, self.%s)]\n" % \
                                    (field.getPrivateName(), 
                                     lookup_str,
                                     field.getPrivateName()))
                self.unindent()

        self.unindentLine('\n')
        self.printLine('# recreate indices and set flags\n')
        # recreate indices
        for field in self.getPythonFields(object):
            if len(self.getAllIndices(field)) > 0:
#                 if field.getPythonType() == 'hash':
#                     self.printLine('for v in cp.%s.itervalues():\n' % \
#                                        field.getPrivateName())
#                 else:
#                     self.printLine('for v in cp.%s:\n' % \
#                                        field.getPrivateName())
#                 self.indent()
#                 for index in self.getAllIndices(field):
#                     self.printLine('cp.db_%s_%s_index[%s] = v\n' % \
#                                        (field.getRegularName(), 
#                                         self.getIndexName(index), 
#                                         self.getIndexKey('v',index)))
#                 self.unindent()
                for index in self.getAllIndices(field):
                    self.printLine('cp.db_%s_%s_index = ' % \
                                       (field.getRegularName(),
                                        self.getIndexName(index)) +
                                   'dict((%s, v) for v in cp.%s%s)\n' % \
                                       (self.getIndexKey('v', index),
                                        field.getPrivateName(),
                                        '.itervalues()' if \
                                            field.getPythonType() == 'hash' \
                                            else ''))

        self.printLine('cp.is_dirty = self.is_dirty\n')
        self.printLine('cp.is_new = self.is_new\n')
        self.printLine('return cp\n\n')

        # create child methods
        self.unindentLine('def %s(self, parent=(None,None), orphan=False):\n' \
                              % object.getChildren())
        refs = self.getReferences(object)
        use_one_line = True
        for ref in refs:
            if not ref.isInverse() and ref.shouldExpand():
                use_one_line = False
                break
        if use_one_line:
            self.indentLine('return [(self, parent[0], parent[1])]\n')
        else:
            self.indentLine('children = []\n')
            for ref in refs:
                if ref.isInverse() or not ref.shouldExpand():
                    continue
                refObj = self.getReferencedObject(ref.getReference())
                if not ref.isPlural():
                    self.printLine('if self.%s is not None:\n' % \
                                       ref.getPrivateName())
                    self.indentLine('children.extend(self.%s.%s(' % \
                                       (ref.getPrivateName(), 
                                        refObj.getChildren())+ \
                                       '(self.vtType, self.db_id), orphan))\n')
                    self.printLine('if orphan:\n')
                    self.indentLine('self.%s = None\n' % ref.getPrivateName())
                    self.unindent(2)
                else:
                    self.printLine('to_del = []\n')
                    self.printLine('for child in self.%s:\n' % \
                                       ref.getIterator())
                    self.indentLine('children.extend(child.%s(' % \
                                        refObj.getChildren() + \
                                        '(self.vtType, self.db_id), orphan))\n')
                    self.printLine('if orphan:\n')
                    self.indentLine('to_del.append(child)\n')
                    self.unindent(2)
                    self.printLine('for child in to_del:\n')
                    self.indentLine('self.%s(child)\n' % ref.getRemover())
                    self.unindent()
                    # self.unindentLine('if orphan:\n')
                    # if ref.getType() == 'hash':
                    #     self.indentLine('self.%s = {}\n' % ref.getFieldName())
                    # else:
                    #     self.indentLine('self.%s = []\n' % ref.getFieldName())
                    # self.unindent()
            self.printLine('children.append((self, parent[0], parent[1]))\n')
            self.printLine('return children\n')

        # create get deleted method
        self.unindentLine('def db_deleted_children(self, remove=False):\n')
        refs = self.getNonInverseReferences(object)
        self.indentLine('children = []\n')
        if len(refs) > 0:
            for ref in refs:
                self.printLine('children.extend(self.db_deleted_%s)\n' % \
                                   ref.getRegularName())
            self.printLine('if remove:\n')
            self.indent()
            for ref in refs:
                self.printLine('self.db_deleted_%s = []\n' % \
                                   ref.getRegularName())
            self.unindent()
        self.printLine('return children\n')

        # create dirty method
        self.unindentLine('def has_changes(self):\n')
        self.indentLine('if self.is_dirty:\n')
        self.indentLine('return True\n')

        refs = self.getNonInverseReferences(object)
        for ref in refs:
            if not ref.isPlural():
                self.unindentLine('if self.%s is not None' % \
                                      ref.getPrivateName()
                                  + ' and self.%s.has_changes():\n' % \
                                      ref.getPrivateName())
                self.indentLine('return True\n')
            else:
                if ref.getType() == 'hash':
                    self.unindentLine('for child in self.%s.itervalues():\n' % \
                                          ref.getPrivateName())
                else:
                    self.unindentLine('for child in self.%s:\n' % \
                                          ref.getPrivateName())
                self.indentLine('if child.has_changes():\n')
                self.indentLine('return True\n')
                self.unindent()
        self.unindentLine('return False\n')
        self.unindent()

        # create methods
  for field in self.getPythonFields(object):
      self.printLine('def %s(self):\n' % \
            field.getDefineAccessor())
      self.indentLine('return self.%s\n' % field.getPrivateName())
      self.unindentLine('def %s(self, %s):\n' % \
         (field.getDefineMutator(),
          field.getRegularName()))
      self.indentLine('self.%s = %s\n' % \
          (field.getPrivateName(), 
           field.getRegularName()))
            self.printLine('self.is_dirty = True\n')
      self.unindentLine('%s = property(%s, %s)\n' % \
           (field.getFieldName(),
            field.getDefineAccessor(),
            field.getDefineMutator()))
      if not field.isPlural():
    self.printLine('def %s(self, %s):\n' % \
          (field.getAppender(), field.getName()))
    self.indentLine('self.%s = %s\n' % (field.getPrivateName(),
              field.getName()))
    self.unindentLine('def %s(self, %s):\n' % \
             (field.getModifier(), field.getName()))
    self.indentLine('self.%s = %s\n' % (field.getPrivateName(),
              field.getName()))
    self.unindentLine('def %s(self, %s):\n' % \
             (field.getRemover(), field.getName()))
                if field.isReference() and not field.isInverse():
                    self.indentLine('if not self.is_new:\n')
                    self.indentLine('self.db_deleted_%s.append(self.%s)\n' % \
                                        (field.getRegularName(),
                                         field.getPrivateName()))
                    self.unindent(2)
    self.indentLine('self.%s = None\n' % field.getPrivateName())
    self.unindent()
      else:
    self.printLine('def %s(self):\n' % field.getList())
    if field.getPythonType() == 'hash':
        self.indentLine('return self.%s.values()\n' % \
            field.getPrivateName())
    else:
        self.indentLine('return self.%s\n' % \
            field.getPrivateName())
    self.unindent()
    
    self.printLine('def %s(self, %s):\n' % \
             (field.getAppender(),
        field.getName()))
                self.indentLine('self.is_dirty = True\n')
    if field.getPythonType() == 'hash':
        childObj = self.getReferencedObject(field.getReference())
        self.printLine('self.%s[%s.%s] = %s\n' % \
            (field.getPrivateName(),
             field.getName(),
             childObj.getKey().getPythonName(),
             field.getName()))
    else:
        self.printLine('self.%s.append(%s)\n' % \
                                     (field.getPrivateName(), field.getName()))
                for index in self.getAllIndices(field):
                    self.printLine('self.db_%s_%s_index[%s] = %s\n' % \
                                       (field.getRegularName(),
                                        self.getIndexName(index), 
                                        self.getIndexKey(field.getName(), 
                                                         index),
                                        field.getName()))
    self.unindent()

    self.printLine('def %s(self, %s):\n' % \
             (field.getModifier(), field.getName()))
                self.indentLine('self.is_dirty = True\n')
    if field.getPythonType() == 'hash':
        childObj = self.getReferencedObject(field.getReference())
        self.printLine('self.%s[%s.%s] = %s\n' % \
            (field.getPrivateName(),
             field.getName(),
             childObj.getKey().getPythonName(),
             field.getName()))
    else:
        childObj = self.getReferencedObject(field.getReference())
        self.printLine('found = False\n')
        self.printLine('for i in xrange(len(self.%s)):\n' % \
            field.getPrivateName())
        self.indentLine('if self.%s[i].%s == %s.%s:\n' % \
            (field.getPrivateName(),
             childObj.getKey().getPythonName(),
             field.getName(),
             childObj.getKey().getPythonName()))
        self.indentLine('self.%s[i] = %s\n' % \
            (field.getPrivateName(),
             field.getName()))
        self.printLine('found = True\n')
        self.printLine('break\n')
        self.unindent(2)
        self.printLine('if not found:\n')
        self.indentLine('self.%s.append(%s)\n' % \
            (field.getPrivateName(),
             field.getName()))
        self.unindent()
                for index in self.getAllIndices(field):
                    self.printLine('self.db_%s_%s_index[%s] = %s\n' % \
                                       (field.getRegularName(),
                                        self.getIndexName(index), 
                                        self.getIndexKey(field.getName(),
                                                         index),
                                        field.getName()))
    self.unindent()

    self.printLine('def %s(self, %s):\n' % \
             (field.getRemover(), field.getName()))
                self.indentLine('self.is_dirty = True\n')
    if field.getPythonType() == 'hash':
        childObj = self.getReferencedObject(field.getReference())
                    self.printLine('if not self.%s[%s.%s].is_new:\n' % \
                                       (field.getPrivateName(),
                                        field.getName(),
                                        childObj.getKey().getPythonName()))
                    self.indentLine('self.db_deleted_%s.append(' % \
                                        field.getRegularName() +
                                    'self.%s[%s.%s])\n' % \
                                        (field.getPrivateName(),
                                         field.getName(),
                                         childObj.getKey().getPythonName()))
        self.unindentLine('del self.%s[%s.%s]\n' % \
                                          (field.getPrivateName(),
                                           field.getName(),
                                           childObj.getKey().getPythonName()))
    else:
        childObj = self.getReferencedObject(field.getReference())
                    
        self.printLine('for i in xrange(len(self.%s)):\n' % \
            field.getPrivateName())
        self.indentLine('if self.%s[i].%s == %s.%s:\n' % \
            (field.getPrivateName(),
             childObj.getKey().getPythonName(),
             field.getName(),
             childObj.getKey().getPythonName()))
                    self.indentLine('if not self.%s[i].is_new:\n' % \
                                        field.getPrivateName())
                    self.indentLine('self.db_deleted_%s.append(' % \
                                        field.getRegularName() +
                                    'self.%s[i])\n' % field.getPrivateName())
        self.unindentLine('del self.%s[i]\n' % \
                                          field.getPrivateName())
        self.printLine('break\n')
        self.unindent(2)
                for index in self.getAllIndices(field):
                    self.printLine('del self.db_%s_%s_index[%s]\n' % \
                                       (field.getRegularName(),
                                        self.getIndexName(index), 
                                        self.getIndexKey(field.getName(),
                                                         index)))
    self.unindent()

    self.printLine('def %s(self, key):\n' % field.getLookup())
    if field.getPythonType() == 'hash':
        self.indentLine('if key in self.%s:\n' % \
            field.getPrivateName())
        self.indentLine('return self.%s[key]\n' % \
            field.getPrivateName())
        self.unindentLine('return None\n')
    else:
        self.indentLine('for i in xrange(len(self.%s)):\n' % \
            field.getPrivateName())
        self.indentLine('if self.%s[i].%s == key:\n' % \
            (field.getPrivateName(),
             childObj.getKey().getPythonName()))
        self.indentLine('return self.%s[i]\n' % \
            field.getPrivateName())
        self.unindent(2)
        self.printLine('return None\n')
    self.unindent()
                for index in self.getAllIndices(field):
                    self.printLine('def db_get_%s_by_%s(self, key):\n' % \
                                       (field.getSingleName(), 
                                        self.getIndexName(index)))
                    self.indentLine('return self.db_%s_%s_index[key]\n' % \
                                        (field.getRegularName(), 
                                         self.getIndexName(index)))
                    self.unindentLine('def db_has_%s_with_%s(self, key):\n' % \
                                          (field.getSingleName(), 
                                           self.getIndexName(index)))
                    self.indentLine('return key in self.db_%s_%s_index\n' % \
                                        (field.getRegularName(), 
                                         self.getIndexName(index)))
                    self.unindent()
                                    
      self.printLine('\n')
   
  self.printLine('def getPrimaryKey(self):\n')
  self.indentLine('return self.%s' % \
      object.getKey().getPrivateName())
  self.unindent()
  self.unindentLine('\n\n')

