#/usr/bin/env python

import QtTesting
import QtTestingImage

object2 = 'MainWindow/menubar/menuSources'
QtTesting.playCommand(object2, 'activate', 'HierarchicalFractal')
object3 = 'MainWindow/objectInspectorDock/1pqProxyTabWidget0/qt_tabwidget_stackedwidget/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/TwoDimensional'
QtTesting.playCommand(object3, 'set_boolean', 'false')
object4 = 'MainWindow/objectInspectorDock/1pqProxyTabWidget0/qt_tabwidget_stackedwidget/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/GhostLevels'
QtTesting.playCommand(object4, 'set_boolean', 'false')
object5 = 'MainWindow/objectInspectorDock/1pqProxyTabWidget0/qt_tabwidget_stackedwidget/objectInspector/Accept'
QtTesting.playCommand(object5, 'activate', '')
object6 = 'MainWindow/menubar/menuFilters/Alphabetical'

objectA = 'MainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/0/MultiViewFrameMenu/SplitHorizontalButton'
QtTesting.playCommand(objectA, 'activate', '')
QtTesting.playCommand(object6, 'activate', 'ProbeLine')
QtTesting.playCommand(object5, 'activate', '')
QtTesting.playCommand(object2, 'activate', 'RTAnalyticSource')
QtTesting.playCommand(object5, 'activate', '')

objectB = 'MainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/1/MultiViewFrameMenu/SplitVerticalButton'
QtTesting.playCommand(objectB, 'activate', '')

QtTesting.playCommand(object6, 'activate', 'ExtractHistogram')
QtTesting.playCommand(object5, 'activate', '')
object7 = 'MainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/0/MultiViewFrameMenu/SplitVerticalButton'
QtTesting.playCommand(object7, 'activate', '')
object1 = 'MainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/MultiViewSplitter:0/0/MultiViewFrameMenu/WindowCaption'
QtTesting.playCommand(object1, 'mousePress', '1,1,0,22,8')
QtTesting.playCommand(object1, 'mouseRelease', '1,0,0,22,8')
object9 = 'MainWindow/menubar/menuFile'
QtTesting.playCommand(object9, 'activate', 'actionFileSaveScreenshot')
object12 = 'MainWindow/SaveSnapshotDialog/selectedViewOnly'
QtTesting.playCommand(object12, 'set_boolean', 'false')
object10 = 'MainWindow/SaveSnapshotDialog/width'
QtTesting.playCommand(object10, 'set_string', '1')
QtTesting.playCommand(object10, 'set_string', '10')
QtTesting.playCommand(object10, 'key', '16777219')
QtTesting.playCommand(object10, 'key', '16777219')
QtTesting.playCommand(object10, 'set_string', '9')
QtTesting.playCommand(object10, 'set_string', '90')
QtTesting.playCommand(object10, 'set_string', '900')
object11 = 'MainWindow/SaveSnapshotDialog/height'
QtTesting.playCommand(object11, 'key', '16777217')
QtTesting.playCommand(object11, 'set_string', '90')
QtTesting.playCommand(object11, 'set_string', '90')
QtTesting.playCommand(object11, 'set_string', '900')
object13 = 'MainWindow/SaveSnapshotDialog/ok'
QtTesting.playCommand(object13, 'activate', '')
object14 = 'MainWindow/FileSaveScreenshotDialog'
#remove old file, if any
QtTesting.playCommand(object14, 'remove',
  '$PARAVIEW_TEST_ROOT/savelargescreenshot.test.png')
QtTesting.playCommand(object14, 'filesSelected', '$PARAVIEW_TEST_ROOT/savelargescreenshot.test.png')

import time
print "Wait for 2 secs"
time.sleep(2);
QtTestingImage.compareImage('$PARAVIEW_TEST_ROOT/savelargescreenshot.test.png',
  'SaveLargeScreenshot.png');
