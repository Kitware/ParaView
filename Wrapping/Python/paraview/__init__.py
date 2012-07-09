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
  pvfilters
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
    """"Make a string into a valid Python variable name."""
    if not name:
        return None
    import string
    valid_chars = "_%s%s" % (string.ascii_letters, string.digits)
    name = str().join([c for c in name if c in valid_chars])
    if not name[0].isalpha():
        name = 'a' + name
    return name

class options:
    """Values set here have any effect, only when importing the paraview module
       in python interpretor directly i.e. not through pvpython or pvbatch. In
       that case, one should use command line arguments for the two
       executables"""

    """When True, act as pvbatch. Default behaviour is to act like pvpython"""
    batch = False

    """When True, acts like pvbatch --symmetric. Requires that batch is set to
    True to have any effect."""
    symmetric = False


"""Set by vvtkPythonProgrammableFilter"""
fromFilter = False
