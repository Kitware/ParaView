#/usr/bin/env python

import QtTesting
import QtTestingImage

#TODO fix test recording so that it actually records the events I had to hack in by hand
hack1='pqClientMainWindow/menubar'
QtTesting.playCommand(hack1, 'activate', 'menuTools')
hack2='pqClientMainWindow/menubar/menuTools'
QtTesting.playCommand(hack2, 'activate', 'actionManage_Plugins')
object1 = 'pqClientMainWindow/PluginManagerDialog/localGroup/localPlugins'
QtTesting.playCommand(object1, 'setCurrent', 'StreamingView')
object2 = 'pqClientMainWindow/PluginManagerDialog/localGroup/loadSelected_Local'
QtTesting.playCommand(object2, 'activate', '')
object3 = 'pqClientMainWindow/PluginManagerDialog/buttonBox/1QPushButton0'
QtTesting.playCommand(object3, 'activate', '')
object4 = 'pqClientMainWindow/centralwidget/MultiViewManager/SplitterFrame/MultiViewSplitter/0/MultiViewFrameMenu/CloseAction'
QtTesting.playCommand(object4, 'activate', '')

#TODO: make view selection by name
#this is hardcoded to the 1st view since the only loaded view plugin goes first
object5 = 'pqClientMainWindow/centralwidget/MultiViewManager/SplitterFrame/MultiViewSplitter/0/1QWidget0/1QScrollArea0/qt_scrollarea_viewport/EmptyView/ConvertActionsFrame/1QPushButton3'

QtTesting.playCommand(object5, 'activate', '')
hack3 = 'pqClientMainWindow/menubar'
QtTesting.playCommand(hack3, 'activate', 'menuSources')
hack4 = 'pqClientMainWindow/menubar/menuSources'
QtTesting.playCommand(hack4, 'activate', 'SphereSource')
object6 = 'pqClientMainWindow/proxyTabDock/proxyTabWidget/qt_tabwidget_stackedwidget/objectInspector/Accept'
QtTesting.playCommand(object6, 'activate', '')
object7 = 'pqClientMainWindow/centralwidget/MultiViewManager/SplitterFrame/MultiViewSplitter/0/Viewport'
QtTesting.playCommand(object7, 'mousePress', '(0.719373,0.369784,1,1,0)')
QtTesting.playCommand(object7, 'mouseMove', '(0.605413,0.271942,1,0,0)')
QtTesting.playCommand(object7, 'mouseRelease', '(0.605413,0.271942,1,0,0)')

# Image compare for default colors.
snapshotWidget = 'pqClientMainWindow/centralwidget/MultiViewManager/SplitterFrame/MultiViewSplitter/0/Viewport'
QtTestingImage.compareImage(snapshotWidget, 'StreamingImage.png', 300, 300);
