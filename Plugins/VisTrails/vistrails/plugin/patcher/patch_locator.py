
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
# patch core.db.locator and db.services.locator
import core.db.locator
from core.db.locator import FileLocator
import db.services.locator
from db.services import io
import core.system
import os.path
import urllib

def patch_untitled_locator():
    fullname = core.system.temporary_save_file('untitled' + core.system.vistrails_default_file_type())
    return FileLocator(fullname)

def patch_encode_name(self, filename):
    name = urllib.quote_plus(filename)
    return core.system.temporary_save_file(name)

def patch_next_temporary(self, temporary):
    if temporary == None:
        return self.encode_name(self._name)
    else:
        return temporary

def patch_iter_temporaries(self, f):
    fname = core.system.temporary_save_file(self.encode_name(self._name))
    if os.path.isfile(fname):
        f(fname)

def patch_save_temporary(self, obj):
    fname = self._find_latest_temporary()
    new_temp_fname = self._next_temporary(fname)
    io.save_to_zip_xml(obj, new_temp_fname)
    
def patch_load(self, type):
    fname = self._find_latest_temporary()
    if fname:
        obj = io.open_from_zip_xml(fname, type)
    else:
        obj = io.open_from_zip_xml(self._name, type)
    obj.locator = self
    return obj

core.db.locator.untitled_locator = patch_untitled_locator
db.services.locator.XMLFileLocator.encode_name = patch_encode_name
db.services.locator.XMLFileLocator._next_temporary = patch_next_temporary
db.services.locator.XMLFileLocator._iter_temporaries = patch_iter_temporaries
db.services.locator.ZIPFileLocator.save_temporary = patch_save_temporary
db.services.locator.ZIPFileLocator.load = patch_load

