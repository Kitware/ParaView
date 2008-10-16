#/usr/bin/env python

import QtTesting
import QtTestingImage

object1 = 'pqClientMainWindow/menubar/menuSources'
QtTesting.playCommand(object1, 'activate', 'SphereSource')
object2 = 'pqClientMainWindow/objectInspectorDock/1pqProxyTabWidget0/qt_tabwidget_stackedwidget/objectInspector/Accept'
QtTesting.playCommand(object2, 'activate', '')
object3 = 'pqClientMainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/0/MultiViewFrameMenu/SplitVerticalButton'
QtTesting.playCommand(object3, 'activate', '')
QtTesting.playCommand(object1, 'activate', 'SphereSource')
object4 = 'pqClientMainWindow/objectInspectorDock/1pqProxyTabWidget0/qt_tabwidget_stackedwidget/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/Center_0'
QtTesting.playCommand(object4, 'set_string', '1')
QtTesting.playCommand(object2, 'activate', '')
object5 = 'pqClientMainWindow/pipelineBrowserDock/pipelineBrowser/PipelineView'
QtTesting.playCommand(object5, 'currentChanged', '/0/0|0')
object6 = 'pqClientMainWindow/objectInspectorDock/1pqProxyTabWidget0/qt_tabwidget_tabbar'
QtTesting.playCommand(object6, 'set_tab', '1')
object7 = 'pqClientMainWindow/objectInspectorDock/1pqProxyTabWidget0/qt_tabwidget_stackedwidget/1QScrollArea0/qt_scrollarea_viewport/1pqDisplayProxyEditorWidget0/Form/ViewGroup/ViewData'
QtTesting.playCommand(object7, 'set_boolean', 'true')
object8 = 'pqClientMainWindow/menubar/menuView'
QtTesting.playCommand(object8, 'activate', 'Animation Inspector')
object9 = 'pqClientMainWindow/animationPanelDock/1pqAnimationPanel0/scrollArea/qt_scrollarea_viewport/AnimationPanel/tracksGroup/propertyName'
QtTesting.playCommand(object9, 'set_string', 'Start Theta')
object10 = 'pqClientMainWindow/animationPanelDock/1pqAnimationPanel0/scrollArea/qt_scrollarea_viewport/AnimationPanel/keyFramePropertiesGroup/addKeyFrame'
QtTesting.playCommand(object10, 'activate', '')
object11 = 'pqClientMainWindow/VCRToolbar/1QToolButton3'
QtTesting.playCommand(object11, 'activate', '')
QtTesting.playCommand(object11, 'activate', '')
object12 = 'pqClientMainWindow/menubar/menuFile'
QtTesting.playCommand(object12, 'activate', '')
QtTesting.playCommand(object12, 'activate', 'actionFileSaveAnimation')
object13 = 'Dialog/checkBoxDisconnect'
QtTesting.playCommand(object13, 'set_boolean', 'true')
object14 = 'Dialog/spinBoxWidth'
QtTesting.playCommand(object14, 'set_int', '300')
object14 = 'Dialog/spinBoxHeight'
QtTesting.playCommand(object14, 'set_int', '300')
object15 = 'Dialog/okButton'
QtTesting.playCommand(object15, 'activate', '')
object16 = 'FileSaveAnimationDialog'

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

