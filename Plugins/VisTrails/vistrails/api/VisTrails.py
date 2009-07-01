
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
## Copyright (C) 2008, 2009 VisTrails, Inc. All rights reserved.
##
############################################################################

import sys
import os.path

# The vistrails directory must be in the python path.
# We know where it is since this file is in a known spot within that directory.
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(sys.modules[__name__].__file__))))


# Import the plugin's CaptureAPI, and make it available in the
# root of the module heirarchy.  This is done to match they way
# that the Maya plugin makes its capture api available.
# We have to do this even before the patcher gets imported!!
from plugin.app import CaptureAPI
sys.modules['CaptureAPI'] = CaptureAPI


# do this early so we always use the patched modules
import plugin.patcher

import gui.application
from gui.utils import getBuilderWindow


# start vistrails
app = gui.application.start_application_without_init()


builderWindow = getBuilderWindow()

if not builderWindow:
    print "no builder window???"

else:

    # startup whatever the capture api needs to run
    CaptureAPI.start(builderWindow)
    

# Let QT take over.
app.exec_()
