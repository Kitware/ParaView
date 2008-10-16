#/usr/bin/env python

import QtTesting
import QtTestingImage
import time

object1 = 'pqClientMainWindow/menubar/menuSources'
QtTesting.playCommand(object1, 'activate', 'Wavelet')
object2 = 'pqClientMainWindow/objectInspectorDock/1pqProxyTabWidget0/qt_tabwidget_stackedwidget/objectInspector/Accept'
QtTesting.playCommand(object2, 'activate', '')
object6 = 'pqClientMainWindow/objectInspectorDock/1pqProxyTabWidget0/qt_tabwidget_tabbar'
QtTesting.playCommand(object6, 'set_tab', '1')
object24 = 'pqClientMainWindow/objectInspectorDock/1pqProxyTabWidget0/qt_tabwidget_stackedwidget/1QScrollArea0/qt_scrollarea_viewport/1pqDisplayProxyEditorWidget0/pqDisplayProxyEditor/StyleGroup/StyleRepresentation/comboBox'
QtTesting.playCommand(object24, 'set_string', 'Surface')
object25 = 'pqClientMainWindow/objectInspectorDock/1pqProxyTabWidget0/qt_tabwidget_stackedwidget/1QScrollArea0/qt_scrollarea_viewport/1pqDisplayProxyEditorWidget0/pqDisplayProxyEditor/ColorGroup/ColorBy/Variables'
QtTesting.playCommand(object25, 'set_string', 'RTData')
QtTesting.playCommand(object6, 'set_tab', '0')
object3 = 'pqClientMainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/0/MultiViewFrameMenu/SplitHorizontalButton'
QtTesting.playCommand(object3, 'activate', '')

QtTesting.playCommand(object1, 'activate', 'Arrow')
QtTesting.playCommand(object2, 'activate', '')
object4 = 'pqClientMainWindow/objectInspectorDock/1pqProxyTabWidget0/qt_tabwidget_stackedwidget/objectInspector/Delete'
QtTesting.playCommand(object4, 'activate', '')

object4 = 'pqClientMainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/1/MultiViewFrameMenu/SplitVerticalButton'
QtTesting.playCommand(object4, 'activate', '')
QtTesting.playCommand(object1, 'activate', 'Sphere')
QtTesting.playCommand(object2, 'activate', '')
object5 = 'pqClientMainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/MultiViewSplitter:1/1/Viewport'
QtTesting.playCommand(object5, 'mousePress', '(0.533784,0.680147,1,1,0)')
QtTesting.playCommand(object5, 'mouseMove', '(0.533784,0.680147,1,0,0)')
QtTesting.playCommand(object5, 'mouseRelease', '(0.533784,0.680147,1,0,0)')
QtTesting.playCommand(object6, 'set_tab', '1')
object8 = 'pqClientMainWindow/objectInspectorDock/1pqProxyTabWidget0/qt_tabwidget_stackedwidget/1QScrollArea0/qt_scrollarea_viewport/1pqDisplayProxyEditorWidget0/Form/ViewGroup/ViewData'
QtTesting.playCommand(object8, 'set_boolean', 'false')
QtTesting.playCommand(object8, 'set_boolean', 'false')
QtTesting.playCommand(object8, 'set_boolean', 'false')
QtTesting.playCommand(object8, 'set_boolean', 'false')
object9 = 'pqClientMainWindow/menubar/menuFile'
QtTesting.playCommand(object9, 'activate', 'actionFileOpen')
QtTesting.playCommand(object6, 'set_tab', '0')
object10 = 'pqClientMainWindow/FileOpenDialog'
QtTesting.playCommand(object10, 'filesSelected', '$PARAVIEW_DATA_ROOT/Data/dualSphereAnimation.pvd')
QtTesting.playCommand(object2, 'activate', '')
object11 = 'pqClientMainWindow/menubar/menuView'
QtTesting.playCommand(object11, 'activate', 'Animation Inspector')
object12 = 'pqClientMainWindow/animationPanelDock/1pqAnimationPanel0/scrollArea/qt_scrollarea_viewport/AnimationPanel/tracksGroup/propertyName'
object14 = 'pqClientMainWindow/animationPanelDock/1pqAnimationPanel0/scrollArea/qt_scrollarea_viewport/AnimationPanel/keyFramePropertiesGroup/addKeyFrame'
object15 = 'pqClientMainWindow/pipelineBrowserDock/pipelineBrowser/PipelineView'
QtTesting.playCommand(object15, 'currentChanged', '/0/1|0')
QtTesting.playCommand(object12, 'set_string', 'End Theta')
QtTesting.playCommand(object14, 'activate', '')
object16 = 'pqClientMainWindow/animationPanelDock/1pqAnimationPanel0/scrollArea/qt_scrollarea_viewport/AnimationPanel/keyFramePropertiesGroup/editorFrame/keyFrameIndex'
QtTesting.playCommand(object16, 'set_int', '1')
QtTesting.playCommand(object16, 'set_int', '0')
QtTesting.playCommand(object15, 'currentChanged', '/0/0|0')
QtTesting.playCommand(object14, 'activate', '')
QtTesting.playCommand(object16, 'set_int', '1')
QtTesting.playCommand(object16, 'set_int', '0')
object17 = 'pqClientMainWindow/VCRToolbar/1QToolButton0'
QtTesting.playCommand(object17, 'activate', '')
object18 = 'pqClientMainWindow/VCRToolbar/1QToolButton3'
QtTesting.playCommand(object16, 'set_int', '1')
QtTesting.playCommand(object16, 'set_int', '0')
QtTesting.playCommand(object16, 'set_int', '1')
QtTesting.playCommand(object16, 'set_int', '0')
QtTesting.playCommand(object16, 'set_int', '1')
object19 = 'pqClientMainWindow/animationPanelDock/1pqAnimationPanel0/scrollArea/qt_scrollarea_viewport/AnimationPanel/keyFramePropertiesGroup/editorFrame/SignalAdaptorKeyFrameValue/lineEdit'
QtTesting.playCommand(object19, 'set_string', '10')
QtTesting.playCommand(object19, 'set_string', '10')
object20 = 'pqClientMainWindow/VCRToolbar/1QToolButton1'
QtTesting.playCommand(object11, 'activate', 'Animation Inspector')
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

