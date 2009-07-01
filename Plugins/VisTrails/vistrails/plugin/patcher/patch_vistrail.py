
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
## Copyright (C) 2008, 2009 VisTrails, Inc. All rights reserved.
##
############################################################################

##############################################################################
import array
from core.vistrail.action import Action

def V_get_description(self, version_number):
    """ get_description(version_number: int) -> string
    
    """
    if version_number in self.actionMap:
        action = self.actionMap[version_number]
        if action.description is not None:
            return action.description
    return ""

def V_get_snapshot(self, version_number):
    """ get_snapshot(version_number: int) -> bool
    
    """
    if version_number in self.actionMap:
        action = self.actionMap[version_number]
        if action.snapshot is not None:
            return action.snapshot
    return ""

def V_change_snapshot(self, value, version_number):
    """ change_snapshot(value: int, version_number: int) -> bool
    
    """
    return self.change_annotation(Action.ANNOTATION_SNAPSHOT,
                                  str(value), version_number)

def V_init(self, locator=None):
    original_V_init(self, locator)
    self.binary_data = array.array('c')
    self.saved_files = []

def V_store_string(self, string):
    """ Append data to binary_data and return their indices """
    start = len(self.binary_data)
    self.binary_data.fromstring(string)
    end = len(self.binary_data)
    return (start, end)

def V_get_string(self, indices):
    """ Return the string given the indices (start,end) without any range check """
    return self.binary_data[indices[0]:indices[1]].tostring()

def V_get_next_snapshot_filename(self, basename, ext):
    """ V_get_next_snapshot_filename(basename: string, ext: string) -> string
    Get the next available snapshot filename with the given extension

    """
    import os.path
    path = core.system.temporary_save_directory()
    num=0
    while os.path.exists(os.path.join(path,basename+str(num)+ext)):
        num+=1
    return basename+str(num)+ext

def V_add_saved_file(self, filename, bin):
    """ V_add_saved_file(filename:string, bin:bool) -> string 
    Store and return a temporary filename 

    """
    fullPath = core.system.temporary_save_file(filename)
    self.saved_files.append((fullPath,bin))
    return fullPath

def V_clean_saved_files(self):
    """ V_clean_saved_files() -> None
    Remove any temporary save files

    """
    import os, os.path
    for (filename,bin) in self.saved_files:
        if os.path.exists(filename):
            os.remove(filename)
    self.saved_files = []


import core.vistrail
original_V_init = core.vistrail.vistrail.Vistrail.__init__
core.vistrail.vistrail.Vistrail.__init__ = V_init
core.vistrail.vistrail.Vistrail.store_string = V_store_string
core.vistrail.vistrail.Vistrail.get_string = V_get_string
core.vistrail.vistrail.Vistrail.get_description = V_get_description
core.vistrail.vistrail.Vistrail.get_snapshot = V_get_snapshot
core.vistrail.vistrail.Vistrail.change_snapshot = V_change_snapshot
core.vistrail.vistrail.Vistrail.add_saved_file = V_add_saved_file
core.vistrail.vistrail.Vistrail.clean_saved_files = V_clean_saved_files
core.vistrail.vistrail.Vistrail.get_next_snapshot_filename = V_get_next_snapshot_filename
