#Tests streaming server side rendering for CS mode
#StreamingClientSideTest.py tests client side rendering in builtin and CS modes

#/usr/bin/env python

import QtTesting
import QtTestingImage

def test_compare_image(name):
  #save image the old fashioned way to make sure we loop
  print "comparing "+name
  QtTesting.playCommand(object1008, 'pause', '2000')
  object1009 = 'pqClientMainWindow/menubar/menu_File'
  QtTesting.playCommand(object1009, 'activate', 'actionFileSaveScreenshot')
  object1010 = 'pqClientMainWindow/SaveSnapshotDialog/width'
  QtTesting.playCommand(object1010, 'set_string', '300')
  object1011 = 'pqClientMainWindow/SaveSnapshotDialog/height'
  QtTesting.playCommand(object1011, 'set_string', '300')
  object1013 = 'pqClientMainWindow/SaveSnapshotDialog/ok'
  QtTesting.playCommand(object1013, 'activate', '')
  object1014 = 'pqClientMainWindow/FileSaveScreenshotDialog'
  QtTesting.playCommand(object1014, 'remove',
                        '$PARAVIEW_TEST_ROOT/'+name)
  QtTesting.playCommand(object1014, 'filesSelected', '$PARAVIEW_TEST_ROOT/'+name)
  QtTesting.playCommand(object1008, 'pause', '2000')
  QtTestingImage.compareImage('$PARAVIEW_TEST_ROOT/'+name, name);

#using this to put 'barriers' in to deal with streaming and testing asynchrony
object1008 = 'pqClientMainWindow'

#load streaming view plugin on client and server sides
print "loading plugin"
object1 = 'pqClientMainWindow/menubar'
QtTesting.playCommand(object1, 'activate', 'menuTools')
object2 = 'pqClientMainWindow/menubar/menuTools'
QtTesting.playCommand(object2, 'activate', 'actionManage_Plugins')
object3 = 'pqClientMainWindow/PluginManagerDialog/localGroup/localPlugins'
QtTesting.playCommand(object3, 'setCurrent', 'StreamingView')
object4 = 'pqClientMainWindow/PluginManagerDialog/localGroup'
QtTesting.playCommand(object4, 'mousePress', '1,1,0,152,504')
QtTesting.playCommand(object4, 'mouseRelease', '1,0,0,152,504')
object5 = 'pqClientMainWindow/PluginManagerDialog/localGroup/loadSelected_Local'
QtTesting.playCommand(object5, 'activate', '')
objectR1 = 'pqClientMainWindow/PluginManagerDialog/remoteGroup/remotePlugins'
QtTesting.playCommand(objectR1, 'setCurrent', 'StreamingView')
objectR2 = 'pqClientMainWindow/PluginManagerDialog/remoteGroup/loadSelected_Remote'
QtTesting.playCommand(objectR2, 'activate', '')
object6 = 'pqClientMainWindow/PluginManagerDialog/buttonBox/1QPushButton0'
QtTesting.playCommand(object6, 'activate', '')

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

# Test the iterating view
print "opening iterating view"
object7 = 'pqClientMainWindow/centralwidget/MultiViewWidget/CoreWidget/qt_tabwidget_stackedwidget/MultiViewWidget1/Frame.0/Close'
QtTesting.playCommand(object7, 'activate', '')
object8 = 'pqClientMainWindow/centralwidget/MultiViewWidget/CoreWidget/qt_tabwidget_stackedwidget/MultiViewWidget1/Frame.0/EmptyView/scrollArea/qt_scrollarea_viewport/widgetFoo/ConvertActionsFrame/IteratingView'
QtTesting.playCommand(object8, 'activate', '')
object9 = 'pqClientMainWindow/pqStreamingControls/dockWidgetContents/scrollArea/qt_scrollarea_viewport/scrollAreaWidgetContents/streaming_controls/cache_size'
QtTesting.playCommand(object9, 'set_string', 'no')
object9_5 = 'pqClientMainWindow/pqStreamingControls/dockWidgetContents/scrollArea/qt_scrollarea_viewport/scrollAreaWidgetContents/streaming_controls/number_of_passes'
QtTesting.playCommand(object9_5, 'set_int', '32')
QtTesting.playCommand(object1, 'activate', 'menuSources')
object11 = 'pqClientMainWindow/menubar/menuSources'
QtTesting.playCommand(object11, 'activate', 'StreamingSource')
object12 = 'pqClientMainWindow/objectInspectorDock/objectInspector/Accept'
QtTesting.playCommand(object12, 'activate', '')
QtTesting.playCommand(object1, 'activate', 'menuFilters')
object13 = 'pqClientMainWindow/menubar/menuFilters/Alphabetical'
QtTesting.playCommand(object13, 'activate', 'DataSetSurfaceFilter')
QtTesting.playCommand(object12, 'activate', '')
object20 = 'pqClientMainWindow/cameraToolbar/actionPositiveZ'
QtTesting.playCommand(object20, 'activate', '')
object21 = 'pqClientMainWindow/centralwidget/MultiViewWidget/CoreWidget/qt_tabwidget_stackedwidget/MultiViewWidget1/Frame.0/Viewport'
QtTesting.playCommand(object21, 'mousePress', '(0.529061,0.519763,1,1,0)')
QtTesting.playCommand(object21, 'mouseMove', '(0.71237,0.632411,1,0,0)')
QtTesting.playCommand(object21, 'mouseRelease', '(0.71237,0.632411,1,0,0)')
#test_compare_image('CSStreamingImage.png')

# Test the prioritizing view
print "opening prioritizing view"
QtTesting.playCommand(object7, 'activate', '')
object15 = 'pqClientMainWindow/centralwidget/MultiViewWidget/CoreWidget/qt_tabwidget_stackedwidget/MultiViewWidget1/Frame.0/EmptyView/scrollArea/qt_scrollarea_viewport/widgetFoo/ConvertActionsFrame/PrioritizingView'
QtTesting.playCommand(object15, 'activate', '')
QtTesting.playCommand(object9, 'set_string', 'no')
QtTesting.playCommand(object9_5, 'set_int', '32')
object16 = 'pqClientMainWindow/pipelineBrowserDock/pipelineBrowser'
QtTesting.playCommand(object16, 'mousePress', '1,1,0,12,13,/0:0/0:0/0:1')
QtTesting.playCommand(object16, 'mouseRelease', '1,0,0,12,13,/0:0/0:0/0:1')
object17 = 'pqClientMainWindow/cameraToolbar/actionNegativeX'
QtTesting.playCommand(object17, 'activate', '')
object18 = 'pqClientMainWindow/cameraToolbar/actionPositiveY'
QtTesting.playCommand(object18, 'activate', '')
object19 = 'pqClientMainWindow/cameraToolbar/actionNegativeY'
QtTesting.playCommand(object19, 'activate', '')
object20 = 'pqClientMainWindow/cameraToolbar/actionPositiveZ'
QtTesting.playCommand(object20, 'activate', '')
object21 = 'pqClientMainWindow/centralwidget/MultiViewWidget/CoreWidget/qt_tabwidget_stackedwidget/MultiViewWidget1/Frame.0/Viewport'
QtTesting.playCommand(object21, 'mousePress', '(0.529061,0.519763,1,1,0)')
QtTesting.playCommand(object21, 'mouseMove', '(0.71237,0.632411,1,0,0)')
QtTesting.playCommand(object21, 'mouseRelease', '(0.71237,0.632411,1,0,0)')
#test_compare_image('CSStreamingImage.png')

# Test the refining view by refining and coarsening
print "opening refining view"
QtTesting.playCommand(object7, 'activate', '')
object27 = 'pqClientMainWindow/centralwidget/MultiViewWidget/CoreWidget/qt_tabwidget_stackedwidget/MultiViewWidget1/Frame.0/EmptyView/scrollArea/qt_scrollarea_viewport/widgetFoo/ConvertActionsFrame/RefiningView'
QtTesting.playCommand(object27, 'activate', '')
object28 = 'pqClientMainWindow/pqStreamingControls/dockWidgetContents/scrollArea/qt_scrollarea_viewport/scrollAreaWidgetContents/streaming_controls/cache_size'
QtTesting.playCommand(object28, 'set_string', 'no')
object29 = 'pqClientMainWindow/pqStreamingControls/dockWidgetContents/scrollArea/qt_scrollarea_vcontainer/1QScrollBar0'
QtTesting.playCommand(object29, 'mousePress', '1,1,0,7,35')
QtTesting.playCommand(object29, 'mouseMove', '1,0,0,4,88')
QtTesting.playCommand(object29, 'mouseRelease', '1,0,0,4,88')
object30 = 'pqClientMainWindow/pqStreamingControls/dockWidgetContents/scrollArea/qt_scrollarea_viewport/scrollAreaWidgetContents/refinement_controls/refinement_depth'
QtTesting.playCommand(object30, 'set_string', '3')
QtTesting.playCommand(object16, 'mousePress', '1,1,0,12,13,/0:0/0:0/0:1')
QtTesting.playCommand(object16, 'mouseRelease', '1,0,0,12,13,/0:0/0:0/0:1')
object34 = 'pqClientMainWindow/cameraToolbar/actionPositiveZ'
QtTesting.playCommand(object34, 'activate', '')
object35 = 'pqClientMainWindow/centralwidget/MultiViewWidget/CoreWidget/qt_tabwidget_stackedwidget/MultiViewWidget1/Frame.0/Viewport'
QtTesting.playCommand(object35, 'mousePress', '(0.529061,0.519763,1,1,0)')
QtTesting.playCommand(object35, 'mouseMove', '(0.71237,0.632411,1,0,0)')
QtTesting.playCommand(object35, 'mouseRelease', '(0.71237,0.632411,1,0,0)')
object36 = 'pqClientMainWindow/pqStreamingControls/dockWidgetContents/scrollArea/qt_scrollarea_viewport/scrollAreaWidgetContents/refinement_controls'
QtTesting.playCommand(object36, 'mousePress', '1,1,0,240,52')
QtTesting.playCommand(object36, 'mouseMove', '1,0,0,243,71')
QtTesting.playCommand(object36, 'mouseRelease', '1,0,0,243,71')
QtTesting.playCommand(object29, 'mousePress', '1,1,0,6,94')
QtTesting.playCommand(object29, 'mouseMove', '1,0,0,7,115')
QtTesting.playCommand(object29, 'mouseRelease', '1,0,0,7,115')
object37 = 'pqClientMainWindow/pqStreamingControls/dockWidgetContents/scrollArea/qt_scrollarea_viewport/scrollAreaWidgetContents/refinement_controls/refine'
QtTesting.playCommand(object37, 'activate', '')
QtTesting.playCommand(object37, 'activate', '')
object38 = 'pqClientMainWindow/pqStreamingControls/dockWidgetContents/scrollArea/qt_scrollarea_viewport/scrollAreaWidgetContents/refinement_controls/coarsen'
QtTesting.playCommand(object38, 'activate', '')
#test_compare_image('CSRefiningImage.png')

QtTesting.playCommand(object1008, 'pause', '2000')
QtTesting.playCommand(object7, 'activate', '')
print "test done"
