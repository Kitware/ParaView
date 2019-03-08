#/usr/bin/env python

import QtTesting
import sys

libname = 'libGUIMyToolBar.so'
if sys.platform == 'win32':
    libname = 'GUIMyToolBar.dll'

object1 = 'pqClientMainWindow/menubar/menuTools'
QtTesting.playCommand(object1, 'activate', 'actionManage_Plugins')
object2 = "pqClientMainWindow/pqPluginDialog/localGroup/loadLocal"
QtTesting.playCommand(object2, 'activate', '')
object3 = 'pqClientMainWindow/pqPluginDialog/pqFileDialog'
QtTesting.playCommand(object3, 'filesSelected', libname)
object4 = 'pqClientMainWindow/pqPluginDialog/buttonBox/1QPushButton0'
QtTesting.playCommand(object4, 'activate', '')
object5 = 'pqClientMainWindow/MyActions/1QToolButton0'
QtTesting.playCommand(object5, 'activate', '')
object6 = '1QMessageBox0/qt_msgbox_buttonbox/1QPushButton0'
QtTesting.playCommand(object6, 'activate', '')

