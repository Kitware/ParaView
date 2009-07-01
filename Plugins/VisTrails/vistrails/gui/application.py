
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
##
############################################################################
""" This is the main application of vistrail, it will calls for
initializations to the theme, packages and the builder...

"""


from PyQt4 import QtGui, QtCore
from core import command_line
from core import debug
from core import system
from core import keychain
from core.db.locator import FileLocator, DBLocator
from core.modules.module_registry import registry
from core.utils import InstanceObject
from core.utils.uxml import (named_elements,
                             elements_filter, enter_named_element)
from gui import qt
import core.configuration
import core.interpreter.default
import core.interpreter.cached
import core.requirements
import core.startup
import gui.theme
import os.path
import sys
import copy

################################################################################

class VistrailsApplicationSingleton(QtGui.QApplication):
    """
    VistrailsApplicationSingleton is the singleton of the application,
    there will be only one instance of the application during VisTrails
    
    """
    
    def __call__(self):
        """ __call__() -> VistrailsApplicationSingleton
        Return self for calling method
        
        """
        if not self._initialized:
            self.init()
        return self

    def __init__(self):
        QtGui.QApplication.__init__(self, sys.argv)
        if QtCore.QT_VERSION < 0x40200: # 0x40200 = 4.2.0
            raise core.requirements.MissingRequirement("Qt version >= 4.2")
        self._initialized = False
        qt.allowQObjects()

    def init(self, optionsDict=None):
        """ VistrailsApplicationSingleton(optionDict: dict)
                                          -> VistrailsApplicationSingleton
        Create the application with a dict of settings
        
        """
        gui.theme.initializeCurrentTheme()
        self.connect(self, QtCore.SIGNAL("aboutToQuit()"), self.finishSession)
        self.configuration = core.configuration.default()
        core.interpreter.default.connect_to_configuration(self.configuration)
        
        self.keyChain = keychain.KeyChain()
        self.setupOptions()

        # Setup configuration to default or saved preferences
        self.vistrailsStartup = core.startup.VistrailsStartup(self.configuration)
        self.temp_configuration = copy.copy(self.configuration)

        
        # now we want to open vistrails and point to a specific version
        # we will store the version in temp options as it doesn't
        # need to be persistent. We will do the same to database
        # information passed in the command line
        self.temp_options = InstanceObject(host=None,
                                           port=None,
                                           db=None,
                                           user=None,
                                           vt_id=None,
                                           parameters=None
                                           ) 

        
        # Command line options override configuration
        self.readOptions()
        if optionsDict:
            for (k, v) in optionsDict.iteritems():
                setattr(self.temp_configuration, k, v)

        interactive = self.temp_configuration.check('interactiveMode')
        if interactive:
            self.setIcon()
            self.createWindows()
            self.processEvents()
            
        self.vistrailsStartup.init()
        # ugly workaround for configuration initialization order issue
        # If we go through the configuration too late,
        # The window does not get maximized. If we do it too early,
        # there are no created windows during spreadsheet initialization.
        if interactive:
            if  self.temp_configuration.check('maximizeWindows'):
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

    def get_python_environment(self):
        """get_python_environment(): returns an environment that
includes local definitions from startup.py. Should only be called
after self.init()"""
        return self._python_environment

    def destroy(self):
        """ destroy() -> None
        Finalize all packages to, such as, get rid of temp files
        
        """
        if hasattr(self, 'vistrailsStartup'):
            self.vistrailsStartup.destroy()

    def __del__(self):
        """ __del__() -> None
        Make sure to finalize in the destructor
        
        """
        self.destroy()

    def _parse_vtinfo(self, info, use_filename=True):
        name = None
        version = None
        if use_filename and os.path.isfile(info):
            name = info
        else:
            data = info.split(":")
            if len(data) >= 2:
                if use_filename and os.path.isfile(str(data[0])):
                        name = str(data[0])
                elif not use_filename:
                    name = str(data[0])
                # will try to convert version to int
                # if it fails, it's a tag name
                try:
                    #maybe a tag name contains ':' in ist name
                    #so we need to bring it back together
                    rest = ":".join(data[1:])
                    version = int(rest)
                except ValueError:
                    version = str(rest)
            elif len(data) == 1:
                if use_filename and os.path.isfile(str(data[0])):
                    name = str(data[0])
                elif not use_filename:
                    name = str(data[0])
        return (name, version)
        
    def interactiveMode(self):
        """ interactiveMode() -> None
        Instantiate the GUI for interactive mode
        
        """     
        if self.temp_configuration.check('showSplash'):
            self.splashScreen.finish(self.builderWindow)
        self.builderWindow.create_first_vistrail()
        self.builderWindow.modulePalette.treeWidget.updateFromModuleRegistry()
        self.builderWindow.modulePalette.connect_registry_signals()
        usedb = False
        if self.temp_db_options.host:
           usedb = True
        if self.input:
            #check if versions are embedded in the filename
            for filename in self.input:
                f_name, version = self._parse_vtinfo(filename, not usedb)
                if not usedb:
                    locator = FileLocator(os.path.abspath(f_name))
                    #_vnode and _vtag will be set when a .vtl file is open and
                    # instead of a FileLocator, a DBLocator is created instead
                    if hasattr(locator, '_vnode'):
                        version = locator._vnode
                        print locator._vnode
                    if hasattr(locator,'_vtag'):
                        # if a tag is set, it should be used instead of the
                        # version number
                        if locator._vtag != '':
                            version = locator._vtag
                            print locator._vtag
                else:
                    locator = DBLocator(host=self.temp_db_options.host,
                                        port=self.temp_db_options.port,
                                        database=self.temp_db_options.db,
                                        user='',
                                        passwd='',
                                        obj_id=f_name,
                                        obj_type=None,
                                        connection_id=None)
                execute = self.temp_configuration.executeWorkflows
                self.builderWindow.open_vistrail_without_prompt(locator,
                                                                version,
                                                                execute)

        if not self.temp_configuration.showSpreadsheetOnly:
            # in some systems (Linux and Tiger) we need to make both calls
            # so builderWindow is activated
            self.builderWindow.raise_()
            self.builderWindow.activateWindow()
        else:
            self.builderWindow.hide()

    def noninteractiveMode(self):
        """ noninteractiveMode() -> None
        Run the console in non-interactive mode
        
        """
        usedb = False
        if self.temp_db_options.host:
           usedb = True
        if self.input:
            w_list = []
            for filename in self.input:
                f_name, version = self._parse_vtinfo(filename, not usedb)
                if f_name and version:
                    if not usedb:
                        locator = FileLocator(os.path.abspath(f_name))
                    else:
                        locator = DBLocator(host=self.temp_db_options.host,
                                            port=self.temp_db_options.port,
                                            database=self.temp_db_options.db,
                                            user=self.temp_db_options.user,
                                            passwd='',
                                            obj_id=f_name,
                                            obj_type=None,
                                            connection_id=None)
                    w_list.append((locator, version))
            import core.console_mode
            if self.temp_db_options.parameters == None:
                self.temp_db_options.parameters = ''
            r = core.console_mode.run(w_list,
                                      self.temp_db_options.parameters)
            return r
        else:
            debug.DebugPrint.critical("no input vistrails provided")
            return False

    def setIcon(self):
        """ setIcon() -> None
        Setup Vistrail Icon
        """
        self.setWindowIcon(gui.theme.CurrentTheme.APPLICATION_ICON)
        
    def setupSplashScreen(self):
        """ setupSplashScreen() -> None
        Create the splash-screen at startup
        
        """
        if self.temp_configuration.check('showSplash'):
            splashPath = (system.vistrails_root_directory() +
                          "/gui/resources/images/vistrails_splash.png")
            pixmap = QtGui.QPixmap(splashPath)
            self.splashScreen = QtGui.QSplashScreen(pixmap, QtCore.Qt.WindowStaysOnTopHint)
            self.splashScreen.show()

    def createWindows(self):
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
        from gui.builder_window import QBuilderWindow

        self.builderWindow = QBuilderWindow()
        self.builderWindow.show()
        self.visDiffParent = QtGui.QWidget(None, QtCore.Qt.ToolTip)
        self.visDiffParent.resize(0,0)
        
    def setupOptions(self):
        """ setupOptions() -> None
        Check and store all command-line arguments
        
        """
        add = command_line.CommandLineParser.add_option
        add("-S", "--startup", action="store", type="str", default=None,
            dest="dotVistrails",
            help="Set startup file (default is ~/.vistrails/startup.py)")
        add("-?", action="help",
            help="show this help message and exit")
        add("-v", "--version", action="callback",
            callback=lambda option, opt, value, parser: self.printVersion(),
            help="print version information and quit")
        add("-V", "--verbose", action="store", type="int", default=None,
            dest="verbose", help="set verboseness level (0--2, "
            "default=0, higher means more verbose)")
        add("-n", "--nosplash", action="store_false",
            default = None,
            help="don't display splash on startup")
        add("-c", "--cache", action="store", type="int", default=None,
            dest="cache", help="enable/disable caching")
        add("-m", "--movies", action="store", type="int", default=None,
            dest="movies", help="set automatic movie creation on spreadsheet "
            "(0 or 1, default=1. Set this to zero to work around vtk bug with "
            "offscreen renderer and opengl texture3d mappers)")
        add("-s", "--multiheads", action="store_true",
            default = None,
            help="display the builder and spreadsheet on different screens "
            "(if available)")
        add("-x", "--maximized", action="store_true",
            default = None,
            help="Maximize VisTrails windows at startup")
        add("-b", "--noninteractive", action="store_true",
            default = None,
            help="run in non-interactive mode")
        add("-e", "--dumpcells", action="store", dest="dumpcells",
            default = None,
            help="when running in non-interactive mode, directory to dump "
            "spreadsheet cells before exiting")
        add("-l", "--nologger", action="store_true",
            default = None,
            help="disable the logging")
        add("-d", "--debugsignals", action="store_true",
            default = None,
            help="debug Qt Signals")
        add("-a", "--parameters", action="store", dest="parameters",
            help="workflow parameter settings (non-interactive mode only)")
        add("-t", "--host", action="store", dest="host",
            help="hostname or ip address of database server")
        add("-r", "--port", action="store", type="int", default=3306,
            dest="port", help="database port")
        add("-f", "--db", action="store", dest="db",
            help="database name")
        add("-u", "--user", action="store", dest="user",
            help="database username")
        add("-i", "--showspreadsheetonly", action="store_true",
            default = None,
            help="only the spreadsheet will be shown. This implies -w was given.\
The builder window can be accessed by a spreadsheet menu option.")
        add("-w", "--executeworkflows", action="store_true",
            default = None,
            help="The workflows will be executed")
        command_line.CommandLineParser.parse_options()

    def printVersion(self):
        """ printVersion() -> None
        Print version of Vistrail and exit
        
        """
        print system.about_string()
        sys.exit(0)

    def readOptions(self):
        """ readOptions() -> None
        Read arguments from the command line
        
        """
        get = command_line.CommandLineParser().get_option
        if get('nosplash')!=None:
            self.temp_configuration.showSplash = bool(get('nosplash'))
        if get('debugsignals')!=None:
            self.temp_configuration.debugSignals = bool(get('debugsignals'))
        if get('dotVistrails')!=None:
            self.temp_configuration.dotVistrails = get('dotVistrails')
        if not self.configuration.check('dotVistrails'):
            self.configuration.dotVistrails = system.default_dot_vistrails()
            self.temp_configuration.dotVistrails = system.default_dot_vistrails()
        if get('multiheads')!=None:
            self.temp_configuration.multiHeads = bool(get('multiheads'))
        if get('maximized')!=None:
            self.temp_configuration.maximizeWindows = bool(get('maximized'))
        if get('movies')!=None:
            self.temp_configuration.showMovies = bool(get('movies'))
        if get('cache')!=None:
            self.temp_configuration.useCache = bool(get('cache'))
        if get('verbose')!=None:
            self.temp_configuration.verbosenessLevel = get('verbose')
        if get('noninteractive')!=None:
            self.temp_configuration.interactiveMode = \
                                                  not bool(get('noninteractive'))
            if get('dumpcells') != None:
                self.temp_configuration.spreadsheetDumpCells = get('dumpcells')
        if get('executeworkflows') != None:
            self.temp_configuration.executeWorkflows = \
                                            bool(get('executeworkflows'))
        if get('showspreadsheetonly') != None:
            self.temp_configuration.showSpreadsheetOnly = \
                                            bool(get('showspreadsheetonly'))
            # asking to show only the spreadsheet will force the workflows to
            # be executed
            if self.temp_configuration.showSpreadsheetOnly:
                self.temp_configuration.executeWorkflows = True
            
        self.temp_db_options = InstanceObject(host=get('host'),
                                                 port=get('port'),
                                                 db=get('db'),
                                                 user=get('user'),
                                                 parameters=get('parameters')
                                                 )
        if get('nologger')!=None:
            self.temp_configuration.nologger = bool(get('nologger'))
        self.input = command_line.CommandLineParser().positional_arguments()
        
    def runInitialization(self):
        """ runInitialization() -> None
        Run init script on the user folder
        
        """
        def initBookmarks():
            """loadBookmarkCollection() -> None
            Init BookmarksManager and creates .vistrails folder if it 
            does not exist 

            """
            if (not os.path.isdir(self.configuration.dotVistrails) and 
                not os.path.isfile(self.configuration.dotVistrails)):
                #create .vistrails dir
                os.mkdir(self.configuration.dotVistrails)

            # This is so that we don't import too many things before we
            # have to. Otherwise, requirements are checked too late.
           # import gui.bookmark_window
           # gui.bookmark_window.initBookmarks(system.default_bookmarks_file())    
            
        #initBookmarks()
        self.showSplash = self.configuration.showSplash

    def finishSession(self):
        core.interpreter.cached.CachedInterpreter.cleanup()
   
    def eventFilter(self, o, event):
        """eventFilter(obj,event)-> boolean
        This will filter all create events and will set on the WA_MacMetalStyle
        attribute of a QWidget.
        
        """
        if(event.type() == QtCore.QEvent.Create and 
           issubclass(type(o),QtGui.QWidget) and
           type(o) != QtGui.QSplashScreen):
            o.setAttribute(QtCore.Qt.WA_MacMetalStyle)
        return QtGui.QApplication.eventFilter(self,o,event)

    def save_configuration(self):
        """ save_configuration() -> None
        Save the current vistrail configuration to the startup.xml file.
        This is required to capture changes to the configuration that we 
        make programmatically during the session, ie., browsed directories or
        window sizes.

        """
        dom = self.vistrailsStartup.startup_dom()
        doc = dom.documentElement
        configuration_element = enter_named_element(doc, 'configuration')
        doc.removeChild(configuration_element)
        self.configuration.write_to_dom(dom, doc)
        self.vistrailsStartup.write_startup_dom(dom)
        dom.unlink()


# The initialization must be explicitly signalled. Otherwise, any
# modules importing vis_application will try to initialize the entire
# app.
def start_application(optionsDict=None):
    """Initializes the application singleton."""
    global VistrailsApplication
    if VistrailsApplication:
        print "Application already started."""
        return
    VistrailsApplication = VistrailsApplicationSingleton()
    try:
        core.requirements.check_all_vistrails_requirements()
    except core.requirements.MissingRequirement, e:
        msg = ("VisTrails requires %s to properly run.\n" %
               e.requirement)
        QtGui.QMessageBox.critical(None, "Missing requirement",
                                   msg)
        sys.exit(1)
    x = VistrailsApplication.init(optionsDict)
    if x == True:
        return 0
    else:
        return 1

VistrailsApplication = None

def stop_application():
    """Stop and finalize the application singleton."""
    global VistrailsApplication
    VistrailsApplication.save_configuration()
    VistrailsApplication.destroy()
    VistrailsApplication.deleteLater()

