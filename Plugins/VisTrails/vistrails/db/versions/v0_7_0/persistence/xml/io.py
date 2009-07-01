
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

from xml.parsers.expat import ExpatError
import xml.dom.minidom

from db import VistrailsDBException
from db.versions.v0_7_0 import version as my_version
    
def parse_xml_file(filename):
    try:
        return xml.dom.minidom.parse(filename)
    except xml.parsers.expat.ExpatError, e:
        msg = 'XML parse error at line %s, col %s: %s' % \
            (e.lineno, e.offset, e.code)
        raise VistrailsDBException(msg)

def write_xml_file(filename, dom, prettyprint=True):
    output = file(filename, 'w')
    if prettyprint:
        dom.writexml(output, '','  ','\n')
    else:
        dom.writexml(output)
    output.close()

def read_xml_object(vtType, node, dao_list):
    return dao_list[vtType].fromXML(node)

def write_xml_object(obj, dom, dao_list, node=None):
    res_node = dao_list[obj.vtType].toXML(obj, dom, node)
    return res_node

def open_from_xml(filename, vtType, dao_list):
    """open_from_xml(filename) -> DBVistrail"""
    dom = parse_xml_file(filename)
    vistrail = read_xml_object(vtType, dom.documentElement, dao_list)
    dom.unlink()
    return vistrail

def save_to_xml(obj, filename, dao_list):
    dom = xml.dom.minidom.getDOMImplementation().createDocument(None, None,
                                                                None)
    root = write_xml_object(obj, dom, dao_list)
    dom.appendChild(root)
    if obj.vtType == 'vistrail':
        root.setAttribute('version', my_version)
        root.setAttribute('xmlns:xsi', 
                          'http://www.w3.org/2001/XMLSchema-instance')
        root.setAttribute('xsi:schemaLocation', 
                          'http://www.vistrails.org/vistrail.xsd')
    write_xml_file(filename, dom)
    dom.unlink()

def serialize(object, dao_list):
    dom = xml.dom.minidom.getDOMImplementation().createDocument(None, None,
                                                                None)
    root = write_xml_object(object, dom, dao_list)
    dom.appendChild(root)
    return dom.toxml()

def unserialize(str, obj_type):
    dom = xml.dom.minidom.parseString(str)
    return read_xml_object(obj_type, dom.documentElement, dao_list)

