#/usr/bin/env python

import QtTesting
import QtTestingImage

object1 = 'pqClientMainWindow/menubar/menuSources'
QtTesting.playCommand(object1, 'activate', 'SphereSource')
object2 = 'pqClientMainWindow/proxyTabDock/proxyTabWidget/qt_tabwidget_stackedwidget/objectInspector/Accept'
QtTesting.playCommand(object2, 'activate', '')
object3 = 'pqClientMainWindow/centralwidget/MultiViewManager/SplitterFrame/MultiViewSplitter/0/MultiViewFrameMenu/SplitVerticalButton'
QtTesting.playCommand(object3, 'activate', '')
QtTesting.playCommand(object1, 'activate', 'SphereSource')
object4 = 'pqClientMainWindow/proxyTabDock/proxyTabWidget/qt_tabwidget_stackedwidget/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/Center_0'
QtTesting.playCommand(object4, 'set_string', '1')
QtTesting.playCommand(object2, 'activate', '')
object5 = 'pqClientMainWindow/pipelineBrowserDock/pipelineBrowser'
QtTesting.playCommand(object5, 'currentChanged', '/0/0|0')
object6 = 'pqClientMainWindow/proxyTabDock/proxyTabWidget/qt_tabwidget_tabbar'
QtTesting.playCommand(object6, 'set_tab', '1')
object7 = 'pqClientMainWindow/proxyTabDock/proxyTabWidget/qt_tabwidget_stackedwidget/1QScrollArea0/qt_scrollarea_viewport/1pqDisplayProxyEditorWidget0/Form/ViewGroup/ViewData'
QtTesting.playCommand(object7, 'set_boolean', 'true')
object8 = 'pqClientMainWindow/menubar'
QtTesting.playCommand(object8, 'activate', 'menu_View')
object8 = 'pqClientMainWindow/menubar/menu_View'
QtTesting.playCommand(object8, 'activate', 'Animation View')
object9 = 'pqClientMainWindow/animationViewDock/animationView/pqAnimationWidget/CreateDeleteWidget/PropertyCombo'
QtTesting.playCommand(object9, 'set_string', 'Start Theta')
object10 = "pqClientMainWindow/animationViewDock/animationView/1pqAnimationWidget0/1QHeaderView0"
QtTesting.playCommand(object10, "mousePress", "1,1,0,0,0,2")
QtTesting.playCommand(object10, "mouseRelease", "1,1,0,0,0,2")
object11 = 'pqClientMainWindow/VCRToolbar/1QToolButton3'
QtTesting.playCommand(object11, 'activate', '')
QtTesting.playCommand(object11, 'activate', '')
object12 = 'pqClientMainWindow/menubar/menu_File'
QtTesting.playCommand(object12, 'activate', '')
QtTesting.playCommand(object12, 'activate', 'actionFileSaveAnimation')
object13 = 'pqAnimationSettingsDialog/checkBoxDisconnect'
QtTesting.playCommand(object13, 'set_boolean', 'true')
object14 = 'pqAnimationSettingsDialog/width'
QtTesting.playCommand(object14, 'set_string', '300')
object14 = 'pqAnimationSettingsDialog/height'
QtTesting.playCommand(object14, 'set_string', '300')
object15 = 'pqAnimationSettingsDialog/okButton'
QtTesting.playCommand(object15, 'activate', '')
object16 = 'pqClientMainWindow/FileSaveAnimationDialog'

# Remove old files.
QtTesting.playCommand(object16, 'remove', '$PARAVIEW_TEST_ROOT/disconnectSave.0000.png')
QtTesting.playCommand(object16, 'remove', '$PARAVIEW_TEST_ROOT/disconnectSave.0001.png')
QtTesting.playCommand(object16, 'remove', '$PARAVIEW_TEST_ROOT/disconnectSave.0002.png')
QtTesting.playCommand(object16, 'remove', '$PARAVIEW_TEST_ROOT/disconnectSave.0003.png')
QtTesting.playCommand(object16, 'remove', '$PARAVIEW_TEST_ROOT/disconnectSave.0004.png')
QtTesting.playCommand(object16, 'remove', '$PARAVIEW_TEST_ROOT/disconnectSave.0005.png')
QtTesting.playCommand(object16, 'remove', '$PARAVIEW_TEST_ROOT/disconnectSave.0006.png')
QtTesting.playCommand(object16, 'remove', '$PARAVIEW_TEST_ROOT/disconnectSave.0007.png')
QtTesting.playCommand(object16, 'remove', '$PARAVIEW_TEST_ROOT/disconnectSave.0008.png')
QtTesting.playCommand(object16, 'remove', '$PARAVIEW_TEST_ROOT/disconnectSave.0009.png')
QtTesting.playCommand(object16, 'filesSelected', '$PARAVIEW_TEST_ROOT/disconnectSave.png')

import time
print "Wait for 60 secs"
time.sleep(60);
QtTestingImage.compareImage('$PARAVIEW_TEST_ROOT/disconnectSave.0005.png', 'DisconnectAndSaveAnimation.png');

