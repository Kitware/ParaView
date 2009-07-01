
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
""" This holds a customized QTabWidget for controlling different
vistrail views and tools

QViewManager
"""

from PyQt4 import QtCore, QtGui
from gui.theme import CurrentTheme
from gui.utils import getBuilderWindow
from gui.vistrail_view import QVistrailView
from core import system
from core.db.locator import FileLocator, untitled_locator
from core.log.log import Log
import core.system
from core.vistrail.pipeline import Pipeline
from core.vistrail.vistrail import Vistrail
from core.modules.module_registry import ModuleRegistry
import copy

################################################################################

class QViewManager(QtGui.QTabWidget):
    """
    QViewManager is a tabbed widget to containing multiple Vistrail
    views. It takes care of emiting useful signals to the builder
    window
    
    """
    def __init__(self, parent=None):
        """ QViewManager(view: QVistrailView) -> QViewManager
        Create an empty tab widget
        
        """
        QtGui.QTabWidget.__init__(self, parent)
        self.closeButton = QtGui.QToolButton(self)
        self.closeButton.setIcon(CurrentTheme.VIEW_MANAGER_CLOSE_ICON)
        self.closeButton.setAutoRaise(True)
        self.setCornerWidget(self.closeButton)
        self.splittedViews = {}
        self.activeIndex = -1
        
        self.connect(self, QtCore.SIGNAL('currentChanged(int)'),
                     self.currentChanged)
        self.connect(self.closeButton, QtCore.SIGNAL('clicked()'),
                     self.closeVistrail)
        
        self._views = {}
        self.single_document_mode = False
        
    def set_single_document_mode(self, on):
        """ set_single_document_mode(on: bool) -> None
        Toggle single document interface
        
        """
        self.single_document_mode = on
        if self.single_document_mode:
            self.tabBar().hide()
        else:
            self.tabBar().show()
        

    def add_vistrail_view(self, view):
        """ add_vistrail_view(view: QVistrailView) -> None
        Add a vistrail view to the tab, and connect to the right signals
        
        """
        self._views[view] = view.controller
        if self.indexOf(view)!=-1:
            return
        
        view.installEventFilter(self)
        self.connect(view.pipelineTab,
                     QtCore.SIGNAL('moduleSelectionChange'),
                     self.moduleSelectionChange)
        self.connect(view,
                     QtCore.SIGNAL('versionSelectionChange'),
                     self.versionSelectionChange)
        self.connect(view,
                     QtCore.SIGNAL('execStateChange()'),
                     self.execStateChange)
        self.connect(view.versionTab,
                     QtCore.SIGNAL('vistrailChanged()'),
                     self.vistrailChanged)
        self.connect(view.pipelineTab,
                     QtCore.SIGNAL('resetQuery()'),
                     self.resetQuery)
        self.connect(view.versionTab,
                     QtCore.SIGNAL('resetQuery()'),
                     self.resetQuery)
        self.connect(view.queryTab,
                     QtCore.SIGNAL('resetQuery()'),
                     self.resetQuery)

        self.emit(QtCore.SIGNAL('vistrailViewAdded'), view)
        self.addTab(view, view.windowTitle())
        if self.count()==1:
            self.emit(QtCore.SIGNAL('currentChanged(int)'), 0)

    def removeVistrailView(self, view):
        """ removeVistrailView(view: QVistrailView) -> None
        Remove the current vistrail view and destroy it
        
        """
        if view:
            del self._views[view]
            view.removeEventFilter(self)
            self.disconnect(view.pipelineTab,
                            QtCore.SIGNAL('moduleSelectionChange'),
                            self.moduleSelectionChange)
            self.disconnect(view,
                            QtCore.SIGNAL('versionSelectionChange'),
                            self.versionSelectionChange)
            self.disconnect(view.versionTab,
                            QtCore.SIGNAL('vistrailChanged()'),
                            self.vistrailChanged)
            self.emit(QtCore.SIGNAL('vistrailViewRemoved'), view)
            index = self.indexOf(view) 
            if index !=-1:
                self.removeTab(self.currentIndex())
                self.activeIndex = self.currentIndex()
            elif self.splittedViews.has_key(view):
                del self.splittedViews[view]
            view.controller.cleanup()
            view.close()
            view.deleteLater()

    def moduleSelectionChange(self, selection):
        """ moduleSelectionChange(selection: list[id]) -> None
        Just echo the signal from the view
        
        """
        self.emit(QtCore.SIGNAL('moduleSelectionChange'), selection)

    def versionSelectionChange(self, versionId):
        """ versionSelectionChange(versionId: int) -> None
        Just echo the signal from the view
        
        """
        self.emit(QtCore.SIGNAL('versionSelectionChange'), versionId)

    def execStateChange(self):
        """ execStateChange() -> None
        Just echo the signal from the view

        """
        self.emit(QtCore.SIGNAL('execStateChange()'))

    def vistrailChanged(self):
        """ vistrailChanged() -> None
        Just echo the signal from the view
        
        """
        self.emit(QtCore.SIGNAL('vistrailChanged()'))

    def copySelection(self):
        """ copySelection() -> None
        Copy the current selected pipeline modules
        
        """
        vistrailView = self.currentWidget()
        if vistrailView:
            vistrailView.pipelineTab.pipelineView.scene().copySelection()

    def group(self):
        """group() -> None
        Creates a group from the selected pipeline modules
        
        """
        vistrailView = self.currentWidget()
        if vistrailView:
            vistrailView.pipelineTab.pipelineView.scene().group()

    def ungroup(self):
        """ungroup() -> None
        Ungroups selected pipeline modules
        
        """
        vistrailView = self.currentWidget()
        if vistrailView:
            vistrailView.pipelineTab.pipelineView.scene().ungroup()

    def currentView(self):
        """currentView() -> VistrailView. Returns the current vistrail view."""
        return self.currentWidget()

    def pasteToCurrentPipeline(self):
        """ pasteToCurrentPipeline() -> None
        Paste what is on the clipboard to the current pipeline
        
        """        
        vistrailView = self.currentWidget()
        if vistrailView:
            vistrailView.pipelineTab.pipelineView.scene().pasteFromClipboard()

    def selectAllModules(self):
        """ selectAllModules() -> None
        Select all modules in the current view
        
        """
        vistrailView = self.currentWidget()
        if vistrailView:
            vistrailView.pipelineTab.pipelineView.scene().selectAll()

    def canSelectAll(self):
        """ canSelectAll() -> bool        
        Check to see if there is any module in the pipeline view to be
        selected
        
        """
        vistrailView = self.currentWidget()
        if vistrailView and vistrailView.controller.current_pipeline:
            return len(vistrailView.controller.current_pipeline.modules)>0
        return False

    def redo(self):
        """ redo() -> none
        Performs a redo step.

        """
        vistrailView = self.currentWidget()
        if not vistrailView:
            return
        new_version = vistrailView.redo()
        self.emit(QtCore.SIGNAL('versionSelectionChange'), new_version)

    def undo(self):
        """ undo() -> None
        Performs an undo step.

        """
        vistrailView = self.currentWidget()
        if not vistrailView:
            return
        new_version = vistrailView.undo()
        self.emit(QtCore.SIGNAL('versionSelectionChange'), new_version)

    def newVistrail(self, recover_files=True):
        """ newVistrail() -> (None or QVistrailView)
        Create a new vistrail with no name. If user cancels process,
        returns None.

        FIXME: We should do the interactive parts separately.
        
        """
        if self.single_document_mode and self.currentView():
            if not self.closeVistrail():
                return None
        if recover_files and untitled_locator().has_temporaries():
            locator = copy.copy(untitled_locator())
            vistrail = locator.load()
        else:
            locator = None
            vistrail = Vistrail()
        return self.set_vistrail_view(vistrail, locator)

    def close_first_vistrail_if_necessary(self):
        # Close first vistrail of no change was made
        if not self._first_view:
            return
        vt = self._first_view.controller.vistrail
        if vt.get_version_count() == 0:
            self.closeVistrail(self._first_view)
            self._first_view = None
        else:
            # We set it to none, since it's been changed, so
            # we don't want to ever close it again.
            self._first_view = None

    def set_vistrail_view(self, vistrail,locator, version=None):
        """set_vistrail_view(vistrail: Vistrail,
                             locator: VistrailLocator,
                             version=None)
                          -> QVistrailView
        Sets a new vistrail view for the vistrail object for the given version
        if version is None, use the latest version
        """
        vistrailView = QVistrailView()
        vistrailView.set_vistrail(vistrail, locator)
        self.add_vistrail_view(vistrailView)
        self.setCurrentWidget(vistrailView)
        vistrailView.controller.inspectAndImportModules()
        if version is None:
            version = vistrail.get_latest_version()
        vistrailView.setup_view(version)
        self.versionSelectionChange(version)
        vistrailView.versionTab.vistrailChanged()
        return vistrailView

    def open_vistrail(self, locator, version=None, quiet=False):
        """open_vistrail(locator: Locator, version = None: int or str)

        opens a new vistrail from the given locator, selecting the
        given version.

        """
        self.close_first_vistrail_if_necessary()
        if self.single_document_mode and self.currentView():
            self.closeVistrail(None,quiet)
        view = self.ensureVistrail(locator)
        if view:
            return view
        try:
            vistrail = locator.load(Vistrail)
            if type(version) == str:
                version = vistrail.get_version_number(version)
            result = self.set_vistrail_view(vistrail, locator, version)
            return result
        except ModuleRegistry.MissingModulePackage, e:
            QtGui.QMessageBox.critical(self,
                                       'Missing package',
                                       (('Cannot find module "%s" in \n' % e._name) +
                                        ('package "%s". Make sure package is \n' % e._identifier) +
                                        'enabled in the Preferences dialog.'))
        except Exception, e:
            QtGui.QMessageBox.critical(None,
                                       'Vistrails',
                                       str(e))
            self.newVistrail()

    def save_vistrail(self, locator_class,
                      vistrailView=None,
                      force_choose_locator=False):
        """

        force_choose_locator=True triggers 'save as' behavior
        """
        if not vistrailView:
            vistrailView = self.currentWidget()
        vistrailView.flush_changes()
        
        if vistrailView:
            gui_get = locator_class.save_from_gui
            # get a locator to write to
            if force_choose_locator:
                locator = gui_get(self, Vistrail.vtType,
                                  vistrailView.controller.locator)
            else:
                locator = (vistrailView.controller.locator or
                           gui_get(self, Vistrail.vtType,
                                   vistrailView.controller.locator))
            if locator == untitled_locator():
                locator = gui_get(self, Vistrail.vtType,
                                  vistrailView.controller.locator)
            # if couldn't get one, ignore the request
            if not locator:
                return False
            try:
                vistrailView.controller.write_vistrail(locator)
            except Exception, e:
                QtGui.QMessageBox.critical(None,
                                           'Vistrails',
                                           str(e))
                return False
            return True
   
    def save_workflow(self, locator_class, force_choose_locator=True):
        vistrailView = self.currentWidget()

        if vistrailView:
            vistrailView.flush_changes()
            gui_get = locator_class.save_from_gui
            if force_choose_locator:
                locator = gui_get(self, Pipeline.vtType,
                                  vistrailView.controller.locator)
            else:
                locator = (vistrailView.controller.locator or
                           gui_get(self, Pipeline.vtType,
                                   vistrailView.controller.locator))
            if locator == untitled_locator():
                locator = gui_get(self, Pipeline.vtType,
                                  vistrailView.controller.locator)
            if not locator:
                return False
            vistrailView.controller.write_workflow(locator)
            return True

    def save_log(self, locator_class, force_choose_locator=True):
        vistrailView = self.currentWidget()

        if vistrailView:
            vistrailView.flush_changes()
            gui_get = locator_class.save_from_gui
            if force_choose_locator:
                locator = gui_get(self, Log.vtType,
                                  vistrailView.controller.locator)
            else:
                locator = (vistrailView.controller.locator or
                           gui_get(self, Log.vtType,
                                   vistrailView.controller.locator))
            if locator == untitled_locator():
                locator = gui_get(self, Log.vtType,
                                  vistrailView.controller.locator)
            if not locator:
                return False
            vistrailView.controller.write_log(locator)
            return True

    def save_tree_to_pdf(self):
        vistrailView = self.currentWidget()
        fileName = QtGui.QFileDialog.getSaveFileName(
            self,
            "Save PDF...",
            core.system.vistrails_file_directory(),
            "PDF files (*.pdf)",
            None)

        if fileName.isEmpty():
            return None
        f = str(fileName)
        vistrailView.versionTab.versionView.scene().saveToPDF(f)
        
    def save_workflow_to_pdf(self):
        vistrailView = self.currentWidget()
        fileName = QtGui.QFileDialog.getSaveFileName(
            self,
            "Save PDF...",
            core.system.vistrails_file_directory(),
            "PDF files (*.pdf)",
            None)

        if fileName.isEmpty():
            return None
        f = str(fileName)
        vistrailView.pipelineTab.pipelineView.scene().saveToPDF(f)
        
             
    def closeVistrail(self, vistrailView=None, quiet=False):
        """ closeVistrail(vistrailView: QVistrailView, quiet: bool) -> bool
        Close the current active vistrail
        
        """
        if not vistrailView:
            vistrailView = self.currentWidget()
        vistrailView.flush_changes()

        if vistrailView:
            if not quiet and vistrailView.controller.changed:
                text = vistrailView.controller.name
                if text=='':
                    text = 'Untitled%s'%core.system.vistrails_default_file_type()
                text = ('Vistrail ' +
                        QtCore.Qt.escape(text) +
                        ' contains unsaved changes.\n Do you want to '
                        'save changes before closing it?')
                res = QtGui.QMessageBox.information(getBuilderWindow(),
                                                    'Vistrails',
                                                    text, 
                                                    '&Save', 
                                                    '&Discard',
                                                    'Cancel',
                                                    0,
                                                    2)
            else:
                res = 1
            if res == 0:
                locator = vistrailView.controller.locator
                if locator is None:
                    class_ = FileLocator()
                else:
                    class_ = type(locator)
                return self.save_vistrail(class_)
            elif res == 2:
                return False
            self.removeVistrailView(vistrailView)
            if self.count()==0:
                self.emit(QtCore.SIGNAL('currentVistrailChanged'), None)
                self.emit(QtCore.SIGNAL('versionSelectionChange'), -1)
        if vistrailView == self._first_view:
            self._first_view = None
        return True
    
    def closeAllVistrails(self, quiet=False):
        """ closeAllVistrails() -> bool        
        Attemps to close every single vistrail, return True if
        everything is closed correctly
        
        """
        for view in self.splittedViews.keys():
            if not self.closeVistrail(view,quiet):
                return False
        while self.count()>0:
            if not self.closeVistrail(None,quiet):
                return False
        return True

    def currentChanged(self, index):
        """ currentChanged(index: int):        
        Emit signal saying a different vistrail has been chosen to the
        builder
        
        """
        self.activeIndex = index
        self.emit(QtCore.SIGNAL('currentVistrailChanged'),
                  self.currentWidget())
        if index >= 0:
            self.emit(QtCore.SIGNAL('versionSelectionChange'), 
                      self.currentWidget().controller.current_version)
        else:
            self.emit(QtCore.SIGNAL('versionSelectionChange'), 
                      -1)
        
    def eventFilter(self, object, event):
        """ eventFilter(object: QVistrailView, event: QEvent) -> None
        Filter the window title change event for the view widget
        
        """
        if event.type()==QtCore.QEvent.WindowTitleChange:
            if object==self.currentWidget():
                self.setTabText(self.currentIndex(), object.windowTitle())
                self.currentChanged(self.currentIndex())
        return QtGui.QTabWidget.eventFilter(self, object, event)

    def getCurrentVistrailFileName(self):
        """ getCurrentVistrailFileName() -> str        
        Return the filename of the current vistrail or None if it
        doesn't have one
        
        """        
        vistrailView = self.currentWidget()
        if vistrailView and vistrailView.controller.name!='':
            return vistrailView.controller.name
        else:
            return None

    def setPIPMode(self, on):
        """ setPIPMode(on: Bool) -> None
        Set the picture-in-picture mode for all views
        
        """
        for viewIndex in xrange(self.count()):
            vistrailView = self.widget(viewIndex)
            vistrailView.setPIPMode(on)

    def setMethodsMode(self, on):
        """ setMethodsMode(on: Bool) -> None
        Turn the methods panel on/off for all views
        
        """
        for viewIndex in xrange(self.count()):
            vistrailView = self.widget(viewIndex)
            vistrailView.setMethodsMode(on)


    def setSetMethodsMode(self, on):
        """ setSetMethodsMode(on: Bool) -> None
        Turn the set methods panel on/off for all views
        
        """
        for viewIndex in xrange(self.count()):
            vistrailView = self.widget(viewIndex)
            vistrailView.setSetMethodsMode(on)

    def setPropertiesMode(self, on):
        """ setPropertiesMode(on: Bool) -> None
        Turn the properties panel on/off for all views
        
        """
        for viewIndex in xrange(self.count()):
            vistrailView = self.widget(viewIndex)
            vistrailView.setPropertiesMode(on)

    def setPropertiesOverlayMode(self, on):
        """ setPropertiesOverlayMode(on: Bool) -> None
        Turn the properties overlay panel on/off for all views
        
        """
        for viewIndex in xrange(self.count()):
            vistrailView = self.widget(viewIndex)
            vistrailView.setPropertiesOverlayMode(on)

    def ensureVistrail(self, locator):
        """ ensureVistrail(locator: VistrailLocator) -> QVistrailView        
        This will first find among the opened vistrails to see if
        vistrails from locator has been opened. If not, it will return None.
        
        """
        for view in self.splittedViews.keys():
            if view.controller.vistrail.locator == locator:
                self.setCurrentWidget(view)
                return view
        for i in xrange(self.count()):
            view = self.widget(i)
            if view.controller.vistrail.locator == locator:
                self.setCurrentWidget(view)
                return view
        return None
    
    def set_first_view(self, view):
        self._first_view = view

    def viewModeChanged(self, mode):
        """ viewModeChanged(mode: Int) -> None
        
        """
        for viewIndex in xrange(self.count()):            
            vistrailView = self.widget(viewIndex)
            vistrailView.viewModeChanged(mode)
    
    def changeCursor(self, mode):
        """ changeCursor(mode: Int) -> None
        
        """
        for viewIndex in xrange(self.count()):            
            vistrailView = self.widget(viewIndex)
            vistrailView.updateCursorState(mode)            
        
    def resetQuery(self):
        """ resetQwuery() -> None
        
        """
        self.queryVistrail(False)

    def queryVistrail(self, on=True):
        """ queryVistrail(on: bool) -> None
        
        """
        self.currentView().setFocus(QtCore.Qt.MouseFocusReason)
        self.currentView().queryVistrail(on)

    def executeCurrentPipeline(self):
        """ executeCurrentPipeline() -> None
        
        """
        self.currentView().setFocus(QtCore.Qt.MouseFocusReason)
        self.currentView().controller.execute_current_workflow()

    def executeCurrentExploration(self):
        """ executeCurrentExploration() -> None
        
        """
        self.currentView().setFocus(QtCore.Qt.MouseFocusReason)
        self.currentView().executeParameterExploration()
