
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
""" This module defines the following classes:
    - QShellDialog
    - QShell

QShell is based on ideas and code from PyCute developed by Gerard Vermeulen.
Used with the author's permission.
More information on PyCute, visit:
http://gerard.vermeulen.free.fr/html/pycute-intro.html

"""
from PyQt4 import QtGui, QtCore
from code import InteractiveInterpreter
import core.system
import copy
import sys
import time
import os.path
import gui.application

################################################################################

class QShellDialog(QtGui.QDialog):
    def __init__(self, parent):
        QtGui.QDialog.__init__(self, parent)
        self.setWindowTitle("VisTrails Shell")
        
        layout = QtGui.QVBoxLayout()
        layout.setSpacing(0)
        layout.setMargin(0)
        self.setLayout(layout)
        #locals() returns the original dictionary, not a copy as
        #the docs say
        app = gui.application.VistrailsApplication
        self.firstLocals = copy.copy(app.get_python_environment())
        self.shell = QShell(self.firstLocals,None)
        self.layout().addWidget(self.shell)
        self.resize(600,400)
        self.createMenu()
        
    
    def createMenu(self):
        """createMenu() -> None
        Creates a menu bar and adds it to the main layout.

        """
        self.newSessionAct = QtGui.QAction(self.tr("&Restart"),self)
        self.newSessionAct.setShortcut(self.tr("Ctrl+R"))
        self.connect(self.newSessionAct, QtCore.SIGNAL("triggered()"),
                     self.newSession)

        self.saveSessionAct = QtGui.QAction(self.tr("&Save"), self)
        self.saveSessionAct.setShortcut(self.tr("Ctrl+S"))
        self.connect(self.saveSessionAct, QtCore.SIGNAL("triggered()"),
                     self.saveSession)

        self.closeSessionAct = QtGui.QAction(self.tr("Close"), self)
        self.closeSessionAct.setShortcut(self.tr("Ctrl+W"))
        self.connect(self.closeSessionAct,QtCore.SIGNAL("triggered()"), 
                     self.closeSession)
        
        self.menuBar = QtGui.QMenuBar(self)
        menu = self.menuBar.addMenu(self.tr("&Session"))
        menu.addAction(self.newSessionAct)
        menu.addAction(self.saveSessionAct)
        menu.addAction(self.closeSessionAct)

        self.layout().setMenuBar(self.menuBar)

    def closeEvent(self, e):
        """closeEvent(e) -> None
        Event handler called when the dialog is about to close."""
        self.closeSession()
        self.emit(QtCore.SIGNAL("shellHidden()"))
    
    def showEvent(self, e):
        """showEvent(e) -> None
        Event handler called when the dialog acquires focus 

        """
        self.shell.show()
        QtGui.QDialog.showEvent(self,e)

    def closeSession(self):
        """closeSession() -> None.
        Hides the dialog instead of closing it, so the session continues open.

        """
        self.hide()

    def newSession(self):
        """newSession() -> None
        Tells the shell to start a new session passing a copy of the original
        locals dictionary.

        """
        self.shell.restart(copy.copy(self.firstLocals))

    def saveSession(self):
        """saveSession() -> None
        Opens a File Save dialog and passes the filename to shell's saveSession.

        """
        default = 'visTrails' + '-' + time.strftime("%Y%m%d-%H%M.log")
        default = os.path.join(core.system.vistrails_file_directory(),default)
        fileName = QtGui.QFileDialog.getSaveFileName(self,
                                                     "Save Session As..",
                                                     default,
                                                     "Log files (*.log)")
        if not fileName:
            return

        self.shell.saveSession(str(fileName))

##############################################################################
# QShell
        
class QShell(QtGui.QTextEdit):
    """This class embeds a python interperter in a QTextEdit Widget
    It is based on PyCute developed by Gerard Vermeulen.
    
    """
    def __init__(self, locals=None, parent=None):
        """Constructor.

        The optional 'locals' argument specifies the dictionary in which code
        will be executed; it defaults to a newly created dictionary with key 
        "__name__" set to "__console__" and key "__doc__" set to None.

        The optional 'log' argument specifies the file in which the interpreter
        session is to be logged.
        
        The optional 'parent' argument specifies the parent widget. If no parent
        widget has been specified, it is possible to exit the interpreter 
        by Ctrl-D.

        """

        QtGui.QTextEdit.__init__(self, parent)
        self.setReadOnly(False)
        
        # to exit the main interpreter by a Ctrl-D if QShell has no parent
        if parent is None:
            self.eofKey = QtCore.Qt.Key_D
        else:
            self.eofKey = None

        self.interpreter = None
        # storing current state
        #this is not working on mac
        #self.prev_stdout = sys.stdout
        #self.prev_stdin = sys.stdin
        #self.prev_stderr = sys.stderr
        # capture all interactive input/output
        #sys.stdout   = self
        #sys.stderr   = self
        #sys.stdin    = self
        
        # user interface setup
        
        self.setAcceptRichText(False)
        self.setWordWrapMode(QtGui.QTextOption.WrapAnywhere)
        
        app = gui.application.VistrailsApplication
        shell_conf = app.configuration.shell
        # font
        font = QtGui.QFont(shell_conf.font_face, shell_conf.font_size)
        font.setFixedPitch(1)
        self.setCurrentFont(font)
        self.reset(locals)

    def reset(self, locals):
        """reset(locals) -> None
        Reset shell preparing it for a new session.
        
        """
        if self.interpreter:
            del self.interpreter
        self.interpreter = InteractiveInterpreter(locals)
 
        # last line + last incomplete lines
        self.line    = QtCore.QString()
        self.lines   = []
        # the cursor position in the last line
        self.point   = 0
        # flag: the interpreter needs more input to run the last lines. 
        self.more    = 0
        # flag: readline() is being used for e.g. raw_input() and input()
        self.reading = 0
        # history
        self.history = []
        self.pointer = 0
        self.last   = 0
                # interpreter prompt.
        if hasattr(sys, "ps1"):
            sys.ps1
        else:
            sys.ps1 = ">>> "
        if hasattr(sys, "ps2"):
            sys.ps2
        else:
            sys.ps2 = "... "
        
        # interpreter banner
        
        self.write('VisTrails shell running Python %s on %s.\n' %
                   (sys.version, sys.platform))
        self.write('Type "copyright", "credits" or "license"'
                   ' for more information on Python.\n')
        self.write(sys.ps1)


    def flush(self):
        """flush() -> None. 
        Simulate stdin, stdout, and stderr.
        
        """
        pass

    def isatty(self):
        """isatty() -> int
        Simulate stdin, stdout, and stderr.
        
        """
        return 1

    def readline(self):
        """readline() -> str
        
        Simulate stdin, stdout, and stderr.
        
        """
        self.reading = 1
        self.__clearLine()
        cursor = self.textCursor()
        cursor.movePosition(QtGui.QTextCursor.End)
        self.setTextCursor(cursor)
      
        while self.reading:
            qApp.processOneEvent()
        if self.line.length() == 0:
            return '\n'
        else:
            return str(self.line) 
    
    def write(self, text):
        """write(text: str) -> None
        Simulate stdin, stdout, and stderr.
        
        """
                
        self.insertPlainText(text)
        cursor = self.textCursor()
        self.last = cursor.position()

    def scroll_bar_at_bottom(self):
        """Returns true if vertical bar exists and is at bottom, or if
        vertical bar does not exist."""
        bar = self.verticalScrollBar()
        if not bar:
            return True
        return bar.value() == bar.maximum()
        
    def __run(self):
        """__run() -> None
        Append the last line to the history list, let the interpreter execute
        the last line(s), and clean up accounting for the interpreter results:
        (1) the interpreter succeeds
        (2) the interpreter fails, finds no errors and wants more line(s)
        (3) the interpreter fails, finds errors and writes them to sys.stderr
        
        """
        should_scroll = self.scroll_bar_at_bottom()
        self.pointer = 0
        self.history.append(QtCore.QString(self.line))
        self.lines.append(str(self.line))
        source = '\n'.join(self.lines)
        self.write('\n')
        self.more = self.interpreter.runsource(source)
        if self.more:
            self.write(sys.ps2)
        else:
            self.write(sys.ps1)
            self.lines = []
        self.__clearLine()
        if should_scroll:
            bar = self.verticalScrollBar()
            if bar:
                bar.setValue(bar.maximum())

    def __clearLine(self):
        """__clearLine() -> None
        Clear input line buffer.
        
        """
        self.line.truncate(0)
        self.point = 0
        
    def __insertText(self, text):
        """__insertText(text) -> None
        Insert text at the current cursor position.
        
        """
        self.insertPlainText(text)
        self.line.insert(self.point, text)
        self.point += text.length()
        
    def keyPressEvent(self, e):
        """keyPressEvent(e) -> None
        Handle user input a key at a time.
        
        """
        text  = e.text()
        key   = e.key()
        if not text.isEmpty():
            ascii = ord(str(text))
        else:
            ascii = -1
        if text.length() and ascii>=32 and ascii<127:
            self.__insertText(text)
            return

        if e.modifiers() & QtCore.Qt.MetaModifier and key == self.eofKey:
            self.parent().closeSession()
        
        if (e.modifiers() & QtCore.Qt.ControlModifier or 
            e.modifiers() & QtCore.Qt.ShiftModifier):
            e.ignore()
            return

        if key == QtCore.Qt.Key_Backspace:
            if self.point:
                QtGui.QTextEdit.keyPressEvent(self, e)
                self.point -= 1
                self.line.remove(self.point, 1)
        elif key == QtCore.Qt.Key_Delete:
            QtGui.QTextEdit.keyPressEvent(self, e)
            self.line.remove(self.point, 1)
        elif key == QtCore.Qt.Key_Return or key == QtCore.Qt.Key_Enter:
            if self.reading:
                self.reading = 0
            else:
                self.__run()
        elif key == QtCore.Qt.Key_Tab:
            self.__insertText(text)
        elif key == QtCore.Qt.Key_Left:
            if self.point:
                QtGui.QTextEdit.keyPressEvent(self, e)
                self.point -= 1
        elif key == QtCore.Qt.Key_Right:
            if self.point < self.line.length():
                QtGui.QTextEdit.keyPressEvent(self, e)
                self.point += 1
        elif key == QtCore.Qt.Key_Home:
            cursor = self.textCursor()
            cursor.movePosition(QtGui.QTextCursor.StartOfLine)
            cursor.setPosition(cursor.position() + 4)
            self.setTextCursor(cursor)
            self.point = 0
        elif key == QtCore.Qt.Key_End:
            QtGui.QTextEdit.keyPressEvent(self, e)
            self.point = self.line.length()
        elif key == QtCore.Qt.Key_Up:
            if len(self.history):
                if self.pointer == 0:
                    self.pointer = len(self.history)
                self.pointer -= 1
                self.__recall()
        elif key == QtCore.Qt.Key_Down:
            if len(self.history):
                self.pointer += 1
                if self.pointer == len(self.history):
                    self.pointer = 0
                self.__recall()
        else:
            e.ignore()

    def __recall(self):
        """__recall() -> None
        Display the current item from the command history.
        
        """
        cursor = self.textCursor()
        cursor.setPosition(self.last)
        
        cursor.select(QtGui.QTextCursor.LineUnderCursor)

        cursor.removeSelectedText()
        self.setTextCursor(cursor)
        self.insertPlainText(sys.ps1)
        self.__clearLine()
        self.__insertText(self.history[self.pointer])

        
    def focusNextPrevChild(self, next):
        """focusNextPrevChild(next) -> None
        Suppress tabbing to the next window in multi-line commands. 
        
        """
        if next and self.more:
            return 0
        return QtGui.QTextEdit.focusNextPrevChild(self, next)

    def mousePressEvent(self, e):
        """mousePressEvent(e) -> None
        Keep the cursor after the last prompt.
        """
        if e.button() == QtCore.Qt.LeftButton:
            cursor = self.textCursor()
            cursor.movePosition(QtGui.QTextCursor.End)
            self.setTextCursor(cursor)
        return

#     def suspend(self):
#         """suspend() -> None
#         Called when hiding the parent window in order to recover the previous
#         state.

#         """
#         #recovering the state
#         sys.stdout   = self.prev_stdout
#         sys.stderr   = self.prev_stderr
#         sys.stdin    = self.prev_stdin

    def show(self):
        """show() -> None
        Store previous state and starts capturing all interactive input and 
        output.
        
        """
#         # storing current state
#         self.prev_stdout = sys.stdout
#         self.prev_stdin = sys.stdin
#         self.prev_stderr = sys.stderr

        # capture all interactive input/output
        sys.stdout   = self
        sys.stderr   = self
        sys.stdin    = self

    def saveSession(self, fileName):
        """saveSession(fileName: str) -> None 
        Write its contents to a file """
        output = open(str(fileName), 'w')
        output.write(self.toPlainText())
        output.close()

    def restart(self, locals=None):
        """restart(locals=None) -> None 
        Restart a new session 

        """
        self.clear()
        self.reset(locals)

    def contentsContextMenuEvent(self,ev):
        """
        contentsContextMenuEvent(ev) -> None
        Suppress the right button context menu.
        
        """
        return
