
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
from xml.dom import minidom
import xml.parsers.expat
import __builtin__

def eval_xml_value(node):
    """eval_xml_value(node) -> value

       evaluates an xml node as the following examples:

       <str value='foo'/> -> 'foo'
       <int value='3'/> -> 3
       <float value='3.141592'> -> 3.141592
       <bool value='False'> -> False
    """
    
    key_name = node.nodeName
    type_ = getattr(__builtin__, key_name)
    str_value = str(node.attributes['value'].value)

    # Tricky case bool('False') == True
    if type_ == bool:
        if str_value == 'True':
            return True
        elif str_value == 'False':
            return False
        else:
            raise Exception("eval_xml_value: Bogus bool value '%s'" % str_value)
    return type_(str_value)

def quote_xml_value(dom, value):
    """quote_xml_value(dom, value) -> value

       quotes a value as an xml node so that
       eval_xml_value(quote_xml_value(dom, value)) == value

       <str value='foo'/> <- 'foo'
       <int value='3'/> <- 3
       <float value='3.141592'> <- 3.141592
       <bool value='False'> <- False
    """

    el = dom.createElement(type(value).__name__)
    el.setAttribute('value', str(value))
    return el

def named_elements(element, elname):
    """named_elements(element, elname) -> Node 
    Helper function that iterates over the element child Nodes searching
    for node with name elname.

    """
    for node in element.childNodes:
        if node.nodeName == elname:
            yield node

def enter_named_element(element, elname):
    """enter_named_element(element, elname) -> Node 
    Returns first child of element with name elname

    """
    for node in named_elements(element, elname):
        return node
    return None

def elements_filter(element, element_predicate):
    """elements_filter(element, element_predicate) -> Node iterator
    Helper function that iterates over the element child Nodes searching
    for nodes that pass element_predicate, that is, node for which

    element_predicate(node) == True

    """
    for node in element.childNodes:
        if element_predicate(node):
            yield node

class XMLWrapper(object):
    """Helper to parse a general XML file. It provides functions to open and 
    close files.
    It must be subclassed to parse specifi files. """

    class XMLParseError(Exception):

       
        def __init__(self, line, char, code):
            self._line = line
            self._char = char
            self._code = code
            
        def __str__(self):
            return ("XML Parse error at line %s, col %s: %s" %
                    (self._line,
                     self._char,
                     xml.parsers.expat.ErrorString(self._code)))

    def open_file(self, filename):
        """open_file(filename: str) -> None 
        Parses an XML file.

        """
        self.filename = filename
        try:
            self.dom = minidom.parse(filename)
        except xml.parsers.expat.ExpatError, e:
            raise self.XMLParseError(e.lineno, e.offset, e.code) 

    def create_document_from_string(self, text):
        """parse_string(text:str) -> dom
        Parses an xml string and returns the DOM object

        """
        try:
            dom = minidom.parseString(text)
        except xml.parsers.expat.ExpatError, e:
            raise self.XMLParseError(e.lineno, e.offset, e.code)
        return dom
    
    def close_file(self):
        """close_file() -> None 
        Removes the association with the XML file loaded by open_file 
        method. 

        """
        if self.dom:
            self.dom.unlink()
        self.filename = None
        self.dom = None
    
    def create_document(self, nodename):
        """create_document(nodename: str) -> xml element 
        Creates a documentElement 
        
        """
        impl = minidom.getDOMImplementation()
        dom = impl.createDocument(None, nodename, None)
        return dom

    def write_document(self, root, filename):
        """write_document(root:xml element, filename: str) -> None
        Save as an XML file 
        
        """
        output_file = file(filename,'w')
        root.writexml(output_file, "  ", "  ", '\n')
        output_file.close()

    def __str__(self):
        """ __str__() -> str 
        Returns the XML that self.dom represents as a string 
        
        """
        return self.dom.toprettyxml()

################################################################################
# Testing

import unittest

class TestXmlUtils(unittest.TestCase):
    def test_named_elements(self):
        """ Exercises searching for elements """
        xmlStr = """<root> 
                        <child>
                            <grandchild></grandchild>
                            <grandchild></grandchild>
                         </child>
                         <child></child>
                     </root>"""
        dom = minidom.parseString(xmlStr)
        root = dom.documentElement
        childcount = 0
        for node in named_elements(root,'child'):
            childcount += 1
        self.assertEquals(childcount,2)
        
        grandchildcount = 0
        for node in named_elements(root,'grandchild'):
            grandchildcount += 1
        self.assertEquals(grandchildcount,0)

    def test_eval_quote(self):
        xmlStr = """<root> 
                        <child>
                            <grandchild></grandchild>
                            <grandchild></grandchild>
                         </child>
                         <child></child>
                     </root>"""
        dom = minidom.parseString(xmlStr)
        def do_it_1(v):
            q = quote_xml_value(dom, v)
            v2 = eval_xml_value(q)
            self.assertEquals(v, v2)
        def do_it_2(q):
            q = minidom.parseString(q).documentElement
            v = eval_xml_value(q)
            self.assertEquals(q.toxml(), quote_xml_value(dom, v).toxml())
        do_it_1(2)
        do_it_1(3.0)
        do_it_1(False)
        do_it_1(True)
        do_it_1('Foobar')
        do_it_1('with<brackets>')

        do_it_2('<str value="Foo"/>')
        do_it_2('<bool value="False"/>')
        do_it_2('<bool value="True"/>')
        do_it_2('<int value="3"/>')
        do_it_2('<float value="4.0"/>')
        
if __name__ == "__main__":
    unittest.main()
