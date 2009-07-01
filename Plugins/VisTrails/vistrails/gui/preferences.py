
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

from PyQt4 import QtGui, QtCore
from core.packagemanager import get_package_manager
from core.utils.uxml import (named_elements,
                             elements_filter, enter_named_element)
from gui.configuration import QConfigurationWidget
from gui.configuration import QGeneralConfiguration
import os.path

##############################################################################

class QPackageConfigurationDialog(QtGui.QDialog):

    def __init__(self, parent, package):
        QtGui.QDialog.__init__(self, parent)

        self.setSizePolicy(QtGui.QSizePolicy.Expanding,
                           QtGui.QSizePolicy.Expanding)

        
        self.setWindowTitle('Configuration for package "%s"' % package.name)
        self._package = package
        c = package.configuration
        self._configuration_object = c
        assert c is not None

        layout = QtGui.QVBoxLayout(self)
        self.setLayout(layout)
        self._status_bar = QtGui.QStatusBar(self)

        self._configuration_widget = QConfigurationWidget(self, c,
                                                          self._status_bar)
        layout.addWidget(self._configuration_widget)

        btns = (QtGui.QDialogButtonBox.Close |
                QtGui.QDialogButtonBox.RestoreDefaults)
        self._button_box = QtGui.QDialogButtonBox(btns,
                                                  QtCore.Qt.Horizontal,
                                                  self)
        self.connect(self._button_box,
                     QtCore.SIGNAL('clicked(QAbstractButton *)'),
                     self.button_clicked)

        self.connect(self._configuration_widget._tree.treeWidget,
                     QtCore.SIGNAL('configuration_changed'),
                     self.configuration_changed)
                     
        layout.addWidget(self._status_bar)
        layout.addWidget(self._button_box)

    def button_clicked(self, button):
        role = self._button_box.buttonRole(button)
        if role == QtGui.QDialogButtonBox.ResetRole:
            txt = ("This will reset all configuration values of " +
                   "this package to their default values. Do you " +
                   "want to proceed?")
            msg_box = QtGui.QMessageBox(QtGui.QMessageBox.Question,
                                        "Really reset?", txt,
                                        (QtGui.QMessageBox.Yes |
                                         QtGui.QMessageBox.No))
            if msg_box.exec_() == QtGui.QMessageBox.Yes:
                self.reset_configuration()
        else:
            assert role == QtGui.QDialogButtonBox.RejectRole
            self.close_dialog()

    def reset_configuration(self):
        self._package.reset_configuration()
        conf = self._package.configuration
        self._configuration_widget.configuration_changed(conf)

    def close_dialog(self):
        self.done(0)

    def configuration_changed(self, item, new_value):
        self._package.set_persistent_configuration()

##############################################################################

class QPackagesWidget(QtGui.QWidget):

    ##########################################################################
    # Initalization

    def __init__(self, parent, status_bar):
        QtGui.QWidget.__init__(self, parent)
        self._status_bar = status_bar

        base_layout = QtGui.QHBoxLayout(self)
        
        left = QtGui.QFrame(self)
        right = QtGui.QFrame(self)

        base_layout.addWidget(left)
        base_layout.addWidget(right, 1)
        
        ######################################################################
        left_layout = QtGui.QVBoxLayout(left)
        left_layout.setMargin(2)
        left_layout.setSpacing(2)
       
        left_layout.addWidget(QtGui.QLabel("Disabled packages:", left))
        self._available_packages_list = QtGui.QListWidget(left)
        left_layout.addWidget(self._available_packages_list)
        left_layout.addWidget(QtGui.QLabel("Enabled packages:", left))
        self._enabled_packages_list = QtGui.QListWidget(left)
        left_layout.addWidget(self._enabled_packages_list)

        self.connect(self._available_packages_list,
                     QtCore.SIGNAL('itemClicked(QListWidgetItem*)'),
                     self.clicked_on_available_list)
        self.connect(self._available_packages_list,
                     QtCore.SIGNAL('itemPressed(QListWidgetItem*)'),
                     self.pressed_available_list)

        self.connect(self._enabled_packages_list,
                     QtCore.SIGNAL('itemClicked(QListWidgetItem*)'),
                     self.clicked_on_enabled_list)
        self.connect(self._enabled_packages_list,
                     QtCore.SIGNAL('itemPressed(QListWidgetItem*)'),
                     self.pressed_enabled_list)

        sm = QtGui.QAbstractItemView.SingleSelection
        self._available_packages_list.setSelectionMode(sm)
        self._enabled_packages_list.setSelectionMode(sm)


        ######################################################################
        right_layout = QtGui.QVBoxLayout(right)
        info_frame = QtGui.QFrame(right)

        info_layout = QtGui.QVBoxLayout(info_frame)
        grid_frame = QtGui.QFrame(info_frame)
        grid_frame.setSizePolicy(QtGui.QSizePolicy.Expanding,
                                 QtGui.QSizePolicy.Expanding)

        info_layout.addWidget(grid_frame)
        grid_layout = QtGui.QGridLayout(grid_frame)
        l1 = QtGui.QLabel("Package Name:", grid_frame)
        grid_layout.addWidget(l1, 0, 0)
        l2 = QtGui.QLabel("Identifier:", grid_frame)
        grid_layout.addWidget(l2, 1, 0)
        l3 = QtGui.QLabel("Dependencies:", grid_frame)
        grid_layout.addWidget(l3, 2, 0)
        l4 = QtGui.QLabel("Reverse Dependencies:", grid_frame)
        grid_layout.addWidget(l4, 3, 0)
        l5 = QtGui.QLabel("Description:", grid_frame)
        grid_layout.addWidget(l5, 4, 0)

        self._name_label = QtGui.QLabel("", grid_frame)
        grid_layout.addWidget(self._name_label, 0, 1)

        self._identifier_label = QtGui.QLabel("", grid_frame)
        grid_layout.addWidget(self._identifier_label, 1, 1)

        self._dependencies_label = QtGui.QLabel("", grid_frame)
        grid_layout.addWidget(self._dependencies_label, 2, 1)

        self._reverse_dependencies_label = QtGui.QLabel("", grid_frame)
        grid_layout.addWidget(self._reverse_dependencies_label, 3, 1)

        self._description_label = QtGui.QLabel("", grid_frame)
        grid_layout.addWidget(self._description_label, 4, 1)

        for lbl in [l1, l2, l3, l4, l5,
                    self._name_label,
                    self._dependencies_label,
                    self._identifier_label,
                    self._reverse_dependencies_label,
                    self._description_label]:
            lbl.setAlignment(QtCore.Qt.AlignTop | QtCore.Qt.AlignLeft)
            lbl.setWordWrap(True)

        grid_layout.setRowStretch(4, 1)
        grid_layout.setColumnStretch(1, 1)

        right_layout.addWidget(info_frame)
        
        self._enable_button = QtGui.QPushButton("&Enable")
        self._enable_button.setEnabled(False)
        self.connect(self._enable_button,
                     QtCore.SIGNAL("clicked()"),
                     self.enable_current_package)
        self._disable_button = QtGui.QPushButton("&Disable")
        self._disable_button.setEnabled(False)
        self.connect(self._disable_button,
                     QtCore.SIGNAL("clicked()"),
                     self.disable_current_package)
        self._configure_button = QtGui.QPushButton("&Configure...")
        self._configure_button.setEnabled(False)
        self.connect(self._configure_button,
                     QtCore.SIGNAL("clicked()"),
                     self.configure_current_package)
        button_box = QtGui.QDialogButtonBox()
        button_box.addButton(self._enable_button, QtGui.QDialogButtonBox.ActionRole)
        button_box.addButton(self._disable_button, QtGui.QDialogButtonBox.ActionRole)
        button_box.addButton(self._configure_button, QtGui.QDialogButtonBox.ActionRole)
        right_layout.addWidget(button_box)
        
        self.populate_lists()

        self._current_package = None

    def populate_lists(self):
        pkg_manager = get_package_manager()
        enabled_pkgs = sorted(pkg_manager.enabled_package_list())
        enabled_pkg_dict = dict([(pkg.codepath, pkg) for
                                   pkg in enabled_pkgs])
        for pkg in enabled_pkgs:
            self._enabled_packages_list.addItem(pkg.codepath)
        available_pkg_names = [pkg for pkg in 
                               sorted(pkg_manager.available_package_names_list())
                               if pkg not in enabled_pkg_dict]
        for pkg in available_pkg_names:
            self._available_packages_list.addItem(pkg)

    ##########################################################################

    def enable_current_package(self):
        av = self._available_packages_list
        inst = self._enabled_packages_list
        item = av.currentItem()
        pos = av.indexFromItem(item).row()
        codepath = str(item.text())
        pm = get_package_manager()

        dependency_graph = pm.dependency_graph()
        new_deps = self._current_package.dependencies()

        unmet_dep = None

        for dep in new_deps:
            if dep not in dependency_graph.vertices:
                unmet_dep = dep
                break
        if unmet_dep:
            msg = QtGui.QMessageBox(QtGui.QMessageBox.Critical,
                                    "Missing dependency",
                                    ("This package requires package '%s'\n" +
                                     "to be enabled. (Complete dependency list is:\n" +
                                     "%s)") % (dep, new_deps),
                                    QtGui.QMessageBox.Ok, self)
            msg.exec_()
        else:
            palette = QtGui.QApplication.instance().builderWindow.modulePalette
            palette.setUpdatesEnabled(False)
            try:
                pm.late_enable_package(codepath)
            finally:
                palette.setUpdatesEnabled(True)
                palette.treeWidget.expandAll()
            self._current_package = pm.get_package_by_codepath(codepath)
            av.takeItem(pos)
            av.clearSelection()
            inst.addItem(item)
            inst.sortItems()
            inst.clearSelection()

    def disable_current_package(self):
        av = self._available_packages_list
        inst = self._enabled_packages_list
        item = inst.currentItem()
        pos = inst.indexFromItem(item).row()
        codepath = str(item.text())
        pm = get_package_manager()

        dependency_graph = pm.dependency_graph()
        identifier = pm.get_package_by_codepath(codepath).identifier

        if dependency_graph.in_degree(identifier) > 0:
            rev_deps = dependency_graph.inverse_adjacency_list[identifier]
            msg = QtGui.QMessageBox(QtGui.QMessageBox.Critical,
                                    "Missing dependency",
                                    ("There are other packages that depend on this:\n %s" +
                                     "Please disable those first.") % rev_deps,
                                    QtGui.QMessageBox.Ok, self)
            msg.exec_()
        else:
            pm.late_disable_package(codepath)
            inst.takeItem(pos)
            av.addItem(item)
            av.sortItems()
            av.clearSelection()
            inst.clearSelection()

    def configure_current_package(self):
        dlg = QPackageConfigurationDialog(self, self._current_package)
        dlg.exec_()

    def set_buttons_to_enabled_package(self):
        self._enable_button.setEnabled(False)
        assert self._current_package
        can_disable = self._current_package.can_be_disabled()
        self._disable_button.setEnabled(can_disable)
        if not can_disable:
            msg = ("Module has reverse dependencies that must\n"+
                   "be first disabled.")
            self._disable_button.setToolTip(msg)
        else:
            self._disable_button.setToolTip("")
        conf = self._current_package.configuration is not None
        self._configure_button.setEnabled(conf)

    def set_buttons_to_available_package(self):
        self._configure_button.setEnabled(False)
        self._disable_button.setEnabled(False)
        self._enable_button.setEnabled(True)

    def set_package_information(self):
        """Looks at current package and sets all labels (name,
        dependencies, etc.) appropriately.

        """
        assert self._current_package
        p = self._current_package
        try:
            p.load()
        except:
            msg = 'ERROR: Could not load package.'
            self._name_label.setText(msg)
            self._identifier_label.setText(msg)
            self._dependencies_label.setText(msg)
            self._description_label.setText(msg)
            self._reverse_dependencies_label.setText(msg)
        else:
            self._name_label.setText(p.name)
            deps = ', '.join(p.dependencies()) or 'No package dependencies.'
            try:
                reverse_deps = (', '.join(p.reverse_dependencies()) or
                                'No reverse dependencies.')
            except KeyError:
                reverse_deps = ("Reverse dependencies only " +
                                "available for enabled packages.")
            self._identifier_label.setText(p.identifier)
            self._dependencies_label.setText(deps)
            self._description_label.setText(' '.join(p.description.split('\n')))
            self._reverse_dependencies_label.setText(reverse_deps)


    ##########################################################################
    # Signal handling

    def pressed_enabled_list(self, item):
        self._available_packages_list.clearSelection()

    def pressed_available_list(self, item):
        self._enabled_packages_list.clearSelection()

    def clicked_on_enabled_list(self, item):
        codepath = str(item.text())
        pm = get_package_manager()
        self._current_package = pm.get_package_by_codepath(codepath)
        self.set_buttons_to_enabled_package()
        self.set_package_information()

    def clicked_on_available_list(self, item):
        codepath = str(item.text())
        pm = get_package_manager()
        self._current_package = pm.look_at_available_package(codepath)
        self.set_buttons_to_available_package()
        self.set_package_information()



class QPreferencesDialog(QtGui.QDialog):

    def __init__(self, parent):
        QtGui.QDialog.__init__(self, parent)
        self._status_bar = QtGui.QStatusBar(self)
        self.setWindowTitle('VisTrails Preferences')
        layout = QtGui.QHBoxLayout(self)
        layout.setMargin(0)
        layout.setSpacing(0)
        self.setLayout(layout)

        f = QtGui.QFrame()
        layout.addWidget(f)
        
        l = QtGui.QVBoxLayout(f)
        f.setLayout(l)
        
        self._tab_widget = QtGui.QTabWidget(f)
        l.addWidget(self._tab_widget)
        self._tab_widget.setSizePolicy(QtGui.QSizePolicy.Expanding,
                                       QtGui.QSizePolicy.Expanding)

        self._general_tab = self.create_general_tab()
        self._tab_widget.addTab(self._general_tab, 'General Configuration')

        self._packages_tab = self.create_packages_tab()
        self._tab_widget.addTab(self._packages_tab, 'Module Packages')
        
        self._configuration_tab = self.create_configuration_tab()
        self._tab_widget.addTab(self._configuration_tab, 'Expert Configuration')

        self._button_box = QtGui.QDialogButtonBox(QtGui.QDialogButtonBox.Close,
                                                  QtCore.Qt.Horizontal,
                                                  f)
        self.connect(self._tab_widget,
                     QtCore.SIGNAL('currentChanged(int)'),
                     self.tab_changed)

        self.connect(self._button_box,
                     QtCore.SIGNAL('clicked(QAbstractButton *)'),
                     self.close_dialog)

        self.connect(self._configuration_tab._tree.treeWidget,
                     QtCore.SIGNAL('configuration_changed'),
                     self.configuration_changed)

        self.connect(self._general_tab,
                     QtCore.SIGNAL('configuration_changed'),
                     self.configuration_changed)

        l.addWidget(self._button_box)
        l.addWidget(self._status_bar)

    def close_dialog(self):
        self.done(0)

    def create_general_tab(self):
        """ create_general_tab() -> None
        
        """
        from gui.application import VistrailsApplication
        return QGeneralConfiguration(self,
                                     VistrailsApplication.configuration)

    def create_configuration_tab(self):
        from gui.application import VistrailsApplication
        return QConfigurationWidget(self,
                                    VistrailsApplication.configuration,
                                    self._status_bar)

    def create_packages_tab(self):
        return QPackagesWidget(self, self._status_bar)

    def sizeHint(self):
        return QtCore.QSize(800, 600)

    def tab_changed(self, index):
        """ tab_changed(index: int) -> None
        Keep general and advanced configurations in sync
        
        """
        from gui.application import VistrailsApplication
        self._configuration_tab.configuration_changed(
            VistrailsApplication.configuration)
        self._general_tab.update_state(
            VistrailsApplication.configuration)

    
    def configuration_changed(self, item, new_value):
        """ configuration_changed(item: QTreeWidgetItem *, 
        new_value: QString) -> None
        Write the current session configuration to startup.xml.
        Note:  This is already happening on close to capture configuration
        items that are not set in preferences.  Do we still need to do it 
        with every preference change?
        
        """
        from PyQt4 import QtCore
        from gui.application import VistrailsApplication
        VistrailsApplication.save_configuration()
