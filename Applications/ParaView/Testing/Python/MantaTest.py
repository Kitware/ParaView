#/usr/bin/env python

#Tests manta client side rendering for both builtin and CS mode
#MantaServerSideTest.py tests server side rendering in CS mode

import QtTesting
import QtTestingImage

#load manta plugin on client and server sides
object1='pqClientMainWindow/menubar'
QtTesting.playCommand(object1, 'activate', 'menuTools')
hack2='pqClientMainWindow/menubar/menuTools'
QtTesting.playCommand(hack2, 'activate', 'actionManage_Plugins')
objectA = 'pqClientMainWindow/PluginManagerDialog/localGroup/localPlugins'
QtTesting.playCommand(objectA, 'setCurrent', 'MantaView')
object2 = 'pqClientMainWindow/PluginManagerDialog/localGroup/loadSelected_Local'
QtTesting.playCommand(object2, 'activate', '')
objecta1 = 'pqClientMainWindow/PluginManagerDialog/remoteGroup/remotePlugins'
QtTesting.playCommand(objecta1, 'setCurrent', 'MantaView')
objecta2 = 'pqClientMainWindow/PluginManagerDialog/remoteGroup/loadSelected_Remote'
QtTesting.playCommand(objecta2, 'activate', '')
object3 = 'pqClientMainWindow/PluginManagerDialog/buttonBox/1QPushButton0'
QtTesting.playCommand(object3, 'activate', '')

#create two manta and one gl window
object6 = 'pqClientMainWindow/centralwidget/MultiViewManager/SplitterFrame/MultiViewSplitter/0/MultiViewFrameMenu/CloseAction'
QtTesting.playCommand(object6, 'activate', '')
object7 = 'pqClientMainWindow/centralwidget/MultiViewManager/SplitterFrame/MultiViewSplitter/0/1QWidget0/1QScrollArea0/qt_scrollarea_viewport/CentralWidgetFrame/EmptyView/ConvertActionsFrame/Manta'
QtTesting.playCommand(object7, 'activate', '')
object8 = 'pqClientMainWindow/centralwidget/MultiViewManager/SplitterFrame/MultiViewSplitter/0/MultiViewFrameMenu/SplitHorizontalAction'
QtTesting.playCommand(object8, 'activate', '')
object9 = 'pqClientMainWindow/centralwidget/MultiViewManager/SplitterFrame/MultiViewSplitter/1/MultiViewFrameMenu/SplitHorizontalAction'
QtTesting.playCommand(object9, 'activate', '')
QtTesting.playCommand(object8, 'activate', '')
object10 = 'pqClientMainWindow/centralwidget/MultiViewManager/SplitterFrame/MultiViewSplitter/1/MultiViewFrameMenu/CloseAction'
QtTesting.playCommand(object10, 'activate', '')
object11 = 'pqClientMainWindow/centralwidget/MultiViewManager/SplitterFrame/MultiViewSplitter/1/1QWidget0/1QScrollArea0/qt_scrollarea_viewport/CentralWidgetFrame/EmptyView/ConvertActionsFrame/Manta'
QtTesting.playCommand(object11, 'activate', '')
object12 = 'pqClientMainWindow/centralwidget/MultiViewManager/SplitterFrame/MultiViewSplitter/2/1QWidget0/1QScrollArea0/qt_scrollarea_viewport/CentralWidgetFrame/EmptyView/ConvertActionsFrame/3D View'
QtTesting.playCommand(object12, 'activate', '')

#create some geometry to show
QtTesting.playCommand(object1, 'activate', 'menuSources')
object13 = 'pqClientMainWindow/menubar/menuSources'
QtTesting.playCommand(object13, 'activate', 'SphereSource')
object14 = 'pqClientMainWindow/objectInspectorDock/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/ThetaResolution'
QtTesting.playCommand(object14, 'key', '16777219')
QtTesting.playCommand(object14, 'set_string', '20')
QtTesting.playCommand(object14, 'set_string', '20')
object15 = 'pqClientMainWindow/objectInspectorDock/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/StartTheta/LineEdit'
QtTesting.playCommand(object15, 'key', '16777217')
object16 = 'pqClientMainWindow/objectInspectorDock/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/EndTheta/LineEdit'
QtTesting.playCommand(object16, 'key', '16777217')
object17 = 'pqClientMainWindow/objectInspectorDock/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/PhiResolution'
QtTesting.playCommand(object17, 'key', '16777217')
QtTesting.playCommand(object17, 'set_string', '20')
QtTesting.playCommand(object17, 'set_string', '20')
object18 = 'pqClientMainWindow/objectInspectorDock/objectInspector/Accept'
QtTesting.playCommand(object18, 'activate', '')

#ascribe some data values to test color mapping
object19 = 'pqClientMainWindow/menubar/menuFilters/pqProxyGroupMenuManager0/Calculator'
QtTesting.playCommand(object19, 'activate', '')
object20 = 'pqClientMainWindow/objectInspectorDock/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/Function'
QtTesting.playCommand(object20, 'set_string', 'coordsX')
QtTesting.playCommand(object18, 'activate', '')

#make it visible in all three views
object21 = 'pqClientMainWindow/centralwidget/MultiViewManager/SplitterFrame/MultiViewSplitter/1/Viewport'
QtTesting.playCommand(object21, 'mousePress', '(0.474654,0.393281,1,1,0)')
QtTesting.playCommand(object21, 'mouseMove', '(0.474654,0.393281,1,0,0)')
QtTesting.playCommand(object21, 'mouseRelease', '(0.474654,0.393281,1,0,0)')
object22 = 'pqClientMainWindow/pipelineBrowserDock/pipelineBrowser'
QtTesting.playCommand(object22, 'mousePress', '1,1,0,15,11,/0:0/0:0/0:1')
QtTesting.playCommand(object22, 'mouseRelease', '1,0,0,15,11,/0:0/0:0/0:1')
object27 = 'pqClientMainWindow/centralwidget/MultiViewWidget/CoreWidget/qt_tabwidget_stackedwidget/MultiViewWidget1/Frame.0/Viewport'
QtTesting.playCommand(object27, 'mousePress', '(0.456221,0.411067,1,1,0)')
QtTesting.playCommand(object27, 'mouseMove', '(0.456221,0.411067,1,0,0)')
QtTesting.playCommand(object27, 'mouseRelease', '(0.456221,0.411067,1,0,0)')
QtTesting.playCommand(object22, 'mousePress', '1,1,0,11,10,/0:0/0:0/0:1')
QtTesting.playCommand(object22, 'mouseRelease', '1,0,0,11,10,/0:0/0:0/0:1')
QtTesting.playCommand(object27, 'mousePress', '(0.806452,0.290514,1,1,67108864)')
QtTesting.playCommand(object27, 'mouseMove', '(0.806452,0.290514,1,0,67108864)')
QtTesting.playCommand(object27, 'mouseRelease', '(0.806452,0.290514,1,0,67108864)')
QtTesting.playCommand(object27, 'mousePress', '(0.806452,0.290514,1,1,67108864)')
QtTesting.playCommand(object27, 'mouseMove', '(0.806452,0.290514,1,0,67108864)')
QtTesting.playCommand(object27, 'mouseRelease', '(0.806452,0.290514,1,0,67108864)')
QtTesting.playCommand(object27, 'mousePress', '(0.806452,0.290514,1,1,33554432)')
QtTesting.playCommand(object27, 'mouseMove', '(0.806452,0.290514,1,0,33554432)')
QtTesting.playCommand(object27, 'mouseRelease', '(0.806452,0.290514,1,0,33554432)')
QtTesting.playCommand(object21, 'mousePress', '(0.419355,0.304348,1,1,0)')
QtTesting.playCommand(object21, 'mouseMove', '(0.419355,0.304348,1,0,0)')
QtTesting.playCommand(object21, 'mouseRelease', '(0.419355,0.304348,1,0,0)')

#test manta material in the middle view
object37 = 'pqClientMainWindow/variableToolbar/displayColor/Variables'
QtTesting.playCommand(object37, 'set_string', 'Solid Color')
object38 = 'pqClientMainWindow/1QTabBar1'
QtTesting.playCommand(object38, 'set_tab_with_text', 'Display')
object39 = 'pqClientMainWindow/displayDock/displayWidgetFrame/displayScrollArea/qt_scrollarea_vcontainer/1QScrollBar0'
QtTesting.playCommand(object39, 'mousePress', '1,1,0,8,40')
QtTesting.playCommand(object39, 'mouseMove', '1,0,0,10,241')
QtTesting.playCommand(object39, 'mouseRelease', '1,0,0,10,241')
object40 = 'pqClientMainWindow/displayDock/displayWidgetFrame/displayScrollArea/qt_scrollarea_viewport/displayWidget/pqDisplayProxyEditor/MantaDisplay/material'
QtTesting.playCommand(object40, 'set_string', 'phong')
object41 = 'pqClientMainWindow/displayDock/displayWidgetFrame/displayScrollArea/qt_scrollarea_viewport/displayWidget/pqDisplayProxyEditor/MantaDisplay/reflectance'
QtTesting.playCommand(object41, 'set_double', '0.5')
object42 = 'pqClientMainWindow/displayDock/displayWidgetFrame/displayScrollArea/qt_scrollarea_viewport/displayWidget/pqDisplayProxyEditor/MantaDisplay/eta'

#make refresh all three views
object1 = 'pqClientMainWindow/centralwidget/MultiViewWidget/CoreWidget/qt_tabwidget_stackedwidget/MultiViewWidget1/Frame.0/Viewport'
QtTesting.playCommand(object1, 'mousePress', '(0.691244,0.23913,1,1,0)')
QtTesting.playCommand(object1, 'mouseMove', '(0.691244,0.23913,1,0,0)')
QtTesting.playCommand(object1, 'mouseRelease', '(0.691244,0.23913,1,0,0)')
QtTesting.playCommand(object1, 'mousePress', '(0.37788,0.181818,1,1,0)')
QtTesting.playCommand(object1, 'mouseMove', '(0.37788,0.181818,1,0,0)')
QtTesting.playCommand(object1, 'mouseRelease', '(0.37788,0.181818,1,0,0)')
object2 = 'pqClientMainWindow/centralwidget/MultiViewManager/SplitterFrame/MultiViewSplitter/1/Viewport'
QtTesting.playCommand(object2, 'mousePress', '(0.391705,0.199605,1,1,0)')
QtTesting.playCommand(object2, 'mouseMove', '(0.391705,0.199605,1,0,0)')
QtTesting.playCommand(object2, 'mouseRelease', '(0.391705,0.199605,1,0,0)')
object3 = 'pqClientMainWindow/centralwidget/MultiViewManager/SplitterFrame/MultiViewSplitter/2/Viewport'
QtTesting.playCommand(object3, 'mousePress', '(0.686636,0.215415,1,1,0)')
QtTesting.playCommand(object3, 'mouseMove', '(0.686636,0.215415,1,0,0)')
QtTesting.playCommand(object3, 'mouseRelease', '(0.686636,0.215415,1,0,0)')
QtTesting.playCommand(object2, 'mousePress', '(0.502304,0.22332,1,1,0)')
QtTesting.playCommand(object2, 'mouseMove', '(0.502304,0.22332,1,0,0)')
QtTesting.playCommand(object2, 'mouseRelease', '(0.502304,0.22332,1,0,0)')
QtTesting.playCommand(object1, 'mousePress', '(0.465438,0.270751,1,1,0)')
QtTesting.playCommand(object1, 'mouseMove', '(0.465438,0.270751,1,0,0)')
QtTesting.playCommand(object1, 'mouseRelease', '(0.465438,0.270751,1,0,0)')


# Image comparison
snapshotWidget = 'pqClientMainWindow/centralwidget/MultiViewManager'
QtTestingImage.compareImage(snapshotWidget, 'MantaImage.png', 300, 300);
