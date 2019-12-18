#/usr/bin/env python

import QtTesting
import QtTestingImage
import time

object1 = 'pqClientMainWindow/menubar/menuSources'
QtTesting.playCommand(object1, 'activate', 'Wavelet')
object2 = 'pqClientMainWindow/objectInspectorDock/objectInspector/Accept'
QtTesting.playCommand(object2, 'activate', '')
object6 = 'pqClientMainWindow/1QTabBar1'
QtTesting.playCommand(object6, 'set_tab_with_text', 'Display')
object24 = 'pqClientMainWindow/displayDock/displayWidgetFrame/displayScrollArea/qt_scrollarea_viewport/displayWidget/pqDisplayProxyEditor/StyleGroup/StyleRepresentation/comboBox'
QtTesting.playCommand(object24, 'set_string', 'Surface')
object25 = 'pqClientMainWindow/displayDock/displayWidgetFrame/displayScrollArea/qt_scrollarea_viewport/displayWidget/pqDisplayProxyEditor/ColorGroup/ColorBy/Variables'
QtTesting.playCommand(object25, 'set_string', 'RTData')
QtTesting.playCommand(object6, 'set_tab_with_text', 'Properties')
object3 = 'pqClientMainWindow/centralwidget/MultiViewWidget/CoreWidget/qt_tabwidget_stackedwidget/MultiViewWidget1/Container/Frame.0/SplitHorizontal'
QtTesting.playCommand(object3, 'activate', '')

QtTesting.playCommand(object1, 'activate', 'Arrow')
QtTesting.playCommand(object2, 'activate', '')
object4 = 'pqClientMainWindow/objectInspectorDock/objectInspector/Delete'
QtTesting.playCommand(object4, 'activate', '')

object4 = 'pqClientMainWindow/centralwidget/MultiViewWidget/CoreWidget/qt_tabwidget_stackedwidget/MultiViewWidget1/Container/Frame.2/SplitVertical'
QtTesting.playCommand(object4, 'activate', '')
QtTesting.playCommand(object1, 'activate', 'Sphere')
QtTesting.playCommand(object2, 'activate', '')
QtTesting.playCommand(object6, 'set_tab_with_text', 'Display')
object8 = 'pqClientMainWindow/displayDock/displayWidgetFrame/displayScrollArea/qt_scrollarea_viewport/displayWidget/Form/ViewGroup/ViewData'
QtTesting.playCommand(object8, 'set_boolean', 'false')
QtTesting.playCommand(object8, 'set_boolean', 'false')
QtTesting.playCommand(object8, 'set_boolean', 'false')
QtTesting.playCommand(object8, 'set_boolean', 'false')
object9 = 'pqClientMainWindow/menubar/menu_File'
QtTesting.playCommand(object9, 'activate', 'actionFileOpen')
QtTesting.playCommand(object6, 'set_tab_with_text', 'Properties')
object10 = 'pqClientMainWindow/FileOpenDialog'
QtTesting.playCommand(object10, 'filesSelected', '$PARAVIEW_DATA_ROOT/Testing/Data/dualSphereAnimation.pvd')
QtTesting.playCommand(object2, 'activate', '')
object11 = 'pqClientMainWindow/menubar/menuView'
QtTesting.playCommand(object11, 'activate', 'Animation View')


object15 = 'pqClientMainWindow/pipelineBrowserDock/pipelineBrowser'
QtTesting.playCommand(object15, 'currentChanged', '/0/1|0')

#object12 = 'pqClientMainWindow/animationPanelDock/1pqAnimationPanel0/scrollArea/qt_scrollarea_viewport/AnimationPanel/tracksGroup/propertyName'
#object14 = 'pqClientMainWindow/animationPanelDock/1pqAnimationPanel0/scrollArea/qt_scrollarea_viewport/AnimationPanel/keyFramePropertiesGroup/addKeyFrame'
#QtTesting.playCommand(object12, 'set_string', 'End Theta')
#QtTesting.playCommand(object14, 'activate', '')

object12 = 'pqClientMainWindow/animationViewDock/animationView/pqAnimationWidget/CreateDeleteWidget/PropertyCombo'
QtTesting.playCommand(object12, 'set_string', 'End Theta')
object10 = "pqClientMainWindow/animationViewDock/animationView/1pqAnimationWidget0/1QHeaderView0"
QtTesting.playCommand(object10, "mousePress", "1,1,0,0,0,2")
QtTesting.playCommand(object10, "mouseRelease", "1,1,0,0,0,2")

QtTesting.playCommand(object15, 'currentChanged', '/0/0|0')
QtTesting.playCommand(object10, "mousePress", "1,1,0,0,0,3")
QtTesting.playCommand(object10, "mouseRelease", "1,1,0,0,0,3")

object17 = 'pqClientMainWindow/VCRToolbar/1QToolButton0'
QtTesting.playCommand(object17, 'activate', '')
object18 = 'pqClientMainWindow/VCRToolbar/1QToolButton3'

#object19 = 'pqClientMainWindow/animationPanelDock/1pqAnimationPanel0/scrollArea/qt_scrollarea_viewport/AnimationPanel/keyFramePropertiesGroup/editorFrame/SignalAdaptorKeyFrameValue/lineEdit'
#QtTesting.playCommand(object19, 'set_string', '10')
#QtTesting.playCommand(object19, 'set_string', '10')
object20 = 'pqClientMainWindow/VCRToolbar/1QToolButton1'
QtTesting.playCommand(object11, 'activate', 'Animation View')
QtTesting.playCommand(object11, 'activate', 'Object Inspector')
QtTesting.playCommand(object11, 'activate', 'Pipeline Browser')
QtTesting.playCommand(object9, 'activate', 'actionFileSaveAnimation')
object21 = 'Dialog/spinBoxWidth'
QtTesting.playCommand(object21, 'set_int', '800')
object22 = 'Dialog/spinBoxHeight'
QtTesting.playCommand(object22, 'set_int', '800')
object22 = 'Dialog/okButton'
QtTesting.playCommand(object22, 'activate', '')
objectSaveAnimationDialog = 'FileSaveAnimationDialog'
QtTesting.playCommand(objectSaveAnimationDialog, 'filesSelected', '$PARAVIEW_TEST_ROOT/movie_test.png')

time.sleep(3);
objectPlayButton = 'pqClientMainWindow/VCRToolbar/1QToolButton2'
while QtTesting.getProperty(objectPlayButton, "text") != 'Play' :
  time.sleep(1);

QtTestingImage.compareImage('$PARAVIEW_TEST_ROOT/movie_test.0005.png', 'SaveAnimationMultiView.png');
