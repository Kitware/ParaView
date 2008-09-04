#/usr/bin/env python

import QtTesting
import sys

libname = 'libGUIMyToolBar.so'
if sys.platform == 'win32':
    libname = 'GUIMyToolBar.dll'

if sys.platform == 'darwin':
    libname = 'libGUIMyToolBar.dylib'

object1 = 'MainWindow/menubar/menuTools'
QtTesting.playCommand(object1, 'activate', 'actionManage_Plugins')
object2 = "MainWindow/pqPluginDialog/localGroup/loadLocal"
QtTesting.playCommand(object2, 'activate', '')
object3 = 'MainWindow/pqPluginDialog/pqFileDialog'
QtTesting.playCommand(object3, 'filesSelected', libname)
object4 = 'MainWindow/pqPluginDialog/buttonBox/1QPushButton0'
QtTesting.playCommand(object4, 'activate', '')
object5 = 'MainWindow/MyActions/1QToolButton0'
QtTesting.playCommand(object5, 'activate', '')
object6 = '1QMessageBox0/qt_msgbox_buttonbox/1QPushButton0'
QtTesting.playCommand(object6, 'activate', '')

