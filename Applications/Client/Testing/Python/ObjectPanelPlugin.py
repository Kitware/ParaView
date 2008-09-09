#/usr/bin/env python

import QtTesting
import sys

libname = 'libGUIConePanel.so'
if sys.platform == 'win32':
    libname = 'GUIConePanel.dll'

if sys.platform == 'darwin':
    libname = 'libGUIConePanel.dylib'

object1 = 'MainWindow/menubar/menuTools'
QtTesting.playCommand(object1, 'activate', 'actionManage_Plugins')
object2 = 'MainWindow/pqPluginDialog/localGroup/loadLocal'
QtTesting.playCommand(object2, 'activate', '')
object3 = 'MainWindow/pqPluginDialog/pqFileDialog'
QtTesting.playCommand(object3, 'filesSelected', libname)
object4 = 'MainWindow/pqPluginDialog/buttonBox/1QPushButton0'
QtTesting.playCommand(object4, 'activate', '')
object5 = 'MainWindow/menubar/menuSources'
QtTesting.playCommand(object5, 'activate', 'Cone')
object7 = 'MainWindow/objectInspectorDock/1pqProxyTabWidget0/qt_tabwidget_stackedwidget/objectInspector/Accept'
QtTesting.playCommand(object7, 'activate', '')

object8 = 'MainWindow/objectInspectorDock/1pqProxyTabWidget0/qt_tabwidget_stackedwidget/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/1QLabel0'

text = QtTesting.getProperty(object8, 'text')
print text

