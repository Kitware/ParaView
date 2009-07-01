
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
""" This common widgets using on the interface of VisTrails. These are
only simple widgets in term of coding and additional features. It
should have no interaction with VisTrail core"""

from PyQt4 import QtCore, QtGui
from gui.theme import CurrentTheme
from core.modules.constant_configuration import StandardConstantWidget

################################################################################

class QToolWindow(QtGui.QDockWidget):
    """
    QToolWindow is a floating-dockable widget. It also keeps track of
    its widget window title to update the tool window accordingly
    
    """
    def __init__(self, widget=None, parent=None):
        """ QToolWindow(parent: QWidget) -> QToolWindow
        Construct a floating, dockable widget
        
        """
        QtGui.QDockWidget.__init__(self, parent)
        self.setFeatures(QtGui.QDockWidget.AllDockWidgetFeatures)        
        self.setWidget(widget)
        if widget:
            self.setWindowTitle(widget.windowTitle())
        self.monitorWindowTitle(widget)

    def monitorWindowTitle(self, widget):
        """ monitorWindowTitle(widget: QWidget) -> None        
        Watching window title changed on widget and use it as a window
        title on this tool window
        
        """
        if widget:
            widget.installEventFilter(self)

    def eventFilter(self, object, event):
        """ eventFilter(object: QObject, event: QEvent) -> bool
        Filter window title change event to change the tool window title
        
        """
        if event.type()==QtCore.QEvent.WindowTitleChange:
            self.setWindowTitle(object.windowTitle())
        elif event.type()==QtCore.QEvent.Close:
            object.removeEventFilter(self)
        return QtGui.QDockWidget.eventFilter(self, object, event)
        # return super(QToolWindow, self).eventFilter(object, event)

class QToolWindowInterface(object):
    """
    QToolWindowInterface can be co-inherited in any class to allow the
    inherited class to switch to be contained in a window
    
    """
    def toolWindow(self):
        """ toolWindow() -> None        
        Return the tool window and set its parent to self.parent()
        while having self as its contained widget
        
        """
        if not hasattr(self, '_toolWindow'):
            self._toolWindow = QToolWindow(self, self.parent())
        elif self._toolWindow.widget()!=self:
            self._toolWindow.setWidget(self)
        return self._toolWindow

    def changeEvent(self, event):
        """ changeEvent(event: QEvent) -> None        
        Make sure to update the tool parent when to match the widget's
        real parent
        
        """
        if (event.type()==QtCore.QEvent.ParentChange and
            hasattr(self, '_toolWindow')):
            if self.parent()!=self._toolWindow:
                self._toolWindow.setParent(self.parent())

class QDockContainer(QtGui.QMainWindow):
    """
    QDockContainer is a window that can contain dock widgets while
    still be contained in a tool window. It is just a straight
    inheritance from QMainWindow
    
    """
    def __init__(self, parent=None):
        """ QMainWindow(parent: QWidget) -> QMainWindow
        Setup window to have its widget dockable everywhere
        
        """
        QtGui.QMainWindow.__init__(self, parent)
        self.setDockNestingEnabled(True)


class QSearchTreeWidget(QtGui.QTreeWidget):
    """
    QSearchTreeWidget is just a QTreeWidget with a support function to
    refine itself when searching for some text

    """
    def __init__(self, parent=None):
        """ QSearchTreeWidget(parent: QWidget) -> QSearchTreeWidget
        Set up size policy and header

        """
        QtGui.QTreeWidget.__init__(self, parent)
        self.setSizePolicy(QtGui.QSizePolicy.Expanding,
                           QtGui.QSizePolicy.Expanding)
        self.setRootIsDecorated(True)
        self.setDragEnabled(True)
        self.flags = QtCore.Qt.ItemIsDragEnabled
    
    def searchItemName(self, name):
        """ searchItemName(name: QString) -> None        
        Search and refine the module tree widget to contain only items
        whose name is 'name'
        
        """
        matchedItems = []
        
        def recursiveSetVisible(item, testFunction):
            """ recursiveSetVisible
            Pass through all items of a item
            
            """
            enabled = testFunction(item)

            visible = enabled
            child = item.child
            for childIndex in xrange(item.childCount()):
                visible |= recursiveSetVisible(child(childIndex),
                                               testFunction)

            # if item is hidden or has changed visibility
            if not visible or (item.isHidden() != (not visible)):
                item.setHidden(not visible)
            
            if visible:
                f = item.flags()
                b = f & self.flags
                if enabled:
                    if not b:
                        item.setFlags(f | self.flags)
                elif b:
                    item.setFlags(f & ~self.flags)
                
            return visible

        if str(name)=='':
            testFunction = lambda x: True
        else:
            matchedItems = set(self.findItems(name,
                                              QtCore.Qt.MatchContains |
                                              QtCore.Qt.MatchWrap |
                                              QtCore.Qt.MatchRecursive))
            testFunction = matchedItems.__contains__
        for itemIndex in xrange(self.topLevelItemCount()):
            recursiveSetVisible(self.topLevelItem(itemIndex),
                                testFunction)
    
    def mimeData(self, itemList):
        """ mimeData(itemList) -> None        
        Setup the mime data to contain itemList because Qt 4.2.2
        implementation doesn't instantiate QTreeWidgetMimeData
        anywhere as it's supposed to. It must have been a bug...
        
        """
        data = QtGui.QTreeWidget.mimeData(self, itemList)
        data.items = itemList
        return data

    def setMatchedFlags(self, flags):
        """ setMatchedFlags(flags: QItemFlags) -> None Set the flags
        for matched item in the search tree. Parents of matched node
        will be visible with these flags off.
        
        """
        self.flags = flags
    
class QSearchTreeWindow(QtGui.QWidget):
    """
    QSearchTreeWindow contains a search box on top of a tree widget
    for easy search and refine. The subclass has to implement
    createTreeWidget() method to return a tree widget that is also 
    needs to expose searchItemName method

    """
    def __init__(self, parent=None):
        """ QSearchTreeWindow(parent: QWidget) -> QSearchTreeWindow
        Intialize all GUI components
        
        """
        QtGui.QWidget.__init__(self, parent)
        self.setWindowTitle('Search Tree')
        
        vLayout = QtGui.QVBoxLayout(self)
        vLayout.setMargin(0)
        vLayout.setSpacing(0)
        self.setLayout(vLayout)
        
        self.searchBox = QSearchBox(False, self)
        vLayout.addWidget(self.searchBox)

        self.treeWidget = self.createTreeWidget()
        vLayout.addWidget(self.treeWidget)
        
        self.connect(self.searchBox,
                     QtCore.SIGNAL('executeIncrementalSearch(QString)'),
                     self.treeWidget.searchItemName)
        self.connect(self.searchBox,
                     QtCore.SIGNAL('executeSearch(QString)'),
                     self.treeWidget.searchItemName)
        self.connect(self.searchBox,
                     QtCore.SIGNAL('resetSearch()'),
                     self.clearTreeWidget)
                     
    def clearTreeWidget(self):
        """ clearTreeWidget():
        Return the default search tree

        """
        self.treeWidget.searchItemName(QtCore.QString(''))

    def createTreeWidget(self):
        """ createTreeWidget() -> QSearchTreeWidget
        Return a default searchable tree widget
        
        """
        return QSearchTreeWidget(self)

class QPromptWidget(QtGui.QLabel):
    """
    QPromptWidget is a widget that will display a prompt text when it
    doesn't have any child visible, or else, it will disappear. This
    is good for drag and drop prompt. The inheritance should call
    setPromptText and showPrompt in appropriate time to show/hide the
    prompt text
    """
    def __init__(self, parent=None):
        """ QPromptWidget(parent: QWidget) -> QPromptWidget
        Set up the font and alignment for the prompt
        
        """
        QtGui.QLabel.__init__(self, parent)        
        self.setAlignment(QtCore.Qt.AlignHCenter | QtCore.Qt.AlignVCenter)
        self.setWordWrap(True)
        self.regularFont = self.font()
        self.promptFont = QtGui.QFont(self.font())
        self.promptFont.setItalic(True)
        self.promptText = ''
        self.promptVisible = False

    def setPromptText(self, text):
        """ setPromptText(text: str) -> None
        Set the prompt text string
        
        """
        self.promptText = text

    def showPrompt(self, show=True):
        """ showPrompt(show: boolean) -> None
        Show/Hide the prompt
        
        """
        if show!=self.promptVisible:
            self.promptVisible = show
            self.repaint(self.rect())
            
    def showPromptByChildren(self):
        """ showPromptByChildren()
        Show/Hide the prompt based on the current state of children
        
        """
        if self.promptText=='':
            self.showPrompt(False)
        else:
            self.showPrompt(self.layout()==None or
                            self.layout().count()==0)            

    def paintEvent(self, event):
        """ paintEvent(event: QPaintEvent) -> None
        Paint the prompt in the center if neccesary
        
        """
        if self.promptVisible:
            painter = QtGui.QPainter(self)
            painter.setFont(self.promptFont)
            painter.drawText(self.rect(),
                             QtCore.Qt.AlignCenter | QtCore.Qt.TextWordWrap,
                             self.promptText)
            painter.end()
        QtGui.QLabel.paintEvent(self, event)
        # super(QPromptWidget, self).paintEvent(event)

class QStringEdit(QtGui.QFrame):
    """
    QStringEdit is a line edit that has an extra button to allow user
    to use a file as the value
    
    """
    def __init__(self, parent=None):
        """ QStringEdit(parent: QWidget) -> QStringEdit
        Create a hbox layout to contain a line edit and a button

        """
        QtGui.QFrame.__init__(self, parent)        
        hLayout = QtGui.QHBoxLayout(self)
        hLayout.setMargin(0)
        hLayout.setSpacing(0)        
        self.setLayout(hLayout)

        self.lineEdit = QtGui.QLineEdit()
        self.lineEdit.setFrame(False)
        self.lineEdit.setSizePolicy(QtGui.QSizePolicy.Expanding,
                                    QtGui.QSizePolicy.Expanding)
        hLayout.addWidget(self.lineEdit)
        self.setFocusProxy(self.lineEdit)

        self.fileButton = QtGui.QToolButton()
        self.fileButton.setText('...')
        self.fileButton.setSizePolicy(QtGui.QSizePolicy.Maximum,
                                      QtGui.QSizePolicy.Expanding)
        self.fileButton.setFocusPolicy(QtCore.Qt.NoFocus)
        self.fileButton.setAutoFillBackground(True)
        self.connect(self.fileButton, QtCore.SIGNAL('clicked()'),
                     self.insertFileNameDialog)
        hLayout.addWidget(self.fileButton)

    def setText(self, text):
        """ setText(text: QString) -> None
        Overloaded function for setting the line edit text
        
        """
        self.lineEdit.setText(text)

    def text(self):
        """ text() -> QString
        Overloaded function for getting the line edit text
        
        """
        return self.lineEdit.text()

    def selectAll(self):
        """ selectAll() -> None
        Overloaded function for selecting all the text
        
        """
        self.lineEdit.selectAll()

    def setFrame(self, frame):
        """ setFrame(frame: bool) -> None
        Show/Hide the frame of this widget
        
        """
        if frame:
            self.setFrameStyle(QtGui.QFrame.StyledPanel |
                               QtGui.QFrame.Plain)
        else:
            self.setFrameStyle(QtGui.QFrame.NoFrame)

    def insertFileNameDialog(self):
        """ insertFileNameDialog() -> None
        Allow user to insert a file name as a value to the string
        
        """
        fileName = QtGui.QFileDialog.getOpenFileName(self,
                                                     'Use Filename '
                                                     'as Value...',
                                                     self.text(),
                                                     'All files '
                                                     '(*.*)')
        if not fileName.isEmpty():
            self.setText(fileName)
        
class MultiLineWidget(StandardConstantWidget):
     def __init__(self, contents, contentType, parent=None):
        """__init__(contents: str, contentType: str, parent: QWidget) ->
                                             StandardConstantWidget
        Initialize the line edit with its contents. Content type is limited
        to 'int', 'float', and 'string'
        
        """
        StandardConstantWidget.__init__(self, parent)

     def update_parent(self):
         pass
     
     def keyPressEvent(self, event):
         """ keyPressEvent(event) -> None       
         If this is a string line edit, we can use Ctrl+Enter to enter
         the file name          

         """
         k = event.key()
         s = event.modifiers()
         if ((k == QtCore.Qt.Key_Enter or k == QtCore.Qt.Key_Return) and
             s & QtCore.Qt.ShiftModifier):
             event.accept()
             if self.contentIsString and self.multiLines:
                 fileNames = QtGui.QFileDialog.getOpenFileNames(self,
                                                                'Use Filename '
                                                                'as Value...',
                                                                self.text(),
                                                                'All files '
                                                                '(*.*)')
                 fileName = fileNames.join(',')
                 if not fileName.isEmpty():
                     self.setText(fileName)
                     return
         QtGui.QLineEdit.keyPressEvent(self,event)

class QSearchEditBox(QtGui.QComboBox):
    def __init__(self, parent=None):
        QtGui.QComboBox.__init__(self, parent)
        self.setEditable(True)
        self.setInsertPolicy(QtGui.QComboBox.InsertAtTop)
        self.setSizePolicy(QtGui.QSizePolicy.Expanding,
                           QtGui.QSizePolicy.Fixed)
        regexp = QtCore.QRegExp("\S.*")
        self.setDuplicatesEnabled(False)
        validator = QtGui.QRegExpValidator(regexp, self)
        self.setValidator(validator)
        self.addItem('Clear Recent Searches')
        item = self.model().item(0, 0)
        font = QtGui.QFont(item.font())
        font.setItalic(True)
        item.setFont(font)

    def keyPressEvent(self, e):
        if e.key() in (QtCore.Qt.Key_Return,QtCore.Qt.Key_Enter):
            if not self.currentText():
                self.emit(QtCore.SIGNAL('resetText'))
        QtGui.QComboBox.keyPressEvent(self, e)
        
class QSearchBox(QtGui.QWidget):
    """ 
    QSearchBox contains a search combo box with a clear button and
    a search icon.

    """
    def __init__(self, refine=True, parent=None):
        """ QSearchBox(parent: QWidget) -> QSearchBox
        Intialize all GUI components
        
        """
        QtGui.QWidget.__init__(self, parent)
        self.setWindowTitle('Search')
        
        hLayout = QtGui.QHBoxLayout(self)
        hLayout.setMargin(0)
        hLayout.setSpacing(2)
        self.setLayout(hLayout)

        if refine:
            self.actionGroup = QtGui.QActionGroup(self)
            self.searchAction = QtGui.QAction('Search', self)
            self.searchAction.setCheckable(True)
            self.actionGroup.addAction(self.searchAction)
            self.refineAction = QtGui.QAction('Refine', self)
            self.refineAction.setCheckable(True)
            self.actionGroup.addAction(self.refineAction)
            self.searchAction.setChecked(True)

            self.searchMenu = QtGui.QMenu()
            self.searchMenu.addAction(self.searchAction)
            self.searchMenu.addAction(self.refineAction)

            self.searchButton = QtGui.QToolButton(self)
            self.searchButton.setIcon(CurrentTheme.QUERY_ARROW_ICON)
            self.searchButton.setAutoRaise(True)
            self.searchButton.setPopupMode(QtGui.QToolButton.InstantPopup)
            self.searchButton.setMenu(self.searchMenu)
            hLayout.addWidget(self.searchButton)
            self.connect(self.searchAction, QtCore.SIGNAL('triggered()'),
                         self.searchMode)
            self.connect(self.refineAction, QtCore.SIGNAL('triggered()'),
                         self.refineMode)
        else:
            self.searchLabel = QtGui.QLabel(self)
            pix = CurrentTheme.QUERY_VIEW_ICON.pixmap(QtCore.QSize(16,16))
            self.searchLabel.setPixmap(pix)
            self.searchLabel.setAlignment(QtCore.Qt.AlignCenter)
            self.searchLabel.setMargin(4)
            hLayout.addWidget(self.searchLabel)
        
        self.searchEdit = QSearchEditBox(self)
        self.setFocusProxy(self.searchEdit)
        #TODO: Add separator!
        self.searchEdit.clearEditText()
        hLayout.addWidget(self.searchEdit)

        self.resetButton = QtGui.QToolButton(self)
        self.resetButton.setIcon(QtGui.QIcon(
                self.style().standardPixmap(QtGui.QStyle.SP_DialogCloseButton)))
        self.resetButton.setIconSize(QtCore.QSize(12,12))
        self.resetButton.setAutoRaise(True)
        self.resetButton.setEnabled(False)
        hLayout.addWidget(self.resetButton)

        self.connect(self.resetButton, QtCore.SIGNAL('clicked()'),
                     self.resetSearch)
        self.connect(self.searchEdit, QtCore.SIGNAL('editTextChanged(QString)'),
                     self.executeIncrementalSearch)
        self.connect(self.searchEdit, QtCore.SIGNAL('activated(int)'),
                     self.executeSearch)
        self.connect(self.searchEdit, QtCore.SIGNAL('resetText'),
                     self.resetSearch)

    def resetSearch(self):
        """
        resetSearch() -> None
        Emit a signal to clear the search.

        """
        self.searchEdit.clearEditText()
        self.resetButton.setEnabled(False)
        self.emit(QtCore.SIGNAL('resetSearch()'))

    def clearSearch(self):
        """ clearSearch() -> None
        Clear the edit text without emitting resetSearch() signal
        This is for when the search is rest from the version view and
        the signal are already taken care of

        """
        self.searchEdit.clearEditText()
        self.resetButton.setEnabled(False)

    def searchMode(self):
        """
        searchMode() -> None

        """
        self.emit(QtCore.SIGNAL('refineMode(bool)'), False) 
    
    def refineMode(self):
        """
        refineMode() -> None

        """
        self.emit(QtCore.SIGNAL('refineMode(bool)'), True) 

    def executeIncrementalSearch(self, text):
        """
        executeIncrementalSearch(text: QString) -> None
        The text is changing, so update the search.

        """
        self.resetButton.setEnabled(str(text)!='')
        self.emit(QtCore.SIGNAL('executeIncrementalSearch(QString)'), text)

    def executeSearch(self, index):
        """
        executeSearch(index: int) -> None
        The text is finished changing or a different item was selected.

        """
        count = self.searchEdit.count() 
        if index == count-1: 
            for i in xrange(count-1): 
                self.searchEdit.removeItem(0) 
            self.resetSearch() 
        else: 
            self.resetButton.setEnabled(True) 
            self.emit(QtCore.SIGNAL('executeSearch(QString)'),  
                      self.searchEdit.currentText())

