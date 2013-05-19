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

class compatibility:
    """Class used to check version number and compatibility"""
    minor = None
    major = None

    def GetVersion(cls):
        if compatibility.minor and compatibility.major:
            version = float(compatibility.minor)
            while version > 1.0:
                version = version / 10.0
            version += float(compatibility.major)
            return version
        return None
    GetVersion = classmethod(GetVersion)

def make_name_valid(name):
    """Make a string into a valid Python variable name."""
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

def print_warning(text):
   """Print text"""
   print text

def print_error(text):
   """Print text"""
   print text

def print_debug_info(text):
   """Print text"""
   print text

"""This variable is set whenever Python is initialized within a ParaView
Qt-based application. Modules within the 'paraview' package often use this to
taylor their behaviour based on whether the Python environment is embedded
within an application or not."""
fromGUI = False
