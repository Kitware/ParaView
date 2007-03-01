#/usr/bin/env python

import QtTesting
import QtTestingImage
import time

object1 = 'MainWindow/menubar/menuFile'
QtTesting.playCommand(object1, 'activate', 'actionServerConnect')
object2 = 'MainWindow/ServerStartupBrowser/connect'
QtTesting.playCommand(object2, 'activate', '')

object1 = 'MainWindow/menubar/menuSources'
QtTesting.playCommand(object1, 'activate', 'Wavelet')
object2 = 'MainWindow/objectInspectorDock/1pqProxyTabWidget0/qt_tabwidget_stackedwidget/objectInspector/Accept'
QtTesting.playCommand(object2, 'activate', '')
object3 = 'MainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/0/MultiViewFrameMenu/SplitHorizontalButton'
QtTesting.playCommand(object3, 'activate', '')

QtTesting.playCommand(object1, 'activate', 'Arrow')
QtTesting.playCommand(object2, 'activate', '')
object4 = 'MainWindow/objectInspectorDock/1pqProxyTabWidget0/qt_tabwidget_stackedwidget/objectInspector/Delete'
QtTesting.playCommand(object4, 'activate', '')

object4 = 'MainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/1/MultiViewFrameMenu/SplitVerticalButton'
QtTesting.playCommand(object4, 'activate', '')
QtTesting.playCommand(object1, 'activate', 'Sphere')
QtTesting.playCommand(object2, 'activate', '')
object5 = 'MainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/MultiViewSplitter:1/1/Viewport'
QtTesting.playCommand(object5, 'mousePress', '(0.533784,0.680147,1,1,0)')
QtTesting.playCommand(object5, 'mouseMove', '(0.533784,0.680147,1,0,0)')
QtTesting.playCommand(object5, 'mouseRelease', '(0.533784,0.680147,1,0,0)')
object6 = 'MainWindow/objectInspectorDock/1pqProxyTabWidget0/qt_tabwidget_tabbar'
QtTesting.playCommand(object6, 'set_tab', '1')
object8 = 'MainWindow/objectInspectorDock/1pqProxyTabWidget0/qt_tabwidget_stackedwidget/1QScrollArea0/qt_scrollarea_viewport/1pqDisplayProxyEditorWidget0/Form/ViewGroup/ViewData'
QtTesting.playCommand(object8, 'set_boolean', 'false')
QtTesting.playCommand(object8, 'set_boolean', 'false')
QtTesting.playCommand(object8, 'set_boolean', 'false')
QtTesting.playCommand(object8, 'set_boolean', 'false')
object9 = 'MainWindow/menubar/menuFile'
QtTesting.playCommand(object9, 'activate', 'actionFileOpen')
QtTesting.playCommand(object6, 'set_tab', '0')
object10 = 'MainWindow/FileOpenDialog'
QtTesting.playCommand(object10, 'filesSelected', '$PARAVIEW_DATA_ROOT/Data/dualSphereAnimation.pvd')
QtTesting.playCommand(object2, 'activate', '')
object11 = 'MainWindow/menubar/menuView'
QtTesting.playCommand(object11, 'activate', 'Animation Inspector')
object12 = 'MainWindow/animationPanelDock/1pqAnimationPanel0/scrollArea/qt_scrollarea_viewport/AnimationPanel/tracksGroup/propertyName'
object14 = 'MainWindow/animationPanelDock/1pqAnimationPanel0/scrollArea/qt_scrollarea_viewport/AnimationPanel/keyFramePropertiesGroup/addKeyFrame'
object15 = 'MainWindow/pipelineBrowserDock/pipelineBrowser/PipelineView'
QtTesting.playCommand(object15, 'currentChanged', '/0/1|0')
QtTesting.playCommand(object12, 'set_string', 'End Theta')
QtTesting.playCommand(object14, 'activate', '')
object16 = 'MainWindow/animationPanelDock/1pqAnimationPanel0/scrollArea/qt_scrollarea_viewport/AnimationPanel/keyFramePropertiesGroup/editorFrame/keyFrameIndex'
QtTesting.playCommand(object16, 'set_int', '1')
QtTesting.playCommand(object16, 'set_int', '0')
QtTesting.playCommand(object15, 'currentChanged', '/0/0|0')
QtTesting.playCommand(object14, 'activate', '')
QtTesting.playCommand(object16, 'set_int', '1')
QtTesting.playCommand(object16, 'set_int', '0')
object17 = 'MainWindow/VCRToolbar/1QToolButton0'
QtTesting.playCommand(object17, 'activate', '')
object18 = 'MainWindow/VCRToolbar/1QToolButton3'
QtTesting.playCommand(object18, 'activate', '')
QtTesting.playCommand(object18, 'activate', '')
QtTesting.playCommand(object18, 'activate', '')
QtTesting.playCommand(object18, 'activate', '')
QtTesting.playCommand(object18, 'activate', '')
QtTesting.playCommand(object18, 'activate', '')
QtTesting.playCommand(object18, 'activate', '')
QtTesting.playCommand(object18, 'activate', '')
QtTesting.playCommand(object17, 'activate', '')
QtTesting.playCommand(object16, 'set_int', '1')
QtTesting.playCommand(object16, 'set_int', '0')
QtTesting.playCommand(object16, 'set_int', '1')
QtTesting.playCommand(object16, 'set_int', '0')
QtTesting.playCommand(object16, 'set_int', '1')
object19 = 'MainWindow/animationPanelDock/1pqAnimationPanel0/scrollArea/qt_scrollarea_viewport/AnimationPanel/keyFramePropertiesGroup/editorFrame/SignalAdaptorKeyFrameValue/lineEdit'
QtTesting.playCommand(object19, 'set_string', '10')
QtTesting.playCommand(object19, 'set_string', '10')
QtTesting.playCommand(object17, 'activate', '')
QtTesting.playCommand(object18, 'activate', '')
QtTesting.playCommand(object18, 'activate', '')
QtTesting.playCommand(object18, 'activate', '')
QtTesting.playCommand(object18, 'activate', '')
QtTesting.playCommand(object18, 'activate', '')
QtTesting.playCommand(object18, 'activate', '')
QtTesting.playCommand(object18, 'activate', '')
QtTesting.playCommand(object18, 'activate', '')
QtTesting.playCommand(object18, 'activate', '')
QtTesting.playCommand(object18, 'activate', '')
object20 = 'MainWindow/VCRToolbar/1QToolButton1'
QtTesting.playCommand(object20, 'activate', '')
QtTesting.playCommand(object20, 'activate', '')
QtTesting.playCommand(object20, 'activate', '')
QtTesting.playCommand(object20, 'activate', '')
QtTesting.playCommand(object20, 'activate', '')
QtTesting.playCommand(object20, 'activate', '')
QtTesting.playCommand(object20, 'activate', '')
QtTesting.playCommand(object18, 'activate', '')
QtTesting.playCommand(object11, 'activate', 'Animation Inspector')
QtTesting.playCommand(object11, 'activate', 'Object Inspector')
QtTesting.playCommand(object11, 'activate', 'Pipeline Browser')
QtTesting.playCommand(object9, 'activate', 'actionFileSaveAnimation')
objectSaveAnimationDialog = 'MainWindow/FileSaveAnimationDialog'
QtTesting.playCommand(objectSaveAnimationDialog, 'filesSelected', '$PARAVIEW_TEST_ROOT/movie_test.png')
object21 = 'Dialog/spinBoxWidth'
QtTesting.playCommand(object21, 'set_int', '800')
object22 = 'Dialog/spinBoxHeight'
QtTesting.playCommand(object22, 'set_int', '800')
object22 = 'Dialog/okButton'
QtTesting.playCommand(object22, 'activate', '')

time.sleep(2);
objectPlayButton = 'MainWindow/VCRToolbar/1QToolButton2'
while QtTesting.getProperty(objectPlayButton, "text") == 'Pause' :
  time.sleep(3)

QtTestingImage.compareImage('$PARAVIEW_TEST_ROOT/movie_test.0005.png', 'SaveAnimationMultiView.png');
