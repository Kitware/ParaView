
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
  try:
      xmlSpec = self.specs[XML_TYPE]
      return True
  except KeyError:
      pass
  return False

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
        # FIXME need these to be relative to version
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
  lists = self.getPythonLists(object)
  hashes = self.getPythonHashes(object)
  inferred = self.getXMLInferredProperties(object)
  choices = self.getXMLChoices(object)
  vars = self.getPythonVarNames(object)
        varPairs = []
        for var in vars:
            varPairs.append('%s=%s' % (var, var))

  # define fromXML function
  self.unindentLine('def fromXML(self, node):\n')
  self.indentLine('if node.nodeName != \'%s\':\n' % object.getName())
  self.indentLine('return None\n')
  self.unindent()

  if len(lists) > 0:
      self.printLine('\n')
      for field in lists:
    self.printLine('%s = []\n' % field.getRegularName())

  if len(hashes) > 0:
      self.printLine('\n')
      for field in hashes:
    self.printLine('%s = {}\n' % field.getRegularName())

  if len(attrs) > 0:
      self.printLine('\n')
      self.printLine('# read attributes\n')
      for property in attrs:
    self.printLine("%s = self.convertFromStr(" % \
             property.getRegularName() +
             "self.getAttribute(node, '%s'), '%s')\n" % \
             (property.getName(),
        property.getPythonType()))

  if len(elements) + len(choices) > 0:
      self.printLine('\n')
            for property in elements:
                if not property.isPlural():
                    self.printLine('%s = None\n' % property.getRegularName())
            for choice in choices:
                if not choice.isPlural():
                    self.printLine('%s = None\n' % choice.getRegularName())
            self.printLine('\n')
      self.printLine('# read children\n')
      self.printLine('for child in list(node.childNodes):\n')

      cond = 'if'
      for property in elements:
                if property.isReference():
                    refObj = self.getReferencedObject(property.getReference())
                    propertyName = refObj.getName()
                else:
                    propertyName = property.getName()
                self.indentLine("%s child.nodeName == '%s':\n" % \
                                    (cond, propertyName))
    self.generateChildParsingCode(property, property, cond)
                self.unindent(2)
    cond = 'elif'

      for choice in choices:
    for property in self.getXMLPropertiesForChoice(choice):
#                     discProperty = self.getDiscriminatorProperty(object, \
#                         choice.getDiscriminator())
                    refObj = self.getReferencedObject(property.getSingleName())
                    self.indentLine("%s child.nodeName == '%s':\n" % \
                                        (cond, refObj.getName()))
#                     self.indentLine("%s %s == '%s' " % \
#                                         (cond, 
#                                          discProperty.getRegularName(),
#                                          property.getName()) + \
#                                         "and child.nodeName == '%s':\n" % \
#                                         (refObject.getName()))
        self.generateChildParsingCode(property, choice, cond)
#                     self.printLine('%s = \'%s\'\n' % \
#                                    (choice.getDiscriminator(),
#                                     property.getName()))
                    self.unindent(2)
                    cond = 'elif'

            self.indentLine("elif child.nodeType == child.TEXT_NODE and child.nodeValue.strip() == '':\n")
            self.indentLine('pass\n') # ('node.removeChild(child)\n')
            self.unindentLine('elif child.nodeType != child.TEXT_NODE:\n')
      self.indentLine('print \'*** ERROR *** nodeName = %s\' % ' +
          'child.nodeName\n')
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
  self.printLine('def toXML(self, %s, doc, node):\n' % \
                           object.getRegularName())

#         self.indentLine('if not %s.has_changes():\n' % object.getRegularName())
#         self.indentLine('return\n')
        self.indentLine('if node is None:\n')
   self.indentLine('node = doc.createElement(\'%s\')\n' % \
       object.getName())
        
        self.unindent()
  if len(attrs) > 0:
      self.printLine('\n')
      self.printLine('# set attributes\n')
      for property in attrs:
    self.printLine("node.setAttribute('%s'," % property.getName() +
             "self.convertToStr(%s.%s, '%s'))\n" % \
             (object.getRegularName(),
        property.getFieldName(),
        property.getPythonType()))

        self.printLine('\n')

        if len(elements) + len(choices) > 0:
            self.printLine('# load DOM node map\n')
            self.printLine('nodeMap = {}\n')
            self.printLine('for childNode in node.childNodes:\n')
            self.indentLine("if childNode.nodeType == childNode.ELEMENT_NODE and self.hasAttribute(childNode, 'id'):\n")
            self.indentLine("nodeId = self.convertFromStr(" +
                            "self.getAttribute(childNode, 'id'), 'long')\n")
            self.printLine('nodeMap[(childNode.nodeName, nodeId)] = childNode\n')
            self.unindent()
            self.unindentLine('\n')

      self.printLine('# set elements\n')
      for property in elements:
    if property.isReference():
        self.printLine('%s = %s.%s\n' % \
           (property.getRegularName(),
                                    object.getRegularName(), 
            property.getFieldName()))
        if property.isPlural():
      if property.getPythonType() == 'hash':
          self.printLine('for %s in %s.itervalues():\n' % \
             (property.getSingleName(),
              property.getRegularName()))
      else:
          self.printLine('for %s in %s:\n' % \
             (property.getSingleName(),
              property.getRegularName()))
                    else:
                        self.printLine('if %s is not None:\n' % \
                                           property.getSingleName())
                    self.indentLine("if nodeMap.has_key(('%s', %s.db_id)):\n" %\
                                        (property.getName(), 
                                         property.getSingleName()))
                    self.indentLine("childNode = nodeMap[('%s', %s.db_id)]\n" %\
                                        (property.getName(),
                                         property.getSingleName()))
                    self.printLine("del nodeMap[('%s', %s.db_id)]\n" % \
                                       (property.getName(),
                                        property.getSingleName()))
                    self.unindentLine('else:\n')
                    self.indentLine("childNode = doc.createElement('%s')\n" % \
                                        property.getSingleName())
                    self.printLine('node.appendChild(childNode)\n')
                    self.unindentLine("self.getDao('%s')" % \
                                          property.getReference() +
                                      ".toXML(%s, doc, childNode)\n" % \
                                          property.getSingleName())
                    self.unindent()
    else:
                    self.printLine('if %s.%s is not None:\n' % \
                                       (object.getRegularName(),
                                        property.getFieldName()))
                    if property.isPlural():
                        self.indentLine('for child in %s.%s:\n' % \
                                            (object.getRegularName(),
                                             property.getFieldName()))
                        self.indent()
                    else:
                        self.indentLine('child = %s.%s\n' % \
                                            (object.getRegularName(),
                                             property.getFieldName()))        
                    self.printLine("if nodeMap.has_key(('%s',child.db_id)):\n"\
                                       % property.getName())
                    self.indentLine("childNode = nodeMap[('%s',child.db_id)]\n"\
                                        % property.getName())
                    self.printLine("del nodeMap[('%s', child.db_id)]\n" % \
                                       property.getName())
                    self.printLine('textNode = childNode.firstChild\n')
                    self.printLine('textNode.replaceWholeText(' + 
                                   "self.convertToStr(child, '%s'))\n" % \
                                       property.getPythonType())
                    self.unindentLine('else:\n')
                    self.indentLine("childNode = doc.createElement('%s')\n" % \
                                        property.getName())
                    self.printLine('node.appendChild(childNode)\n')
                    self.printLine('textNode = doc.createTextNode(' +
                                      "self.convertToStr(child, '%s'))\n" % \
                                          property.getPythonType())
                    self.printLine('childNode.appendChild(textNode)\n')
                    self.unindent(2)
                    if property.isPlural():
                        self.unindent()
      for choice in choices:
    self.printLine('%s = %s.%s\n' % \
             (choice.getRegularName(),
                                object.getRegularName(),
        choice.getFieldName()))
    if choice.isPlural():
        if choice.getPythonType() == 'hash':
      self.printLine('for %s in %s.itervalues():\n' % \
               (choice.getSingleName(),
          choice.getRegularName()))
        else:
      self.printLine('for %s in %s:\n' % \
               (choice.getSingleName(),
          choice.getRegularName()))
        self.indent()
    cond = 'if'
    for property in self.getXMLPropertiesForChoice(choice):
#                     self.printLine('%s %s.%s == \'%s\':\n' % \
#                                    (cond,
#                                     object.getRegularName(),
#                                     self.getDiscriminatorProperty( \
#                         object, choice.getDiscriminator()).getFieldName(),
#                                     property.getName()))
                    self.printLine("%s %s.vtType == '%s':\n" % \
                                       (cond,
                                        choice.getSingleName(),
                                        property.getSingleName()))

                    self.indentLine("if nodeMap.has_key(('%s', %s.db_id)):\n"%\
                                        (property.getName(), 
                                         choice.getSingleName()))
                    self.indentLine("childNode = nodeMap[('%s', %s.db_id)]\n" %\
                                        (property.getName(),
                                         choice.getSingleName()))
                    self.printLine("del nodeMap[('%s', %s.db_id)]\n" % \
                                       (property.getName(),
                                        choice.getSingleName()))
                    self.unindentLine('else:\n')
                    self.indentLine("childNode = doc.createElement('%s')\n" % \
                                        property.getSingleName())
                    self.printLine('node.appendChild(childNode)\n')
                    self.unindentLine("self.getDao('%s')" % \
                                          property.getReference() + 
                                      ".toXML(%s, doc, childNode)\n" % \
                                          choice.getSingleName())
                    cond = 'elif'
                    self.unindent()
    if choice.isPlural():
        self.unindent()

      self.printLine('\n')
            self.printLine('# delete nodes not around anymore\n')
            self.printLine('for childNode in nodeMap.itervalues():\n')
            self.indentLine('childNode.parentNode.removeChild(childNode)\n')
            self.unindent()
        self.printLine('return node\n\n')

  self.unindent(2)
  
    def generateChildParsingCode(self, property, field, cond):
  if property.isReference():
      self.indentLine("%s = self.getDao('%s')" % \
          (field.getSingleName(), property.getReference()) +
          '.fromXML(child)\n')

      if field.isPlural():
    if field.getPythonType() == 'hash':
        # get child's primary key and put it into the hash
        childObj = self.getReferencedObject(field.getReference())
        self.printLine("%s[%s.%s] = %s\n" % \
           (field.getRegularName(),
            field.getSingleName(),
            childObj.getKey().getFieldName(),
            field.getSingleName()))
    else:
        self.printLine('%s.append(%s)\n' % \
           (field.getRegularName(), 
            field.getSingleName()))
  else:
      self.indentLine('%s = self.convertFromStr(' % \
                                field.getRegularName() +
                            "child.firstChild.nodeValue,'%s')\n" % \
                                field.getPythonType())
            if field.isPlural():
                # can only be a list
                self.printLine('%s.append(%s)\n' % (field.getRegularName(),
                                                    field.getSingleName()))

