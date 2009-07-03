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
    "Make a string into a valid Python variable name"
    if not name:
        return None
    if name.find('(') >= 0 or name.find(')') >=0:
        return None
    name = name.replace(' ','')
    name = name.replace('-','')
    name = name.replace(':','')
    name = name.replace('.','')
    if not name[0].isalpha():
        name = 'a' + name
    return name
        
