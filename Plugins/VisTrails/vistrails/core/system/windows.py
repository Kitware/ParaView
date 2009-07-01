
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
import os
import shutil
import sys
import stat
import subprocess
import core.system
import popen2

try:
    from ctypes import windll, Structure, c_ulong, byref, sizeof
    importSuccess = True
    
    class WIN32MEMORYSTATUS(Structure):
        """ Structure that represents memory information returned by 
        Windows API
        
        """
        _fields_ = [
            ('dwLength', c_ulong),
            ('dwMemoryLoad', c_ulong),
            ('dwTotalPhys', c_ulong),
            ('dwAvailPhys', c_ulong),
            ('dwTotalPageFile', c_ulong),
            ('dwAvailPageFile', c_ulong),
            ('dwTotalVirtual', c_ulong),
            ('dwAvailVirtual', c_ulong)
            ]

except ImportError:
    importSuccess = False
    
##############################################################################
def parse_meminfo():
    """ 
    parse_meminfo() -> int
    Calls Windows 32 API GlobalMemoryStatus(Ex) to get memory information 
    It requires ctypes module
    
    """ 
    try:
        kernel32 = windll.kernel32

        result = WIN32MEMORYSTATUS()
        result.dwLength = sizeof(WIN32MEMORYSTATUS)
        kernel32.GlobalMemoryStatus(byref(result))
    except:
        return -1
    return result.dwTotalPhys

def guess_total_memory():
    """ guess_total_memory() -> int 
    Return system memory in bytes. If ctypes is not installed it returns -1 
    
    """
    if importSuccess:
        return parse_meminfo()
    else:
        return -1

def temporary_directory():
    """ temporary_directory() -> str 
    Returns the path to the system's temporary directory. Tries to use the $TMP 
    environment variable, if it is present. Else, tries $TEMP, else uses 'c:/' 
    
    """
    if os.environ.has_key('TMP'):
        return os.environ['TMP'] + '\\'
    elif os.environ.has_key('TEMP'):
        return os.environ['TEMP'] + '\\'
    else:
        return 'c:/'

def home_directory():
    """ home_directory() -> str 
    Returns user's home directory using windows environment variables
    $HOMEDRIVE and $HOMEPATH
    
    """
    if len(os.environ['HOMEPATH']) == 0:
  return '\\'
    else:
  return os.environ['HOMEDRIVE'] + os.environ['HOMEPATH']

def remote_copy_program():
    return "pscp -P"

def remote_shell_program():
    return "plink -P"

def graph_viz_dot_command_line():
    """ graph_viz_dot_command_line() -> str
    Returns dot command line

    """
    return 'dot -Tplain -o'

def remove_graph_viz_temporaries():
    pass

def link_or_copy(src, dst):
    """link_or_copy(src:str, dst:str) -> None 
    Copies file src to dst 
    
    """
    shutil.copyfile(src, dst)

def executable_is_in_path(filename):
    """ executable_is_in_path(filename: str) -> string    
    Check if exename can be reached in the PATH environment. Return
    the filename if true, or an empty string if false.
    
    """
    pathlist = (os.environ['PATH'].split(os.pathsep) +
                [core.system.vistrails_root_directory(),
                 "."])
    for dir in pathlist:
        fullpath = os.path.join(dir, filename)
        try:
            st = os.stat(fullpath)
        except os.error:
            try:
                st = os.stat(fullpath+'.exe')
            except:
                continue        
        if stat.S_ISREG(st[stat.ST_MODE]):
            return filename
    return ""

def executable_is_in_pythonpath(filename):
    """ executable_is_in_pythonpath(filename: str) -> string    
    Check if exename can be reached in the PYTHONPATH environment. Return
    the filename if true, or an empty string if false.
    
    """
    pathlist = sys.path
    for dir in pathlist:
        fullpath = os.path.join(dir, filename)
        try:
            st = os.stat(fullpath)
        except os.error:
            try:
                st = os.stat(fullpath+'.exe')
            except:
                continue        
        if stat.S_ISREG(st[stat.ST_MODE]):
            return filename
    return ""

def list2cmdline(lst):
    for el in lst:
        assert type(el) == str
    return '"%s"' % subprocess.list2cmdline(lst)

def execute_cmdline(lst, output):
    """execute_cmdline(lst: list of str)-> int Builds a command line
    enquoting the arguments properly and executes it using popen4. It
    returns the output on output. popen4 doesn't return a code, so it
    will always return -1

    """
    cmdline = list2cmdline(lst)
    out, inp = popen2.popen4(cmdline)
    output.extend(out.readlines())
    return -1
    
################################################################################

import unittest

class TestWindows(unittest.TestCase):
     """ Class to test Windows specific functions """
     
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
         result = executable_is_in_path('cmd')
         assert result != ""

if __name__ == '__main__':
    unittest.main()
