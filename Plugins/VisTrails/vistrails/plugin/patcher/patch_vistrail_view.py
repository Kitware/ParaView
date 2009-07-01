
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
# Patch vistrail view

import gui.vistrail_view

def QVV_init(self, *args, **kwargs):
    original_init(self, *args, **kwargs)
    self.setPropertiesMode(False)
    self.setPropertiesOverlayMode(True)

def QVV_setup_view(self, *args, **kwargs):
    original_setup_view(self, *args, **kwargs)
    self.setPIPMode(False)

def CPP_versionSelected(self, versionId, byClick):
    original_versionSelected(self,versionId,byClick)
    import CaptureAPI
    CaptureAPI.afterVersionSelectedByUser()


original_init = gui.vistrail_view.QVistrailView.__init__
gui.vistrail_view.QVistrailView.__init__ = QVV_init

original_setup_view = gui.vistrail_view.QVistrailView.setup_view
gui.vistrail_view.QVistrailView.setup_view = QVV_setup_view

original_versionSelected=gui.vistrail_view.QVistrailView.versionSelected
gui.vistrail_view.QVistrailView.versionSelected=CPP_versionSelected