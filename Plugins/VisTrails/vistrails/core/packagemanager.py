
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

"""The package manager takes care of everything that has got to do
with handling packages, from setting paths to adding new packages
to checking dependencies to initializing them."""

from core import debug
from core.modules.module_registry import registry
from core.utils import VistrailsInternalError, InstanceObject
from core.utils.uxml import (named_elements,
                             elements_filter, enter_named_element)
from core.configuration import ConfigurationObject, get_vistrails_configuration
import copy
import core.data_structures.graph
import os
import sys
import traceback
from PyQt4 import QtCore
##############################################################################

class Package(object):

    Base, User, Other = 0, 1, 2

    class InitializationFailed(Exception):
        def __init__(self, package, exception, traceback):
            self.package = package
            self.exception = exception
            self.traceback = traceback
        def __str__(self):
            return ("Package '%s' failed to initialize, raising '%s: %s'. Traceback:\n%s" %
                    (self.package.name,
                     self.exception.__class__.__name__,
                     self.exception,
                     self.traceback))

    class LoadFailed(Exception):
        def __init__(self, package, exception, traceback):
            self.package = package
            self.exception = exception
            self.traceback = traceback
        def __str__(self):
            return ("Package '%s' failed to load, raising '%s: %s'. Traceback:\n%s" %
                    (self.package._codepath,
                     self.exception.__class__.__name__,
                     self.exception,
                     self.traceback))

    def __init__(self, codepath, load_configuration=True):
        self._codepath = codepath
        self._load_configuration = load_configuration
        self._module = None
        self._initialized = False

    def load(self, module=None):
        """load(module=None). Loads package's module. If module is not None,
        then uses that as the module instead of 'import'ing it.

        If package is already initialized, this is a NOP.

        """

        errors = []
        def module_import(name):
            return

        if self._initialized:
            return
        if module is not None:
            self._module = module
            self._package_type = self.Other
            return
        def import_from(prefix):
            try:
                self._module = getattr(__import__(prefix+self._codepath,
                                                  globals(),
                                                  locals(), []),
                                       self._codepath)
                self._package_type = self.Base
            except ImportError, e:
                errors.append(e)
                return False
            return True

        try:
            r = (not import_from('packages.') and
                 not import_from('userpackages.'))
        except Exception, e:
            raise self.LoadFailed(self, e, traceback.format_exc())
            
        if r:
            dbg = debug.DebugPrint
            dbg.critical("Could not enable package %s" % self._codepath)
            for e in errors:
                dbg.critical("Exceptions raised:")
                dbg.critical(str(e))
            raise ImportError("Package %s not present "
                              "(or one of its imports failed)" % self._codepath)

        # Sometimes we don't want to change startup.xml, for example
        # when peeking at a package that's on the available package list
        # on edit -> preferences. That's what the _load_configuration field
        # is for
        if self._load_configuration:
            if hasattr(self._module, 'configuration'):
                # hold a copy of the initial configuration so it can be reset
                self._initial_configuration = copy.copy(self._module.configuration)
            self.load_persistent_configuration()
            self.create_startup_package_node()

    def initialize(self):
        if self._initialized:
            return
        print "Initializing", self._codepath
        registry.set_current_package_name(self._get_identifier())
        try:
            self._module.initialize()
            # Perform auto-initialization
            if hasattr(self._module, '_modules'):
                for module in self._module._modules:
                    registry.auto_add_module(module)
        except Exception, e:
            raise self.InitializationFailed(self, e, traceback.format_exc())

        # The package might have decided to rename itself, let's store that
        registry.set_current_package_name(None)
        self._initialized = True

    def report_missing_module(self, module_name, module_namespace):
        """report_missing_module(name, namespace):

        Calls the package's module handle_missing_module function, if
        present, to allow the package to dynamically add a missing
        module.
        """
        try:
            handle = self._module.handle_missing_module
        except AttributeError:
            return False
        try:
            return handle(module_name, module_namespace)
        except Exception, e:
            debug.DebugPrint.critical("Call to handle_missing_module in package '%s'"
                                      " raised exception '%s'. Assuming package could not"
                                      " handle call" % (self.name,
                                                        str(e)))
        return False

    def check_requirements(self):
        try:
            callable_ = self._module.package_requirements
        except AttributeError:
            return
        else:
            callable_()

    def menu_items(self):
        try:
            callable_ = self._module.menu_items
        except AttributeError:
            return None
        else:
            return callable_()

    def can_be_disabled(self):
        """Returns whether has no reverse dependencies (other
        packages that depend on it."""
        mgr = get_package_manager()
        return mgr.dependency_graph().in_degree(self.identifier) == 0

    def finalize(self):
        if not self._initialized:
            return
        print "Finalizing",self.name
        try:
            callable_ = self._module.finalize
        except AttributeError:
            pass
        else:
            callable_()
        # Save configuration
        if self.configuration:
            self.set_persistent_configuration()
        self._initialized = False

    def dependencies(self):
        try:
            callable_ = self._module.package_dependencies
        except AttributeError:
            return []
        else:
            return callable_()

    def reverse_dependencies(self):
        mgr = get_package_manager()
        lst = [x[0] for x in
               mgr.dependency_graph().inverse_adjacency_list[self.identifier]]
        return lst

    def initialized(self):
        return self._initialized

    def _get_name(self):
        try:
            return self._module.name
        except AttributeError, e:
            try:
                v = self._module.__file__
            except AttributeError:
                v = self._module
            msg = ("Package %s is missing attribute 'name'" % v)
            debug.DebugPrint.critical(msg)
            raise e

    # name is the human-readable package name
    name = property(_get_name)

    def _get_module(self):
        return self._module

    module = property(_get_module)

    def _get_configuration(self):
        if hasattr(self._module, 'configuration'):
            return self._module.configuration
        else:
            return None
    def _set_configuration(self, configuration):
        if hasattr(self._module, 'configuration'):
            self._module.configuration = configuration
        else:
            raise AttributeError("Can't set configuration on a module without one")
    configuration = property(_get_configuration, _set_configuration)

    def _get_description(self):
        if hasattr(self._module, '__doc__'):
            return self._module.__doc__ or "No description available"
        else:
            return "No description available"
    description = property(_get_description)


    def _get_version(self):
        return self._module.version
    version = property(_get_version)

    def _get_identifier(self):
        try:
            return self._module.identifier
        except AttributeError, e:
            try:
                v = self._module.__file__
            except AttributeError:
                v = self._module
            msg = ("Package %s is missing attribute 'identifier'" % v)
            debug.DebugPrint.critical(msg)
            raise e

    # identifier is the unique identifier across packages
    identifier = property(_get_identifier)

    # codepath is the package codepath
    def _get_codepath(self):
        return self._codepath
    codepath = property(_get_codepath)



    ##########################################################################
    # Configuration

    def find_disabledpackage_element(self, doc):
        """find_disabledpackage_element(documentElement) -> Node or None

        Returns the package's disabledpackage element, if
        present. Returns None otherwise.

        """
        packages = enter_named_element(doc, 'disabledpackages')
        assert packages
        for package_node in named_elements(packages, 'package'):
            if str(package_node.attributes['name'].value) == self.codepath:
                return package_node
        return None

    def remove_own_dom_element(self):
        """remove_own_dom_element() -> None

        Opens the startup DOM, looks for the element that belongs to the package.
        If it is there and there's a configuration, moves it to disabledpackages
        node. This is done as part of package disable.

        """
        from PyQt4 import QtCore
        startup = QtCore.QCoreApplication.instance().vistrailsStartup
        dom = startup.startup_dom()
        doc = dom.documentElement

        def find_it():
            packages = enter_named_element(doc, 'packages')
            for package_node in named_elements(packages, 'package'):
                if str(package_node.attributes['name'].value) == self.codepath:
                    return package_node

        package_node = find_it()
        oldpackage_element = self.find_disabledpackage_element(doc)

        assert oldpackage_element is None
        packages = enter_named_element(doc, 'packages')
        disabledpackages = enter_named_element(doc, 'disabledpackages')
        packages.removeChild(package_node)
        disabledpackages.appendChild(package_node)
        startup.write_startup_dom(dom)

    def reset_configuration(self):
        """Reset_configuration() -> Resets configuration to original
        package settings."""

        (dom, element) = self.find_own_dom_element()
        doc = dom.documentElement
        configuration = enter_named_element(element, 'configuration')
        if configuration:
            element.removeChild(configuration)
        self.configuration = copy.copy(self._initial_configuration)

        from PyQt4 import QtCore
        startup = QtCore.QCoreApplication.instance().vistrailsStartup
        startup.write_startup_dom(dom)

    def find_own_dom_element(self):
        """find_own_dom_element() -> (DOM, Node)

        Opens the startup DOM, looks for the element that belongs to the package,
        and returns DOM and node. Creates a new one if element is not there.

        """
        from PyQt4 import QtCore
        dom = QtCore.QCoreApplication.instance().vistrailsStartup.startup_dom()
        doc = dom.documentElement
        packages = enter_named_element(doc, 'packages')
        for package_node in named_elements(packages, 'package'):
            if str(package_node.attributes['name'].value) == self.codepath:
                return (dom, package_node)

        # didn't find anything, create a new node

        package_node = dom.createElement("package")
        package_node.setAttribute('name', self.codepath)
        packages.appendChild(package_node)

        from PyQt4 import QtCore
        QtCore.QCoreApplication.instance().vistrailsStartup.write_startup_dom(dom)
        return (dom, package_node)

    def load_persistent_configuration(self):
        (dom, element) = self.find_own_dom_element()

        configuration = enter_named_element(element, 'configuration')
        if configuration:
            self.configuration.set_from_dom_node(configuration)
        dom.unlink()

    def set_persistent_configuration(self):
        (dom, element) = self.find_own_dom_element()
        child = enter_named_element(element, 'configuration')
        if child:
            element.removeChild(child)
        self.configuration.write_to_dom(dom, element)
        from PyQt4 import QtCore
        QtCore.QCoreApplication.instance().vistrailsStartup.write_startup_dom(dom)
        dom.unlink()

    def create_startup_package_node(self):
        (dom, element) = self.find_own_dom_element()
        doc = dom.documentElement
        disabledpackages = enter_named_element(doc, 'disabledpackages')
        packages = enter_named_element(doc, 'packages')

        oldpackage = self.find_disabledpackage_element(doc)

        if oldpackage is not None:
            # Must remove element from oldpackages,
            # _and_ the element that was just created in find_own_dom_element()
            disabledpackages.removeChild(oldpackage)
            packages.removeChild(element)
            packages.appendChild(oldpackage)
            configuration = enter_named_element(oldpackage, 'configuration')
            if configuration:
                self.configuration.set_from_dom_node(configuration)
            from PyQt4 import QtCore
            QtCore.QCoreApplication.instance().vistrailsStartup.write_startup_dom(dom)
        dom.unlink()


##############################################################################

global _package_manager
_package_manager = None

class PackageManager(QtCore.QObject):
    # add_package_menu_signal is emitted with a tuple containing the package
    # identifier, package name and the menu item
    add_package_menu_signal = QtCore.SIGNAL("add_package_menu")
    # remove_package_menu_signal is emitted with the package identifier
    remove_package_menu_signal = QtCore.SIGNAL("remove_package_menu")
    #package_error_message_signal is emitted with the package identifier,
    # package name and the error message
    package_error_message_signal = QtCore.SIGNAL("package_error_message_signal")

    class DependencyCycle(Exception):
        def __init__(self, p1, p2):
            self._package_1 = p1
            self._package_2 = p2
        def __str__(self):
            return ("Packages '%s' and '%s' have cyclic dependencies" %
                    (self._package_1,
                     self._package_2))

    class MissingPackage(Exception):
        def __init__(self, n):
            self._package_name = n
        def __str__(self):
            return "Package '%s' is missing." % self._package_name

    class PackageInternalError(Exception):
        def __init__(self, n, d):
            self._package_name = n
            self._description = d
        def __str__(self):
            return "Package '%s' has a bug: %s" % (self._package_name,
                                                   self._description)

    def import_packages_module(self):
        """Imports the packages module using path trickery to find it
        in the right place.

        """
        # Imports standard packages directory
        conf = self._configuration
        old_sys_path = sys.path
        if conf.check('packageDirectory'):
            sys.path.append(conf.packageDirectory)
        try:
            import packages
        finally:
            sys.path = old_sys_path
        return packages


    def import_user_packages_module(self):
        """Imports the packages module using path trickery to find it
        in the right place.

        """
        # Imports user packages directory
        conf = self._configuration
        old_sys_path = sys.path
        if conf.check('userPackageDirectory'):
            sys.path.append(conf.userPackageDirectory + '/' + os.path.pardir)
        try:
            import userpackages
        finally:
            sys.path = old_sys_path
        return userpackages


    def __init__(self, configuration):
        global _package_manager
        if _package_manager:
            m = "Package manager can only be constructed once."
            raise VistrailsInternalError(m)
        QtCore.QObject.__init__(self)
        _package_manager = self
        self._configuration = configuration
        self._package_list = {}
        self._identifier_map = {}
        self._dependency_graph = core.data_structures.graph.Graph()

    def finalize_packages(self):
        """Finalizes all installed packages. Call this only prior to
exiting VisTrails."""
        for package in self._package_list.itervalues():
            package.finalize()
        self._package_list = {}
        self._identifier_map = {}        
        global _package_manager
        _package_manager = None

    def add_package(self, packageName):
        """Adds a new package to the manager. This does not initialize it.
To do so, call initialize_packages()"""
        self._package_list[packageName] = Package(packageName)

    def remove_package(self, codepath):
        """remove_package(name): Removes a package from the system."""
        pkg = self._package_list[codepath]
        self._dependency_graph.delete_vertex(pkg.identifier)
        del self._identifier_map[pkg.identifier]
        pkg.finalize()
        self.remove_menu_items(pkg)
        del self._package_list[codepath]
        registry.delete_package(pkg)

    def has_package(self, identifier):
        """has_package(identifer: string) -> Boolean.
Returns true if given package identifier is present."""
        return self._identifier_map.has_key(identifier)

    def look_at_available_package(self, codepath):
        """look_at_available_package(codepath: string) -> Package

        Returns a Package object for an uninstalled package. This does
        NOT install a package.
        """
        return Package(codepath, False)

    def get_package_by_codepath(self, codepath):
        """get_package_by_codepath(codepath: string) -> Package.
        Returns a package with given codepath if it is enabled,
        otherwise throws exception
        """
        if codepath not in self._package_list:
            raise self.MissingPackage(codepath)
        else:
            return self._package_list[codepath]

    def get_package_by_identifier(self, identifier):
        """get_package_by_identifier(identifier: string) -> Package.
        Returns a package with given identifier if it is enabled,
        otherwise throws exception
        """
        # FIXME: This should really be handled better
        if identifier == 'edu.utah.sci.vistrails.basic':
            return InstanceObject(name='Basic Modules')
        if identifier not in self._identifier_map:
            raise self.MissingPackage(identifier)
        else:
            return self._identifier_map[identifier]

    def get_package_configuration(self, codepath):
        """get_package_configuration(codepath: string) ->
        ConfigurationObject or None

        Returns the configuration object for the package, if existing,
        or None. Throws MissingPackage if package doesn't exist.
        """

        pkg = self.get_package_by_codepath(codepath)

        if not hasattr(pkg.module, 'configuration'):
            return None
        else:
            c = pkg.module.configuration
            if not isinstance(c, ConfigurationObject):
                d = "'configuration' attribute should be a ConfigurationObject"
                raise self.PackageInternalError(codepath, d)
            return c

    def add_dependencies(self, package):
        """add_dependencies(package) -> None.  Register all
        dependencies a package contains by calling the appropriate
        callback.

        Does not add multiple dependencies - if a dependency is already there,
        add_dependencies ignores it.
        """
        deps = package.dependencies()
        missing_packages = [identifier
                            for identifier in deps
                            if identifier not in self._dependency_graph.vertices]
        if len(missing_packages):
            raise ImportError("Package '%s' has unmet dependencies: %s" %
                              (package.name,
                               missing_packages))

        for dep_name in deps:
            if (dep_name not in
                self._dependency_graph.adjacency_list[package.identifier]):
                self._dependency_graph.add_edge(package.identifier, dep_name)

    def late_enable_package(self, package_codepath):
        """late_enable_package enables a package 'late', that is,
        after VisTrails initialization. All dependencies need to be
        already enabled.
        """
        if package_codepath in self._package_list:
            raise VistrailsInternalError('duplicate package identifier: %s' %
                                         package_codepath)
        self.add_package(package_codepath)
        pkg = self.get_package_by_codepath(package_codepath)
        pkg.load()
        self._dependency_graph.add_vertex(pkg.identifier)
        self._identifier_map[pkg.identifier] = pkg
        self.add_dependencies(pkg)
        pkg.check_requirements()
        pkg.initialize()
        self.add_menu_items(pkg)

    def late_disable_package(self, package_codepath):
        """late_disable_package disables a package 'late', that is,
        after VisTrails initialization. All reverse dependencies need to be
        already disabled.
        """
        pkg = self.get_package_by_codepath(package_codepath)
        self.remove_package(package_codepath)
        pkg.remove_own_dom_element()

    def initialize_packages(self,package_dictionary={}):
        """initialize_packages(package_dictionary={}): None

        Initializes all installed packages. If module_dictionary is
not {}, then it should be a dictionary from package names to preloaded
package-like objects (in theory they have to be modules that respect
the correct interface, but nothing actually prevents anyone from
creating a class that behaves similarly)."""
        packages = self.import_packages_module()
        userpackages = self.import_user_packages_module()

        failed = []
        # import the modules
        for package in self._package_list.itervalues():
            if package.initialized():
                continue
            try:
                package.load(package_dictionary.get(package.codepath, None))
            except Package.LoadFailed, e:
                print "FAILED TO LOAD, let's disable it"
                # We disable the package manually to skip over things
                # we know will not be necessary - the only thing needed is
                # the reference in the package list
                package.remove_own_dom_element()
                failed.append(package)
            else:
                if self._dependency_graph.vertices.has_key(package.identifier):
                    raise VistrailsInternalError('duplicate package identifier: %s' %
                                                 package.identifier)
                self._dependency_graph.add_vertex(package.identifier)
                self._identifier_map[package.identifier] = package

        for pkg in failed:
            del self._package_list[pkg.codepath]

        # determine dependencies
        for package in self._package_list.itervalues():
            self.add_dependencies(package)

        # perform actual initialization
        try:
            g = self._dependency_graph.inverse_immutable()
            sorted_packages = g.vertices_topological_sort()
        except core.data_structures.graph.Graph.GraphContainsCycles, e:
            raise self.DependencyCycle(e.back_edge[0],
                                       e.back_edge[1])

        for name in sorted_packages:
            pkg = self._identifier_map[name]
            if not pkg.initialized():
                pkg.check_requirements()
                pkg.initialize()
                self.add_menu_items(pkg)

    def add_menu_items(self, pkg):
        """add_menu_items(pkg: Package) -> None
        If the package implemented the function menu_items(),
        the package manager will emit a signal with the menu items to
        be added to the builder window """
        items = pkg.menu_items()
        if items:
            self.emit(self.add_package_menu_signal,
                      pkg.identifier,
                      pkg.name,
                      items)

    def remove_menu_items(self, pkg):
        """remove_menu_items(pkg: Package) -> None
        Send a signal with the pkg identifier. The builder window should
        catch this signal and remove the package menu items"""
        if pkg.menu_items():
            self.emit(self.remove_package_menu_signal,
                      pkg.identifier)

    def show_error_message(self, pkg, msg):
        """show_error_message(pkg: Package, msg: str) -> None
        Print a message to standard error output and emit a signal to the
        builder so if it is possible, a message box is also shown """
        print "Package %s (%s) says: %s"%(pkg.name,
                                         pkg.identifier,
                                         msg)
        self.emit(self.package_error_message_signal,
                  pkg.identifier,
                  pkg.name,
                  msg)

    def enabled_package_list(self):
        """package_list() -> returns list of all enabled packages."""
        return self._package_list.values()

    def identifier_is_available(self, identifier):
        """identifier_is_available(identifier: str) -> Pkg

        returns true if there exists a package with the given
        identifier in the list of available (ie, disabled) packages.

        If true, returns succesfully loaded, uninitialized package."""
        for codepath in self.available_package_names_list():
            try:
                pkg = self.get_package_by_codepath(codepath)
            except self.MissingPackage:
                pkg = self.look_at_available_package(codepath)
                try:
                    pkg.load()
                except pkg.LoadFailed:
                    pass
                if pkg.identifier == identifier:
                    return pkg
        return None

    def available_package_names_list(self):
        """available_package_names_list() -> returns list with code-paths of all
        available packages, by looking at the appropriate directories.

        The distinction between package names, identifiers and
        code-paths is described in doc/package_system.txt
        """

        lst = []

        def is_vistrails_package(path):
            return ((path.endswith('.py') and
                     not path.endswith('__init__.py') and
                     os.path.isfile(path)) or
                    os.path.isdir(path) and os.path.isfile(path + '/__init__.py'))

        def visit(_, dirname, names):
            for name in names:
                if is_vistrails_package(dirname + '/' + name):
                    if name.endswith('.py'):
                        name = name[:-3]
                    lst.append(name)
            # We want a shallow walk, so we prune the names list
            del names[:]

        # Finds standard packages
        import packages
        os.path.walk(os.path.dirname(packages.__file__), visit, None)
        import userpackages
        os.path.walk(os.path.dirname(userpackages.__file__), visit, None)

        return lst

    def dependency_graph(self):
        """dependency_graph() -> Graph.  Returns a graph with package
        dependencies, where u -> v if u depends on v.  Vertices are
        strings representing package names."""
        return self._dependency_graph

def get_package_manager():
    global _package_manager
    if not _package_manager:
        raise VistrailsInternalError("package manager not constructed yet.")
    return _package_manager

##############################################################################
