#/usr/bin/env python

import QtTesting
import QtTestingImage

object1 = 'pqClientMainWindow/menubar/menuSources'
QtTesting.playCommand(object1, 'activate', 'RTAnalyticSource')
object2 = 'pqClientMainWindow/objectInspectorDock/1pqProxyTabWidget0/qt_tabwidget_stackedwidget/objectInspector/Accept'
QtTesting.playCommand(object2, 'activate', '')
object3 = 'pqClientMainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/0/MultiViewFrameMenu/SplitHorizontalButton'
QtTesting.playCommand(object3, 'activate', '')
object4 = 'pqClientMainWindow/menubar/menuFilters/DataAnalysis'
QtTesting.playCommand(object4, 'activate', 'ProbeLine')
QtTesting.playCommand(object2, 'activate', '')
object5 = 'pqClientMainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/1/MultiViewFrameMenu/SplitVerticalButton'
QtTesting.playCommand(object5, 'activate', '')
object6 = 'pqClientMainWindow/pipelineBrowserDock/pipelineBrowser/PipelineView'
QtTesting.playCommand(object6, 'mousePress', '1,1,0,51,13,/0:0/0:0')
QtTesting.playCommand(object6, 'mouseRelease', '1,0,0,51,13,/0:0/0:0')
QtTesting.playCommand(object4, 'activate', 'ExtractHistogram')
QtTesting.playCommand(object2, 'activate', '')
object7 = 'pqClientMainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/0/MultiViewFrameMenu/SplitVerticalButton'
QtTesting.playCommand(object7, 'activate', '')
object8 = 'pqClientMainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/MultiViewSplitter:0/0/MultiViewFrameMenu/WindowCaption'
QtTesting.playCommand(object8, 'mousePress', '1,1,0,42,4')
QtTesting.playCommand(object8, 'mouseRelease', '1,0,0,42,4')

object9 = 'pqClientMainWindow/menubar/menuFile'
QtTesting.playCommand(object9, 'activate', 'actionFileSaveScreenshot')
object12 = 'pqClientMainWindow/SaveSnapshotDialog/selectedViewOnly'
QtTesting.playCommand(object12, 'set_boolean', 'false')
object10 = 'pqClientMainWindow/SaveSnapshotDialog/width'
QtTesting.playCommand(object10, 'set_string', '1')
QtTesting.playCommand(object10, 'set_string', '10')
QtTesting.playCommand(object10, 'key', '16777219')
QtTesting.playCommand(object10, 'key', '16777219')
QtTesting.playCommand(object10, 'set_string', '9')
QtTesting.playCommand(object10, 'set_string', '90')
QtTesting.playCommand(object10, 'set_string', '900')
object11 = 'pqClientMainWindow/SaveSnapshotDialog/height'
QtTesting.playCommand(object11, 'key', '16777217')
QtTesting.playCommand(object11, 'set_string', '90')
QtTesting.playCommand(object11, 'set_string', '90')
QtTesting.playCommand(object11, 'set_string', '900')
object13 = 'pqClientMainWindow/SaveSnapshotDialog/ok'
QtTesting.playCommand(object13, 'activate', '')
object14 = 'pqClientMainWindow/FileSaveScreenshotDialog'
#remove old file, if any
QtTesting.playCommand(object14, 'remove',
  '$PARAVIEW_TEST_ROOT/savelargescreenshot.test.png')
QtTesting.playCommand(object14, 'filesSelected', '$PARAVIEW_TEST_ROOT/savelargescreenshot.test.png')

import time
print "Wait for 2 secs"
time.sleep(2);
QtTestingImage.compareImage('$PARAVIEW_TEST_ROOT/savelargescreenshot.test.png',
  'SaveLargeScreenshot.png');
