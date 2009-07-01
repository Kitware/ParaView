
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

"""Describes the start-up process of VisTrails"""

from core import debug
from core import system
from core.utils.uxml import named_elements, elements_filter, \
     eval_xml_value, enter_named_element
import copy
import core.packagemanager
import core.utils
import os.path
import shutil
import sys
import tempfile
import core.configuration
import xml.dom.minidom

################################################################################

class VistrailsStartup(object):
    """
    VistrailsStartup is the class that initializes VisTrails based on
    a configuration. Both application mode (interactive and
    non-interactive) and import mode of VisTrails use this start up
    process to install packages and run .vistrails file. The purpose
    of this class is to separate the initialization process with Qt
    Application
    
    """

    def __init__(self, config=None):
        """ VistrailsStartup(config: ConfigurationObject) -> None
        Setup the configuration
        
        """
        assert (config is None or
                isinstance(config, core.configuration.ConfigurationObject))
        if config:
            self.configuration = config
        else:
            self.configuration = core.configuration.default()
        self._package_manager = core.packagemanager.PackageManager(
            self.configuration)
        self.startupHooks = []
        self._python_environment = self.runDotVistrails()
        self.load_configuration()
        
    def init(self):
        """ init() -> None        
        Initialize VisTrails with optionsDict. optionsDict can be
        another VisTrails Configuration object, e.g. ConfigurationObject
        
        """
        self.load_packages()
        self.setupDefaultFolders()
        self.setupBaseModules()
        self.installPackages()
        self.runStartupHooks()

    ##########################################################################
    # startup.xml related

    def startup_dom(self):
        filename = os.path.join(self.configuration.dotVistrails,'startup.xml')
        return xml.dom.minidom.parse(filename)

    def write_startup_dom(self, dom):
        filename = os.path.join(self.configuration.dotVistrails,'startup.xml')
        f = file(filename, 'w')
        f.write(dom.toxml())
    
    def load_configuration(self):
        """load_configuration() -> None

        Loads the appropriate configuration from .vistrails/startup.xml.
        """
        dom = self.startup_dom()
        conf = enter_named_element(dom.documentElement, 'configuration')
        self.configuration.set_from_dom_node(conf)

    def load_packages(self):
        """load_packages() -> None

        Loads the appropriate packages from .vistrails/startup.xml.
        """
        
        def parse_package(node):
            is_value = (lambda node: node.nodeName in
                        set(['bool', 'str', 'int', 'float']))
            package_name = str(node.attributes['name'].value)
            self._package_manager.add_package(package_name)
        dom = self.startup_dom()
        doc = dom.documentElement
        packages_node = enter_named_element(doc, 'packages')
        for package_node in named_elements(packages_node, 'package'):
            parse_package(package_node)

    ##########################################################################

    def get_python_environment(self):
        """get_python_environment(): returns the python environment generated
by startup.py. This should only be called after init()."""
        return self._python_environment

    def runDotVistrails(self):
        """ runDotVistrails() -> None
        Setup to run user .vistrails file

        """        
        def addStartupHook(hook):
            """ addStartupHook(hook: function) -> None
            Add a hook for start-up after initialization
            
            """
            self.startupHooks.append(hook)

        def addPackage(packageName, *args, **keywords):
            """ addPackage(packageName: str, *args) -> None
            """
            self._package_manager.add_package(packageName)

        def create_user_packages_dir():
            debug.critical('Will try to create userpackages directory')
            userpackagesname = os.path.join(self.configuration.dotVistrails,
                                            'userpackages')
            if not os.path.isdir(userpackagesname):
                try:
                    os.mkdir(userpackagesname)
                except:
                    msg = ("""Failed to create userpackages directory: '%s'.
                    This could be an indication of a permissions problem.
                    Make sure directory '%s' in writable.""" %
                           (userpackagesname,
                            self.configuration.dotVistrails))
                    debug.critical(msg)
                    sys.exit(1)
            try:
                name = os.path.join(userpackagesname, '__init__.py')
                f = file(name, 'w')
                f.write('pass\n')
                f.close()
            except:
                msg = ("""Failed to create file '%s'. This could indicate a
                rare combination of a race condition and a permissions problem.
                Please make sure it is writable.""" % name)
                debug.critical(msg)
                sys.exit(1)
                       

        def install_default_startup():
            debug.critical('Will try to create default startup script')
            try:
                root_dir = core.system.vistrails_root_directory()
                default_file = os.path.join(root_dir,'core','resources',
                                            'default_vistrails_startup')
                user_file = os.path.join(self.configuration.dotVistrails,
                                         'startup.py')
                shutil.copyfile(default_file,user_file)
                debug.log('Succeeded!')
            except:
                debug.critical("""Failed to copy default file to .vistrails.
                This could be an indication of a permissions problem.
                Make sure directory '%s' is writable"""
                % self.configuration.dotVistrails)
                sys.exit(1)

        def install_default_startupxml_if_needed():
            fname = os.path.join(self.configuration.dotVistrails,
                                 'startup.xml')
            root_dir = core.system.vistrails_root_directory() 
            origin = os.path.join(root_dir, 'core','resources',
                                  'default_vistrails_startup_xml')
            def skip():
                if os.path.isfile(fname):
                    try:
                        d = self.startup_dom()
                        v = str(d.getElementsByTagName('startup')[0].attributes['version'].value)
                        r = core.utils.version_string_to_list(v)
                        return r >= [0, 1]
                    except:
                        return False
                else:
                    return False
            if skip():
                return
            try:
                shutil.copyfile(origin, fname)
                debug.log('Succeeded!')
            except:
                debug.critical("""Failed to copy default configuration
                file to .vistrails. This could be an indication of a
                permissions problem. Please make sure '%s' is writable."""
                               % self.configuration.dotVistrails)

        def create_default_directory():
            debug.critical('Will try to create default directory')
            try:
                os.mkdir(self.configuration.dotVistrails)
                debug.critical('Succeeded!')
            except:
                debug.critical("""Failed to create initialization directory.
                This could be an indication of a permissions problem.
                Make sure parent directory of '%'s is writable."""
                % self.configuration.dotVistrails)
                sys.exit(1)

        def execDotVistrails(tried_once=False):
            """ execDotVistrails() -> None
            Actually execute the Vistrail initialization
            
            """
            # if it is file, then must move old-style .vistrails to
            # directory.
            if os.path.isfile(self.configuration.dotVistrails):
                debug.warning("Old-style initialization hooks. Will try to set things correctly.")
                (fd, name) = tempfile.mkstemp()
                os.close(fd)
                shutil.copyfile(self.configuration.dotVistrails, name)
                try:
                    os.unlink(self.configuration.dotVistrails)
                except:
                    debug.critical("""Failed to remove old initialization file.
                    This could be an indication of a permissions problem.
                    Make sure file '%s' is writable."""
                    % self.configuration.dotVistrails)
                    sys.exit(1)
                create_default_directory()
                try:
                    destiny = os.path.join(self.configuration.dotVistrails,
                                           'startup.py')
                    shutil.copyfile(name, destiny)
                except:
                    debug.critical("""Failed to copy old initialization file to
                    newly-created initialization directory. This must have been
                    a race condition. Please remove '%s' and
                    restart VisTrails."""
                    % self.configuration.dotVistrails)
                    sys.exit(1)
                debug.critical("Successful move!")
                try:
                    os.unlink(name)
                except:
                    debug.warning("Failed to erase temporary file.")

            if os.path.isdir(self.configuration.dotVistrails):
                userpackages = os.path.join(self.configuration.dotVistrails,
                                            'userpackages')
                startup = os.path.join(self.configuration.dotVistrails,
                                       'startup.py')
                if not os.path.isdir(userpackages):
                    create_user_packages_dir()
                try:
                    
                    dotVistrails = file(startup)
                    g = {}
                    localsDir = {'configuration': self.configuration,
                                 'addStartupHook': addStartupHook,
                                 'addPackage': addPackage}
                    old_path = copy.copy(sys.path)
                    sys.path.append(self.configuration.dotVistrails)
                    exec dotVistrails in localsDir
                    sys.path = old_path
                    del localsDir['addPackage']
                    del localsDir['addStartupHook']
                    return localsDir
                except IOError:
                    if tried_once:
                        debug.critical("""Still cannot find default file.
                        Something has gone wrong. Please make sure ~/.vistrails
                        exists, is writable, and ~/.vistrails/startup.py does
                        not exist.""")
                        sys.exit(1)
                    debug.critical('%s not found' % startup)
                    debug.critical('Will try to install default' +
                                              'startup file')
                    install_default_startup()
                    install_default_startupxml_if_needed()
                    return execDotVistrails(True)
            elif not os.path.lexists(self.configuration.dotVistrails):
                debug.critical('%s not found' % self.configuration.dotVistrails)
                create_default_directory()
                install_default_startup()
                install_default_startupxml_if_needed()
                return execDotVistrails(True)


        install_default_startupxml_if_needed()
        # Now execute the dot vistrails
        return execDotVistrails()

    def setupDefaultFolders(self):
        """ setupDefaultFolders() -> None        
        Give default values to folders when there are no values specified
        
        """
        if self.configuration.has('rootDirectory'):
            system.set_vistrails_root_directory(self.configuration.rootDirectory)
        if self.configuration.has('dataDirectory'):
            system.set_vistrails_data_directory( \
                self.configuration.dataDirectory)
        if self.configuration.has('fileDirectory'):
            system.set_vistrails_file_directory( \
                self.configuration.fileDirectory)
        if (self.configuration.has('verbosenessLevel') and
            self.configuration.verbosenessLevel != -1):
            dbg = debug.DebugPrint
            verbose = self.configuration.verbosenessLevel
            if verbose < 0:
                msg = ("""Don't know how to set verboseness level to %s - "
                       "setting tothe lowest one I know of: 0""" % verbose)
                dbg.critical(msg)
                verbose = 0
            if verbose > 2:
                msg = ("""Don't know how to set verboseness level to %s - "
                       "setting to the highest one I know of: 2""" % verbose)
                dbg.critical(msg)
                verbose = 2
            levels = [dbg.Critical, dbg.Warning, dbg.Log]
            dbg.set_message_level(levels[verbose])
            dbg.log("Set verboseness level to %s" % verbose)
        if not self.configuration.has('userPackageDirectory'):
            s = os.path.join(core.system.default_dot_vistrails(),
                             'userpackages')
            self.configuration.userPackageDirectory = s

    def setupBaseModules(self):
        """ setupBaseModules() -> None        
        Import basic modules for self-registration. The import here is
        on purpose, not a typo against the coding rule
        
        """
        import core.modules.vistrails_module
        import core.modules.basic_modules
        import core.modules.sub_module

    def installPackages(self):
        """ installPackages() -> None
        Scheme through packages directory and initialize them all
        """
        # Imports standard packages directory
        self._package_manager.initialize_packages()

    def runStartupHooks(self):
        """ runStartupHooks() -> None
        After initialization, need to run all start up hooks registered
        
        """
        for hook in self.startupHooks:
            try:
                hook()
            except Exception, e:
                dbg = debug.DebugPrint
                dbg.critical("Exception raised during hook: %s - %s" %
                             (e.__class__, e))

    def destroy(self):
        """ destroy() -> None
        Finalize all packages to, such as, get rid of temp files
        
        """
        self._package_manager.finalize_packages()
