#/usr/bin/env python

import QtTesting
import sys

print 'start'

libname = 'libGUIMyToolBar.so'
if sys.platform == 'win32':
    libname = 'GUIMyToolBar.dll'

if sys.platform == 'darwin':
    libname = 'libGUIMyToolBar.dylib'

print 'libname = ' + libname

object1 = 'MainWindow/menubar/menuTools'
QtTesting.playCommand(object1, 'activate', 'actionManage_Plugins')
print 'played1'
object2 = 'MainWindow/pqPluginDialog/clientGroup/loadClient'
QtTesting.playCommand(object2, 'activate', '')
print 'played2'
object3 = 'MainWindow/pqPluginDialog/pqFileDialog'
QtTesting.playCommand(object3, 'filesSelected', libname)
print 'played3'
object4 = 'MainWindow/pqPluginDialog/buttonBox/1QPushButton0'
QtTesting.playCommand(object4, 'activate', '')
print 'played4'
object5 = 'MainWindow/MyActions/1QToolButton0'
QtTesting.playCommand(object5, 'activate', '')
print 'played4'
object6 = '1QMessageBox0/qt_msgbox_buttonbox/1QPushButton0'
QtTesting.playCommand(object6, 'activate', '')

print 'end'

