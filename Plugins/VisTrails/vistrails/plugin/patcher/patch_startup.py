
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
# patch core.startup
import core.startup

def patch_init(self):
    self.setupDefaultFolders()
    self.setupBaseModules()
    self.runStartupHooks()

def patch_runDotVistrails(self):
    return {'configuration': self.configuration}

def patch_load_configuration(self):
    pass

core.startup.VistrailsStartup.init = patch_init
core.startup.VistrailsStartup.runDotVistrails = patch_runDotVistrails
core.startup.VistrailsStartup.load_configuration = patch_load_configuration
