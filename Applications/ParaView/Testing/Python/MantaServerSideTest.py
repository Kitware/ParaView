#/usr/bin/env python

#Tests manta server side rendering for CS mode
#MantaTest.py tests client side rendering in builtin and CS mode

import QtTesting
import QtTestingImage

#TODO fix test recording so that it actually records these events
#most of which I had to hack in by hand

#force server side rendering by setting threshold to 0
object2 = 'pqClientMainWindow/menubar/menu_Edit'
QtTesting.playCommand(object2, 'activate', 'actionEditSettings')
objectb2 = 'pqClientMainWindow/ApplicationSettings/PageNames'
QtTesting.playCommand(objectb2, 'setCurrent', '4.0')
QtTesting.playCommand(objectb2, 'expand', '4.0')
QtTesting.playCommand(objectb2, 'setCurrent', '4.0.2.0')
objectb3 = 'pqClientMainWindow/ApplicationSettings/Stack/pqGlobalRenderViewOptions/stackedWidget/Server/compositingParameters/compositeThreshold'
QtTesting.playCommand(objectb3, 'set_int', '0')
objectb4 = 'pqClientMainWindow/ApplicationSettings/CloseButton'
QtTesting.playCommand(objectb4, 'activate', '')

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
object4 = 'pqClientMainWindow/centralwidget/MultiViewWidget/CoreWidget/qt_tabwidget_stackedwidget/MultiViewWidget1/Frame.0/Close'
QtTesting.playCommand(object4, 'activate', '')
object5 = 'pqClientMainWindow/centralwidget/MultiViewWidget/CoreWidget/qt_tabwidget_stackedwidget/MultiViewWidget1/Frame.0/CentralWidgetFrame/EmptyView/scrollArea/qt_scrollarea_viewport/widgetFoo/ConvertActionsFrame/MantaView'
QtTesting.playCommand(object5, 'activate', '')

#show something
hack3 = 'pqClientMainWindow/menubar'
QtTesting.playCommand(hack3, 'activate', 'menuSources')
hack4 = 'pqClientMainWindow/menubar/menuSources'
QtTesting.playCommand(hack4, 'activate', 'SphereSource')
object6 = 'pqClientMainWindow/objectInspectorDock/objectInspector/Accept'
QtTesting.playCommand(object6, 'activate', '')

#test sphere 'glyphs' while we are at it
objectfoo = 'pqClientMainWindow/representationToolbar/displayRepresentation/comboBox'
QtTesting.playCommand(objectfoo, 'set_string', 'Points')

object7 = 'pqClientMainWindow/centralwidget/MultiViewWidget/CoreWidget/qt_tabwidget_stackedwidget/MultiViewWidget1/Frame.0/Viewport'
QtTesting.playCommand(object7, 'mousePress', '(0.719373,0.369784,1,1,0)')
QtTesting.playCommand(object7, 'mouseMove', '(0.605413,0.271942,1,0,0)')
QtTesting.playCommand(object7, 'mouseRelease', '(0.605413,0.271942,1,0,0)')

# Image comparison
snapshotWidget = 'pqClientMainWindow/centralwidget/MultiViewWidget/CoreWidget/qt_tabwidget_stackedwidget/MultiViewWidget1/Frame.0/Viewport'
QtTestingImage.compareImage(snapshotWidget, 'MantaSSImage.png', 300, 300);
