#/usr/bin/env python

import QtTesting
import sys

libname = 'libGUIConePanel.so'
if sys.platform == 'win32':
    libname = 'GUIConePanel.dll'

if sys.platform == 'darwin':
    libname = 'libGUIConePanel.dylib'

object1 = 'pqClientMainWindow/menubar/menuTools'
QtTesting.playCommand(object1, 'activate', 'actionManage_Plugins')
object2 = 'pqClientMainWindow/pqPluginDialog/localGroup/loadLocal'
QtTesting.playCommand(object2, 'activate', '')
object3 = 'pqClientMainWindow/pqPluginDialog/pqFileDialog'
QtTesting.playCommand(object3, 'filesSelected', libname)
object4 = 'pqClientMainWindow/pqPluginDialog/buttonBox/1QPushButton0'
QtTesting.playCommand(object4, 'activate', '')
object5 = 'pqClientMainWindow/menubar/menuSources'
QtTesting.playCommand(object5, 'activate', 'Cone')
object7 = 'pqClientMainWindow/objectInspectorDock/objectInspector/Accept'
QtTesting.playCommand(object7, 'activate', '')

object8 = 'pqClientMainWindow/objectInspectorDock/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/1QLabel0'

text = QtTesting.getProperty(object8, 'text')
print text

