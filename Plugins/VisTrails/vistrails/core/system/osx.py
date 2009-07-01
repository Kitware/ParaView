
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
""" Mac OS X specific file """
      
# from xml import dom
# from xml.dom.xmlbuilder import DOMInputSource, DOMBuilder

import xml.etree.cElementTree as ElementTree
import datetime
import os
import shutil
import time
from core.system.unix import executable_is_in_path, list2cmdline, \
     executable_is_in_pythonpath, execute_cmdline
import core.utils
    
###############################################################################
# Extract system detailed information of a Mac system
#
# Based on a Python Cookbook recipe available online :
#    http://aspn.activestate.com/ASPN/Cookbook/Python/Recipe/303063
# and in the Python Coobook, page 418
# Credit: Brian Quinlan
#
# Rewritten to use ElementTree instead of PyXML
#
# This recipe uses the system_profiler application to retrieve detailed
# information about a Mac OS X system. There are two useful ways to use it:
# the first is to ask for a complete Python datastructure containing
# information about the system (see OSXSystemProfiler.all()) and the 
# other is two ask for particular keys in the system information database.

def group(lst, n):
    """group([0,3,4,10,2,3], 2) => [(0,3), (4,10), (2,3)]
    
    Group a list into consecutive n-tuples. Incomplete tuples are
    discarded e.g.
    
    >>> group(range(10), 3)
    [(0, 1, 2), (3, 4, 5), (6, 7, 8)]
    
    """
    return zip(*[lst[i::n] for i in xrange(n)]) 

class OSXSystemProfiler(object):
    "Provide information from the Mac OS X System Profiler"

    def __init__(self, detail=-1):
        """detail can range from -2 to +1, with larger numbers returning more
        information. Beware of +1, it can take several minutes for
        system_profiler to generate the data."""

        command = 'system_profiler -xml -detailLevel %d' % detail
        self.document = ElementTree.parse(os.popen(command))

    def _content(self, node):
        "Get the text node content of an element or an empty string"
        return (node.text or '')

    def _convert_value_node(self, node):
        """Convert a 'value' node (i.e. anything but 'key') into a Python data
        structure"""
        if node.tag == 'string':
            return self._content(node)
        elif node.tag == 'integer':
            return int(self._content(node))
        elif node.tag == 'real':
            return float(self._content(node))
        elif node.tag == 'date': #  <date>2004-07-05T13:29:29Z</date>
            return datetime.datetime(
                *time.strptime(self._content(node), '%Y-%m-%dT%H:%M:%SZ')[:5])
        elif node.tag == 'array':
            return [self._convert_value_node(n) for n in node.getchildren()]
        elif node.tag == 'dict':
            return dict([(self._content(n), self._convert_value_node(m))
                for n, m in group(node.getchildren(), 2)])
        else:
            raise ValueError(node.tag)
    
    def __getitem__(self, key):
        nodes = self.document.findall('//dict')
        results = []
        for node in nodes:
            for child in node:
                if child.tag == 'key' and child.text == key:
                    v = self._convert_value_node(node)[key]
                    if isinstance(v, dict) and v.has_key('_order'):
                        # this is just information for display
                        pass
                    else:
                        results.append(v)
        return results
    
#     def all(self):
#         """Return the complete information from the system profiler
#         as a Python data structure"""
        
#         return self._convert_value_node(
#             self.document.documentElement.firstChild)

###############################################################################

def example():
    from optparse import OptionParser
    from pprint import pprint

    info = OSXSystemProfiler()
    parser = OptionParser()
    parser.add_option("-f", "--field", action="store", dest="field",
                      help="display the value of the specified field")
    
    (options, args) = parser.parse_args()
    if len(args) != 0:
        parser.error("no arguments are allowed")
    
    if options.field is not None:
        pprint(info[options.field])
    else:
        # just print some comment keys known to exist in only one important
        # dictionary
        for k in ['cpu_type', 'current_processor_speed', 'l2_cache_size',
                  'physical_memory', 'user_name', 'os_version', 'ip_address']:
            print '%s: %s' % (k, info[k][0])

###############################################################################

def parse_meminfo():
    """ parse_meminfo() -> int
    Uses the system_profiler application to retrieve detailed information
    about a Mac OS X system.

    """
#     try:
#         from xml import dom, xpath
     
#     except ImportError:
#         print '**** Install PyXML to get the max memory information\n ****'
#         return -1
        
    result = -1
    info = OSXSystemProfiler()
    mem = info['physical_memory'][0]
    if mem.upper().endswith(' GB'):
        result = int(mem[:-3]) * 1024 * 1024 * 1024L
    elif mem.upper().endswidth(' MB'):
        result = int(mem[:-3]) * 1024 * 1024L
    return result

def guess_total_memory():
    """ guess_total_memory() -> int 
    Return system memory in bytes. If PyXML is not installed it returns -1 
    
    """
    return parse_meminfo()

def temporary_directory():
    """ temporary_directory() -> str 
    Returns the path to the system's temporary directory 
    
    """
    return "/tmp/"

def home_directory():
    """ home_directory() -> str 
    Returns user's home directory using environment variable $HOME
    
    """
    return os.getenv('HOME')

def remote_copy_program():
    return "scp -p"

def remote_shell_program():
    return "ssh -p"

def graph_viz_dot_command_line():
    """ graph_viz_dot_command_line() -> str
    Returns dot command line

    """
    return 'dot -Tplain -o '

def remove_graph_viz_temporaries():
    """ remove_graph_viz_temporaries() -> None 
    Removes temporary files generated by dot 
    
    """
    os.unlink(temporary_directory() + "dot_output_vistrails.txt")
    os.unlink(temporary_directory() + "dot_tmp_vistrails.txt")

def link_or_copy(src, dst):
    """link_or_copy(src:str, dst:str) -> None 
    Tries to create a hard link to a file. If it is not possible, it will
    copy file src to dst 
    
    """
    # Links if possible, but we're across devices, we need to copy.
    try:
        os.link(src, dst)
    except OSError, e:
        if e.errno == 18:
            # Across-device linking is not possible. Let's copy.
            shutil.copyfile(src, dst)
        else:
            raise e

################################################################################

import unittest

class TestMacOSX(unittest.TestCase):
     """ Class to test Mac OS X specific functions """
     
     def test1(self):
         """ Test if guess_total_memory() is returning an int >= 0"""
         result = guess_total_memory()
         assert type(result) == type(1) or type(result) == type(1L)
         assert result >= 0

     def test2(self):
         """ Test if home_directory is not empty """
         result = home_directory()
         assert result != ""

     def test3(self):
         """ Test if temporary_directory is not empty """
         result = temporary_directory()
         assert result != ""

     def test_executable_file_in_path(self):
         # Should exist in any POSIX shell, which is what we have in OSX
         result = executable_is_in_path('ls')
         assert result == "/bin/ls" # Any UNIX should respect this.

if __name__ == '__main__':
    unittest.main()
             
