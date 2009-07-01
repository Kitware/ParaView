
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

"""Widgets to display/edit configuration objects."""

from PyQt4 import QtGui, QtCore
from gui.common_widgets import QSearchTreeWindow, QSearchTreeWidget
from core.configuration import ConfigurationObject
import core.system

##############################################################################

def bool_conv(st):
    if st == 'True':
        return True
    elif st == 'False':
        return False
    else:
        raise TypeError('Bogus value for bool_conv ' + str(st))

class QConfigurationTreeWidgetItem(QtGui.QTreeWidgetItem):

    def __init__(self, parent, obj, parent_obj, name):
        lst = QtCore.QStringList(name)
        t = type(obj)
        if t == bool:
            self._obj_type = bool_conv
        else:
            self._obj_type = t
        self._parent_obj = parent_obj
        self._name = name
        if t == ConfigurationObject:
            lst << '' << ''
            QtGui.QTreeWidgetItem.__init__(self, parent, lst)
            self.setFlags(self.flags() & ~(QtCore.Qt.ItemIsDragEnabled |
                                           QtCore.Qt.ItemIsSelectable ))
        elif t == tuple and obj[0] is None and type(obj[1]) == type:
            self._obj_type = obj[1]
            lst << '' << obj[1].__name__
            QtGui.QTreeWidget.__init__(self, parent, lst)
            self.setFlags((self.flags() & ~QtCore.Qt.ItemIsDragEnabled) |
                          QtCore.Qt.ItemIsEditable)
        else:
            lst << str(obj) << type(obj).__name__
            QtGui.QTreeWidgetItem.__init__(self, parent, lst)
            self.setFlags((self.flags() & ~QtCore.Qt.ItemIsDragEnabled) |
                          QtCore.Qt.ItemIsEditable)

    def change_value(self, new_value):
        if self._parent_obj:
            setattr(self._parent_obj, self._name, self._obj_type(new_value))

    def _get_name(self):
        return self._name
    name = property(_get_name)

class QConfigurationTreeWidgetItemDelegate(QtGui.QItemDelegate):
    """
    QConfigurationTreeWidgetItemDelegate allows a custom editor for
    each column of the QConfigurationTreeWidget    
    """
    
    def createEditor(self, parent, option, index):
        """ createEditor(parent: QWidget,
                         option: QStyleOptionViewItem,
                         index: QModelIndex) -> QWidget
        Return the editing widget depending on columns
        
        """
        # We only allow users to edit the  second column
        if index.column()==1:
            dataType = str(index.sibling(index.row(), 2).data().toString())
            
            # Create the editor based on dataType
            if dataType=='int':
                editor = QtGui.QLineEdit(parent)
                editor.setValidator(QtGui.QIntValidator(parent))
            elif dataType=='bool':
                editor = QtGui.QComboBox(parent)
                editor.addItem('True')
                editor.addItem('False')
            else:
                editor = QtGui.QItemDelegate.createEditor(self, parent,
                                                          option, index)
            return editor            
        return None

    def setEditorData(self, editor, index):
        """ setEditorData(editor: QWidget, index: QModelIndex) -> None
        Set the editor to reflects data at index
        
        """
        if type(editor)==QtGui.QComboBox:           
            editor.setCurrentIndex(editor.findText(index.data().toString()))
        else:
            QtGui.QItemDelegate.setEditorData(self, editor, index)

    def setModelData(self, editor, model, index):
        """ setModelData(editor: QStringEdit,
                         model: QAbstractItemModel,
                         index: QModelIndex) -> None
        Set the text of the editor back to the item model
        
        """
        if type(editor)==QtGui.QComboBox:
            model.setData(index, QtCore.QVariant(editor.currentText()))
        elif type(editor) == QtGui.QLineEdit:
            model.setData(index, QtCore.QVariant(editor.text()))
        else:
            # Should never get here
            assert False
    

class QConfigurationTreeWidget(QSearchTreeWidget):

    def __init__(self, parent, configuration_object):
        QSearchTreeWidget.__init__(self, parent)
        self.setMatchedFlags(QtCore.Qt.ItemIsEditable)
        self.setColumnCount(3)
        lst = QtCore.QStringList()
        lst << 'Name'
        lst << 'Value'
        lst << 'Type'
        self.setHeaderLabels(lst)
        self.create_tree(configuration_object)

    def create_tree(self, configuration_object):
        def create_item(parent, obj, parent_obj, name):
            item = QConfigurationTreeWidgetItem(parent, obj, parent_obj, name)
            if type(obj) == ConfigurationObject:
                for key in sorted(obj.keys()):
                    create_item(item, getattr(obj, key), obj, key)

        # disconnect() and clear() are here because create_tree might
        # also be called when an entirely new configuration object is set.

        self.disconnect(self, QtCore.SIGNAL('itemChanged(QTreeWidgetItem *, int)'),
                        self.change_configuration)
        self.clear()
        self._configuration = configuration_object
        create_item(self, self._configuration, None, 'configuration')

        self.expandAll()
        self.resizeColumnToContents(0)
        self.connect(self,
                     QtCore.SIGNAL('itemChanged(QTreeWidgetItem *, int)'),
                     self.change_configuration)

    def change_configuration(self, item, col):
        if item.flags() & QtCore.Qt.ItemIsEditable:
            new_value = self.indexFromItem(item, col).data().toString()
            item.change_value(new_value)
            self.emit(QtCore.SIGNAL('configuration_changed'),
                      item, new_value)
        
class QConfigurationTreeWindow(QSearchTreeWindow):

    def __init__(self, parent, configuration_object):
        self._configuration_object = configuration_object
        QSearchTreeWindow.__init__(self, parent)

    def createTreeWidget(self):
        self.setWindowTitle('Configuration')
        treeWidget = QConfigurationTreeWidget(self, self._configuration_object)
        
        # The delegate has to be around (self._delegate) to
        # work, else the instance will be clean by Python...
        self._delegate = QConfigurationTreeWidgetItemDelegate()
        treeWidget.setItemDelegate(self._delegate)
        return treeWidget


class QConfigurationWidget(QtGui.QWidget):

    def __init__(self, parent, configuration_object, status_bar):
        QtGui.QWidget.__init__(self, parent)
        layout = QtGui.QVBoxLayout(self)
        self.setLayout(layout)
        self._status_bar = status_bar

        self._tree = QConfigurationTreeWindow(self, configuration_object)
        lbl = QtGui.QLabel("Set configuration variables for VisTrails here.", self)
        layout.addWidget(lbl)
        layout.addWidget(self._tree)

    def configuration_changed(self, new_configuration):
        self._tree.treeWidget.create_tree(new_configuration)

class QGeneralConfiguration(QtGui.QWidget):
    """
    QGeneralConfiguration is a widget for showing a few general preferences
    that can be set with widgets.

    """
    def __init__(self, parent, configuration_object):
        """
        QGeneralConfiguration(parent: QWidget, 
        configuration_object: ConfigurationObject) -> None

        """
        QtGui.QWidget.__init__(self, parent)
        layout = QtGui.QVBoxLayout()
        layout.setMargin(10)
        layout.setSpacing(10)
        self.setLayout(layout)
        self._configuration = None
        self.create_default_widgets(self,layout)
        self.create_other_widgets(self,layout)
        self.update_state(configuration_object)
        self.connect_default_signals()
        self.connect_other_signals()
        
    def connect_default_signals(self):
        self.connect(self._autosave_cb,
                     QtCore.SIGNAL('stateChanged(int)'),
                     self.autosave_changed)
        self.connect(self._db_connect_cb,
                     QtCore.SIGNAL('stateChanged(int)'),
                     self.db_connect_changed)
        self.connect(self._use_cache_cb,
                     QtCore.SIGNAL('stateChanged(int)'),
                     self.use_cache_changed)
        self.connect(self._splash_cb,
                     QtCore.SIGNAL('stateChanged(int)'),
                     self.splash_changed)
        self.connect(self._maximize_cb,
                     QtCore.SIGNAL('stateChanged(int)'),
                     self.maximize_changed)
        self.connect(self._multi_head_cb,
                     QtCore.SIGNAL('stateChanged(int)'),
                     self.multi_head_changed)

    def connect_other_signals(self):
        if core.system.systemType in ['Darwin']:
            self.connect(self._use_metal_style_cb,
                         QtCore.SIGNAL('stateChanged(int)'),
                         self.metalstyle_changed)
        
    def create_default_widgets(self, parent, layout):
        """create_default_widgets(parent: QWidget, layout: QLayout)-> None
        Creates default widgets in parent
        
        """
        parent._autosave_cb = QtGui.QCheckBox(parent)
        parent._autosave_cb.setText('Automatically save vistrails')
        layout.addWidget(parent._autosave_cb)

        parent._db_connect_cb = QtGui.QCheckBox(parent)
        parent._db_connect_cb.setText('Read/Write to database by default')
        layout.addWidget(parent._db_connect_cb)
        
        parent._use_cache_cb = QtGui.QCheckBox(parent)
        parent._use_cache_cb.setText('Cache execution results')
        layout.addWidget(parent._use_cache_cb)

        parent._splash_cb = QtGui.QCheckBox(parent)
        parent._splash_cb.setText('Show splash dialog on startup')
        layout.addWidget(parent._splash_cb)

        parent._maximize_cb = QtGui.QCheckBox(parent)
        parent._maximize_cb.setText('Maximize windows on startup')
        layout.addWidget(parent._maximize_cb)

        parent._multi_head_cb = QtGui.QCheckBox(parent)
        parent._multi_head_cb.setText('Use multiple displays on startup')
        layout.addWidget(parent._multi_head_cb)
        

    def create_other_widgets(self, parent, layout):
        """create_other_widgets(parent: QWidget, layout: QLayout)-> None
        Creates system specific widgets in parent
        
        """
        if core.system.systemType in ['Darwin']:
            parent._use_metal_style_cb = QtGui.QCheckBox(parent)
            parent._use_metal_style_cb.setText('Use brushed metal appearance')
            layout.addWidget(parent._use_metal_style_cb)
        layout.addStretch()

    def update_state(self, configuration):
        """ update_state(configuration: VistrailConfiguration) -> None
        
        Update the dialog state based on a new configuration
        """
        
        self._configuration = configuration

        if self._configuration.has('autosave'):
            self._autosave_cb.setChecked(
                getattr(self._configuration, 'autosave'))
        if self._configuration.has('dbDefault'):
            self._db_connect_cb.setChecked(
                getattr(self._configuration, 'dbDefault'))
        if self._configuration.has('useCache'):
            self._use_cache_cb.setChecked(
                getattr(self._configuration, 'useCache'))
        if self._configuration.has('showSplash'):
            self._splash_cb.setChecked(
                getattr(self._configuration, 'showSplash'))
        if self._configuration.has('maximizeWindows'):
            self._maximize_cb.setChecked(
                getattr(self._configuration, 'maximizeWindows'))
        if self._configuration.has('multiHeads'):
            self._multi_head_cb.setChecked(
                getattr(self._configuration, 'multiHeads'))
        #other widgets
        self.update_other_state()
        
    def update_other_state(self):
        """ update_state(configuration: VistrailConfiguration) -> None
        
        Update the dialog state based on a new configuration
        """
        if core.system.systemType in ['Darwin']:
            self._use_metal_style_cb.setChecked(
                self._configuration.check('useMacBrushedMetalStyle'))
            
    def autosave_changed(self, on):
        """ autosave_changed(on: int) -> None
        
        """
        item = 'autosave'
        setattr(self._configuration, item, bool(on))
        self.emit(QtCore.SIGNAL('configuration_changed'),
                  None, bool(on))

    def db_connect_changed(self, on):
        """ db_connect_changed(on: int) -> None

        """
        item = 'dbDefault'
        setattr(self._configuration, item, bool(on))
        self.emit(QtCore.SIGNAL('configuration_changed'),
                  None, bool(on))

    def use_cache_changed(self, on):
        """ use_cache_changed(on: int) -> None

        """
        item = 'useCache'
        setattr(self._configuration, item, bool(on))
        self.emit(QtCore.SIGNAL('configuration_changed'),
                  None, bool(on))

    def splash_changed(self, on):
        """ splash_changed(on: int) -> None

        """
        item = 'showSplash'
        setattr(self._configuration, item, bool(on))
        self.emit(QtCore.SIGNAL('configuration_changed'),
                  None, bool(on))

    def maximize_changed(self, on):
        """ maximize_changed(on: int) -> None

        """
        item = 'maximizeWindows'
        setattr(self._configuration, item, bool(on))
        self.emit(QtCore.SIGNAL('configuration_changed'),
                  None, bool(on))

    def multi_head_changed(self, on):
        """ multi_head_changed(on: int) -> None

        """
        item = 'multiHeads'
        setattr(self._configuration, item, bool(on))
        self.emit(QtCore.SIGNAL('configuration_changed'),
                  None, bool(on))

    def metalstyle_changed(self, on):
        """ metalstyle_changed(on: int) -> None
        
        """
        item = 'useMacBrushedMetalStyle'
        setattr(self._configuration, item, bool(on))
        self.emit(QtCore.SIGNAL('configuration_changed'),
                  None, bool(on))
