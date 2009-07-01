
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
import core.system
import os
import os.path

def patch_temporary_save_directory():
    """ temporary_save_directory() -> str 
    Returns the path to a unique save directory that is located in the
    system's temporary directory.

    """
    # not unique yet
    saveDir = os.path.join(core.system.temporary_directory(), 'vt_saves')
    if not os.path.exists(saveDir):
        os.mkdir(saveDir)
    return saveDir

def patch_temporary_save_file(filename):
    """ 

    """
    file = os.path.join(core.system.temporary_save_directory(), filename)

    path,ext = os.path.splitext(file)

    num=0
    while True:
        if num==0:
            newName = file
        else:
            newName = path+str(num)+ext

        if not os.path.exists(newName):
            return newName

        num+=1


def patch_clean_temporary_save_directory():
    """
    Clean the contents of the vt_saves temporary directory.
    """
    for f in os.listdir(patch_temporary_save_directory()):
        os.unlink(os.path.join(patch_temporary_save_directory(), f))
        

core.system.temporary_save_directory = patch_temporary_save_directory
core.system.temporary_save_file = patch_temporary_save_file
core.system.clean_temporary_save_directory = patch_clean_temporary_save_directory
