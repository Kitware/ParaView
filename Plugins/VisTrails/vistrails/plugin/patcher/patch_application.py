
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
# patch gui.application

from PyQt4 import QtGui, QtCore
import gui.application
import plugin.pgui.application
#from plugin.pcore.preferences import Preferences
from core import keychain
from core.utils import InstanceObject
from core.system import systemType
import core.system
import copy
import core.configuration
import CaptureAPI

def ga_stop_application():
    """Stop and finalize the application singleton."""
    app = gui.application.VistrailsApplication
    if app!=None:
        builderWindow = app.builderWindow
        builderWindow.close()
        builderWindow.destroy()
        app.destroy()
        QtCore.QCoreApplication.processEvents()

def ga_start_application_without_init():
    """Initializes the application singleton without package initializing """
    if gui.application.VistrailsApplication==None:
        gui.application.VistrailsApplication = gui.application.VistrailsApplicationSingleton()
    return gui.application.VistrailsApplication()

def VAS_createWindows(self):
    """ createWindows() -> None
    Create and configure all GUI widgets including the builder

    """
    self.setupSplashScreen()
    metalstyle = self.configuration.check('useMacBrushedMetalStyle')
    if metalstyle:
        #to make all widgets to have the mac's nice looking
        self.installEventFilter(self)

    # This is so that we don't import too many things before we
    # have to. Otherwise, requirements are checked too late.
    from plugin.pgui.builder_window import QPluginBuilderWindow
    self.builderWindow = QPluginBuilderWindow()
    self.builderWindow.show()
    self.visDiffParent = QtGui.QWidget(None, QtCore.Qt.ToolTip)
    self.visDiffParent.resize(0,0)

def VAS_init(self, optionsDict=None):
    """ VistrailsApplicationSingleton(optionDict: dict)
                                      -> VistrailsApplicationSingleton
    Create the application with a dict of settings

    """
    if self._initialized: return
    gui.theme.initializeCurrentTheme()
    self.connect(self, QtCore.SIGNAL("aboutToQuit()"), self.finishSession)
    self.configuration = core.configuration.default()
    core.interpreter.default.connect_to_configuration(self.configuration)

    self.keyChain = keychain.KeyChain()

    # Setup configuration to default or saved preferences
    self.vistrailsStartup = plugin.pgui.application.PluginVistrailsStartup(self.configuration)
    self.temp_configuration = copy.copy(self.configuration)
    self.temp_configuration.showSplash = False
    self.temp_configuration.autosave = True
    #self.temp_configuration.fileDirectory = Preferences.get('VisTrailsFileDirectory')
    self.temp_configuration.fileDirectory = CaptureAPI.getPreference('VisTrailsFileDirectory')
    core.system.set_vistrails_file_directory(self.temp_configuration.fileDirectory)

    # Command line options override configuration
    self.input = None

    self.temp_options = InstanceObject(host=None,
                                       port=None,
                                       db=None,
                                       user=None,
                                       vt_id=None,
                                       parameters=None
                                       )
    self.temp_db_options = InstanceObject(host=None,
                                          port=None,
                                          db=None,
                                          user=None,
                                          parameters=None
                                          )
    
    if optionsDict:
        for (k, v) in optionsDict.iteritems():
            setattr(self.configuration, k, v)

    interactive = self.configuration.check('interactiveMode')
    if interactive:
        if systemType!='Darwin':
            self.setIcon()
        self.createWindows()

    self.vistrailsStartup.init()
    # ugly workaround for configuration initialization order issue
    # If we go through the configuration too late,
    # The window does not get maximized. If we do it too early,
    # there are no created windows during spreadsheet initialization.
    if interactive:
        if self.temp_configuration.check('maximizeWindows'):
            self.builderWindow.showMaximized()
        if self.temp_configuration.check('dbDefault'):
            self.builderWindow.setDBDefault(True)
    self.runInitialization()
    self._python_environment = self.vistrailsStartup.get_python_environment()
    self._initialized = True

    if interactive:
        self.interactiveMode()
    else:
        return self.noninteractiveMode()
    return True

def VAS_destroy(self):
    if hasattr(self, 'vistrailsStartup'):
        self.vistrailsStartup.destroy()
    self._initialized = False
    self.disconnect(self, QtCore.SIGNAL("aboutToQuit()"), self.finishSession)

gui.application.PluginVistrailsStartup = plugin.pgui.application.PluginVistrailsStartup
gui.application.VistrailsApplicationSingleton.createWindows = VAS_createWindows
gui.application.VistrailsApplicationSingleton.init = VAS_init
gui.application.VistrailsApplicationSingleton.destroy = VAS_destroy
delattr(gui.application.VistrailsApplicationSingleton, '__del__')
gui.application.stop_application = ga_stop_application
gui.application.start_application_without_init = ga_start_application_without_init
gui.application.pumpedThread = None
