
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

"""Routines common to Linux and OSX."""
import os
import os.path
import popen2
import stat
import subprocess
import sys
import core.utils

def executable_is_in_path(filename):
    """executable_is_in_path(filename): string
    Tests if filename corresponds to an executable file on the path. Returns
the filename if true, or an empty string if false."""
    cmdline = ['which','%s' % filename]
    output = []
    result = execute_cmdline(cmdline, output)
    if result == 256:
        return ""
    if result != 0:
        msg = ("'%s' failed. Return code %s" %
               (cmdline, result))
        raise core.utils.VistrailsInternalError(msg)
    else:
        output = output[0][:-1]
        return output

def executable_is_in_pythonpath(filename):
    """executable_is_in_pythonpath(filename: str)
    Check if exename can be reached in the PYTHONPATH environment. Return
    the filename if true, or an empty string if false.
    
    """
    pathlist = sys.path
    for dir in pathlist:
        fullpath = os.path.join(dir, filename)
        try:
            st = os.stat(fullpath)
        except os.error:
            continue        
        if stat.S_ISREG(st[stat.ST_MODE]):
            return filename
    return ""

def list2cmdline(lst):
    for el in lst:
        assert type(el) == str
    return subprocess.list2cmdline(lst)

def execute_cmdline(lst, output):
    """execute_cmdline(lst: list of str)-> int
    Builds a command line enquoting the arguments properly and executes it
    using Popen4. It returns the error code and the output is on 'output'. 

    """
    cmdline = list2cmdline(lst)
    process = popen2.Popen4(cmdline)
    result = -1
    while result == -1:
        result = process.poll()
    output.extend(process.fromchild.readlines())
    return result
