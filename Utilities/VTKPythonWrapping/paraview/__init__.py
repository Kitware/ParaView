#==============================================================================
#
#  Program:   ParaView
#  Module:    __init__.py
#
#  Copyright (c) Kitware, Inc.
#  All rights reserved.
#  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
#
#     This software is distributed WITHOUT ANY WARRANTY; without even
#     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#     PURPOSE.  See the above copyright notice for more information.
#
#==============================================================================
r"""
This module is not meant to be used directly. Please look at one of the modules
it provides:
  servermanager
  vtk
  numeric
  util
  simple
"""

class compatibility:
    minor = None 
    major = None
    
#    @classmethod
    def GetVersion(cls):
        if compatibility.minor and compatibility.major:
            return compatibility.major + float(compatibility.minor)/10
        return None
    GetVersion = classmethod(GetVersion)

def make_name_valid(name):
    """"Make a string into a valid Python variable name.  Return None if
    the name contains parentheses."""
    if not name or '(' in name or ')' in name:
        return None
    import string
    valid_chars = "_%s%s" % (string.ascii_letters, string.digits)
    name = str().join([c for c in name if c in valid_chars])
    if not name[0].isalpha():
        name = 'a' + name
    return name

>>>>>>> 412ce2c... ENH: Add new python module smstate.  This module uses the smtrace module to trace all the currently registered proxies, effectively saving state as a python script.  The new feature is found in the Trace tab in the python shell.  Click 'Trace State' to grab the state, then click 'Save Trace' to save it to disk.  ENH: smtrace and smstate now work with piecewise functions, this allows trace/state to correctly capture display properties for volume renderings.  ENH: smtrace knows about chart views and representations, still some bugs that must be hand corrected in the generated python.:Utilities/VTKPythonWrapping/paraview/__init__.py
        
