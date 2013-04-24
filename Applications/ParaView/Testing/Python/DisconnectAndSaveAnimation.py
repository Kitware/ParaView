#/usr/bin/env python

import QtTesting
import QtTestingImage

object1 = 'pqClientMainWindow/menubar/menuSources'
QtTesting.playCommand(object1, 'activate', 'SphereSource')
object2 = 'pqClientMainWindow/propertiesDock/propertiesPanel/Accept'
QtTesting.playCommand(object2, 'activate', '')
object3 = 'pqClientMainWindow/centralwidget/MultiViewWidget/CoreWidget/qt_tabwidget_stackedwidget/MultiViewWidget1/Frame.0/SplitVertical'
QtTesting.playCommand(object3, 'activate', '')
QtTesting.playCommand(object1, 'activate', 'SphereSource')
object4 = 'pqClientMainWindow/propertiesDock/propertiesPanel/scrollArea/qt_scrollarea_viewport/scrollAreaWidgetContents/PropertiesFrame/ProxyPanel/Center/LineEdit0'
QtTesting.playCommand(object4, 'set_string', '1')
QtTesting.playCommand(object2, 'activate', '')
object5 = 'pqClientMainWindow/pipelineBrowserDock/pipelineBrowser'
QtTesting.playCommand(object5, 'currentChanged', '/0/0|0')
object6 = 'pqClientMainWindow/1QTabBar1'

object7 = 'pqClientMainWindow/pipelineBrowserDock/pipelineBrowser'
QtTesting.playCommand(object7, 'mousePress', '1,1,0,10,12,/0:0/0:1')
QtTesting.playCommand(object7, "mouseRelease", "1,0,0,10,12,/0:0/0:1")

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
print "Wait for 20 secs"
time.sleep(20);
QtTestingImage.compareImage('$PARAVIEW_TEST_ROOT/disconnectSave.0005.png', 'DisconnectAndSaveAnimation.png');

