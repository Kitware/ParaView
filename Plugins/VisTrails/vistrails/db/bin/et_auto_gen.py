
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

XML_TYPE = 'xml'
XML_SPACES = 2
PYTHON_SPACES = 4

XML_SCHEMA_HEADER = \
"""<?xml version="1.0"?>
<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema" elementFormDefault="qualified" attributeFormDefault="unqualified">
"""
XML_SCHEMA_FOOTER = \
"""</xs:schema>
"""

class XMLObject(Object):
    def __init__(self):
  Object.__init__(self)
  
    def getName(self):
  try:
      return self.layouts[XML_TYPE]['name']
  except KeyError:
      pass
  return Object.getName()

    def getNodeType(self):
  try:
      return self.layouts[XML_TYPE]['nodeType']
  except KeyError:
      pass
  return 'xs:element'

class XMLProperty(Property):
    def __init__(self):
  Property.__init__(self)

    def hasSpec(self):
        return self.specs.has_key(XML_TYPE)

    def getName(self):
  try:
      return self.specs[XML_TYPE]['name']
  except KeyError:
      pass
  return Property.getName(self)

    def getNodeType(self):
  try:
      return self.specs[XML_TYPE]['nodeType']
  except KeyError:
      pass
  return 'xs:attribute'

    def getAttributeType(self):
  try:
      return self.specs[XML_TYPE]['type']
  except KeyError:
      pass
  return 'xs:string'

    def getAttributeUse(self):
  try:
      return self.specs[XML_TYPE]['use']
  except KeyError:
      pass
  return None

    def getChoice(self):
  try:
      return self.specs[XML_TYPE]['choice']
  except KeyError:
      pass
  return None

    def isInferred(self):
  try:
      return self.specs[XML_TYPE]['inferred'] == 'true'
  except KeyError:
      pass
  return False

class XMLChoice(Choice):
    def __init__(self):
  Choice.__init__(self)

    def hasSpec(self):
        return self.properties[0].hasSpec()

class XMLAutoGen(AutoGen):
    def __init__(self, objects):
  AutoGen.__init__(self, objects)
  for obj in self.objects.values():
      obj.__class__ = XMLObject
      for property in obj.properties:
    property.__class__ = XMLProperty
      for choice in obj.choices:
    choice.__class__ = XMLChoice
    for property in choice.properties:
        property.__class__ = XMLProperty

    def reset(self, spaces = XML_SPACES):
  AutoGen.reset(self, spaces)

    def getXMLAttributes(self, object):
  attributes = []
  for property in object.properties:
      if property.hasSpec() and not property.isInferred() and \
                   property.getNodeType() == 'xs:attribute':
    attributes.append(property)
  return attributes

    def getXMLElements(self, object):
  elements = []
  for property in object.properties:
      if property.hasSpec() and not property.isInferred() and \
                   property.getNodeType() == 'xs:element':
    elements.append(property)
  return elements

    def getXMLChoices(self, object):
  choices = []
  for choice in object.choices:
      if not choice.isInverse():
    for property in choice.properties:
        if property.hasSpec():
      choices.append(choice)
      break;
  return choices
  
    def isXMLChoice(self, object):
  if isinstance(object, XMLChoice):
      return True
  return False

    def getXMLPropertiesForChoice(self, choice):
  choiceProps = []
  for property in choice.properties:
      if property.hasSpec():
    choiceProps.append(property)
  return choiceProps

    def getXMLInferredProperties(self, object):
        inferred = []
        for property in object.properties:
            if property.isInferred():
                inferred.append(property)
        return inferred

    def generateSchema(self, rootName):
  self.reset(XML_SPACES)
  self.printLine(XML_SCHEMA_HEADER)

  root = self.objects[rootName]
  
   self.indentLine('<%s name="%s">\n' % \
      (root.getNodeType(), root.getName()))
  self.generateSchemaForObject(root)
  self.printLine('</%s>\n' % root.getNodeType())

  for obj in self.refObjects:
      self.printLine('<%s name="%s">\n' % \
    (obj.getNodeType(), obj.getName()))
      self.generateSchemaForObject(obj)
      self.printLine('</%s>\n' % obj.getNodeType())

  self.unindentLine(XML_SCHEMA_FOOTER)
  return self.getOutput()
  
    def generateSchemaForObject(self, object):
  elements = self.getXMLElements(object)
  attrs = self.getXMLAttributes(object)

  if not self.isXMLChoice(object):
      choices = self.getXMLChoices(object)
  
      if len(elements) + len(attrs) + len(choices) > 0:
    self.indentLine('<xs:complexType>\n')
      if len(elements) > 0:
    self.indentLine('<xs:sequence>\n')
  else:
      choices = []

  for property in elements:
      if property.isReference():
    minOccurs = '0'
    maxOccurs = '1'
    if property.getMapping() == 'one-to-many':
        maxOccurs = 'unbounded'

    # find reference
    refObj = self.getReferencedObject(property.getReference())
    if refObj is not None and refObj not in self.refObjects:
        self.refObjects.append(refObj)
    
    if property.getName() == refObj.getName():
        self.indentLine('<xs:element ref="%s" ' % \
            refObj.getName() + \
            'minOccurs="%s" maxOccurs="%s"/>\n' % \
            (minOccurs, maxOccurs))
    else:
        self.indentLine('<xs:element name="%s" ref="%s" ' % \
            (property.getName(), refObj.getName()) + \
            'minOccurs="%s" maxOccurs="%s"/>\n' % \
            (minOccurs, maxOccurs))
      else:
    minOccurs = '0'
    maxOccurs = 'unbounded'
    self.indentLine('<xs:element name="%s" '% property.getName() + \
        'minOccurs="%s" maxOccurs="%s"/>\n' % \
        (minOccurs, maxOccurs))
      self.unindent()

  if len(choices) > 0:
      for choice in choices:
    self.indentLine('<xs:choice>\n')
    self.generateSchemaForObject(choice)
    self.printLine('</xs:choice>\n')
    self.unindent()

  if len(elements) > 0 and not self.isXMLChoice(object):
      self.printLine('</xs:sequence>\n')
      self.unindent()

  for property in attrs:
      if property.getAttributeUse() is not None:
    use = ' use=%s' & property.getAttributeUse()
      else:
    use = ''
      self.indentLine('<xs:attribute name="%s" type="%s"%s/>\n' % \
         (property.getName(), property.getAttributeType(), 
          use))
      self.unindent()

  if len(elements) + len(attrs) + len(choices) > 0 and \
    not self.isXMLChoice(object):
      self.printLine('</xs:complexType>\n')
      self.unindent()

    def generateDAOList(self):
  self.reset(PYTHON_SPACES)
  self.printLine('"""generated automatically by auto_dao.py"""\n\n')

  self.printLine('class XMLDAOListBase(dict):\n\n')
  self.indentLine('def __init__(self, daos=None):\n')
  self.indentLine('if daos is not None:\n')
  self.indentLine('dict.update(self, daos)\n\n')
  for obj in self.objects.values():
      self.unindentLine('if \'%s\' not in self:\n' % \
         obj.getRegularName())
      self.indentLine('self[\'%s\'] = %sXMLDAOBase(self)\n' % \
         (obj.getRegularName(), 
          obj.getClassName()))
  return self.getOutput()

    def generateDAO(self, version):
  self.reset(PYTHON_SPACES)
  self.printLine('"""generated automatically by auto_dao.py"""\n\n')
        # self.printLine('from elementtree import ElementTree\n')
        self.printLine('from core.system import get_elementtree_library\n')
        self.printLine('ElementTree = get_elementtree_library()\n\n')
  self.printLine('from xml_dao import XMLDAO\n')
  self.printLine('from db.versions.%s.domain import *\n\n' % version)
  for obj in self.objects.values():
      self.generatePythonDAOClass(obj)
  return self.getOutput()

    def generatePythonDAOClass(self, object):
        print 'generating xml: %s' % object.getRegularName()
  self.printLine('class %sXMLDAOBase(XMLDAO):\n\n' % \
           object.getClassName())
  self.indentLine('def __init__(self, daoList):\n')
  self.indentLine('self.daoList = daoList\n\n')
  self.unindentLine('def getDao(self, dao):\n')
  self.indentLine('return self.daoList[dao]\n\n')

  attrs = self.getXMLAttributes(object)
  elements = self.getXMLElements(object)
  choices = self.getXMLChoices(object)
        varPairs = []
        for field in self.getPythonFields(object):
            if field.hasSpec():
                varPairs.append('%s=%s' % (field.getRegularName(), 
                                           field.getRegularName()))

  # define fromXML function
  self.unindentLine('def fromXML(self, node):\n')
  self.indentLine('if node.tag != \'%s\':\n' % object.getName())
  self.indentLine('return None\n')
  self.unindent()

  if len(attrs) > 0:
      self.printLine('\n')
      self.printLine('# read attributes\n')
      for property in attrs:
                self.printLine("data = node.get('%s', None)\n" % \
                                   property.getName())
                self.printLine("%s = self.convertFromStr(data, '%s')\n" % \
                                    (property.getRegularName(),
                                     property.getPythonType()))

        def generatePropertyParseCode(property, cond):
            if property.isReference():
                refObj = self.getReferencedObject(property.getSingleName())
                propertyName = refObj.getName()
            else:
                propertyName = property.getName()
            self.printLine("%s child.tag == '%s':\n" % (cond, propertyName))

            if property.isReference():
                self.indentLine("_data = self.getDao('%s').fromXML(child)\n" % \
                                    property.getReference())
            else:
                self.indentLine("_data = " + 
                                "self.convertFromStr(child.text,'%s')\n" % \
                                     property.getReference())

        def generateFieldStoreCode(field):
            if field.isPlural():
                if field.getPythonType() == 'hash':
                    if field.isReference():
                        childObj = \
                            self.getReferencedObject(field.getReference())
                        key = childObj.getKey().getFieldName()
                    else:
                        key = ''
                    self.printLine("%s[_data.%s] = _data\n" % \
                                       (field.getRegularName(),
                                        key))
                else:
                    self.printLine('%s.append(_data)\n' % \
                                       field.getRegularName())
            else:
                self.printLine('%s = _data\n' % field.getRegularName())
            

  if len(elements) + len(choices) > 0:
      self.printLine('\n')
            for field in elements + choices:
                if not field.isPlural():
                    self.printLine('%s = None\n' % field.getRegularName())
                else:
                    if field.getPythonType() == 'hash':
                        self.printLine('%s = {}\n' % field.getRegularName())
                    else:
                        self.printLine('%s = []\n' % field.getRegularName())

            self.printLine('\n')
      self.printLine('# read children\n')
      self.printLine('for child in node.getchildren():\n')
            self.indent()

      cond = 'if'
            for field in elements + choices:
                if field.isChoice():
                    for property in self.getXMLPropertiesForChoice(field):
                        generatePropertyParseCode(property, cond)
                        generateFieldStoreCode(field)
                        cond = 'elif'
                        self.unindent()
                else:
                    generatePropertyParseCode(field, cond)
                    generateFieldStoreCode(field)
                    cond = 'elif'
                    self.unindent()
            self.printLine("elif child.text.strip() == '':\n")
            self.indentLine('pass\n')
            self.unindentLine('else:\n')
      self.indentLine('print \'*** ERROR *** tag = %s\' % ' +
          'child.tag\n')
      self.unindent(2)

        self.printLine('\n')
        returnStr = 'obj = %s(' % object.getClassName()
        sep = ',\n' + (' ' * (len(returnStr) + 8))
  self.printLine('obj = %s(%s)\n' % \
           (object.getClassName(), sep.join(varPairs)))
        self.printLine('obj.is_dirty = False\n')
        self.printLine('return obj\n')
  self.unindent(1)
  self.printLine('\n')

  # define toXML function
  self.printLine('def toXML(self, %s, node=None):\n' % \
                           object.getRegularName())

#         self.indentLine('if not %s.has_changes():\n' % object.getRegularName())
#         self.indentLine('return\n')
        self.indentLine('if node is None:\n')
   self.indentLine('node = ElementTree.Element(\'%s\')\n' % \
       object.getName())
        
        self.unindent()
  if len(attrs) > 0:
      self.printLine('\n')
      self.printLine('# set attributes\n')
      for property in attrs:
    self.printLine("node.set('%s'," % property.getName() +
             "self.convertToStr(%s.%s, '%s'))\n" % \
             (object.getRegularName(),
        property.getFieldName(),
        property.getPythonType()))

        self.printLine('\n')


        def generatePropertyOutputCode(property, field=None):
            self.printLine("childNode = ElementTree.SubElement(" +
                           "node, '%s')\n" % property.getSingleName())
            if property.isReference():
                if field is None:
                    field = property
                self.printLine("self.getDao('%s')" % property.getReference() +
                               ".toXML(%s, childNode)\n" % \
                                   field.getSingleName())
            else:
                self.printLine("childNode.text = " + \
                                   "self.convertToStr(child, '%s'))\n" % \
                                   property.getPythonType())

        if len(elements) + len(choices) > 0:
      self.printLine('# set elements\n')
      for field in elements + choices:
    if field.isReference():
        self.printLine('%s = %s.%s\n' % \
           (field.getRegularName(),
                                    object.getRegularName(), 
            field.getFieldName()))
        if field.isPlural():
      if field.getPythonType() == 'hash':
          self.printLine('for %s in %s.itervalues():\n' % \
             (field.getSingleName(),
              field.getRegularName()))
      else:
          self.printLine('for %s in %s:\n' % \
             (field.getSingleName(),
              field.getRegularName()))
                    else:
                        self.printLine('if %s is not None:\n' % \
                                           field.getSingleName())
                    self.indent()
                    if field.isChoice():
                        cond = 'if'
                        for property in self.getXMLPropertiesForChoice(field):
                            self.printLine("%s %s.vtType == '%s':\n" % \
                                               (cond,
                                                field.getSingleName(),
                                                property.getSingleName()))
                            self.indent()
                            generatePropertyOutputCode(property, field)
                            self.unindent()
                            cond = 'elif'
                    else:
                        generatePropertyOutputCode(field)
                    self.unindent()
      self.printLine('\n')
        self.printLine('return node\n\n')

  self.unindent(2)
