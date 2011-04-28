#/usr/bin/env python

#Tests manta client side rendering for both builtin and CS mode
#MantaServerSideTest.py tests server side rendering in CS mode

import QtTesting
import QtTestingImage

#TODO fix test recording so that it actually records these events
#most of which I had to hack in by hand

#load manta plugin on client and server sides
hack1='pqClientMainWindow/menubar'
QtTesting.playCommand(hack1, 'activate', 'menuTools')
hack2='pqClientMainWindow/menubar/menuTools'
QtTesting.playCommand(hack2, 'activate', 'actionManage_Plugins')
object1 = 'pqClientMainWindow/PluginManagerDialog/localGroup/localPlugins'
QtTesting.playCommand(object1, 'setCurrent', 'MantaView')
object2 = 'pqClientMainWindow/PluginManagerDialog/localGroup/loadSelected_Local'
QtTesting.playCommand(object2, 'activate', '')
objecta1 = 'pqClientMainWindow/PluginManagerDialog/remoteGroup/remotePlugins'
QtTesting.playCommand(objecta1, 'setCurrent', 'MantaView')
objecta2 = 'pqClientMainWindow/PluginManagerDialog/remoteGroup/loadSelected_Remote'
QtTesting.playCommand(objecta2, 'activate', '')
object3 = 'pqClientMainWindow/PluginManagerDialog/buttonBox/1QPushButton0'
QtTesting.playCommand(object3, 'activate', '')

#close the 3D view and make a manta view
object4 = 'pqClientMainWindow/centralwidget/MultiViewManager/SplitterFrame/MultiViewSplitter/0/MultiViewFrameMenu/CloseAction'
QtTesting.playCommand(object4, 'activate', '')
object5 = 'pqClientMainWindow/centralwidget/MultiViewManager/SplitterFrame/MultiViewSplitter/0/1QWidget0/1QScrollArea0/qt_scrollarea_viewport/EmptyView/ConvertActionsFrame/Manta'
QtTesting.playCommand(object5, 'activate', '')

#show something
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

# Image comparison
snapshotWidget = 'pqClientMainWindow/centralwidget/MultiViewManager/SplitterFrame/MultiViewSplitter/0/Viewport'
QtTestingImage.compareImage(snapshotWidget, 'MantaImage.png', 300, 300);
