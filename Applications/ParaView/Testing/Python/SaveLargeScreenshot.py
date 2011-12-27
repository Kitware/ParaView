#/usr/bin/env python

import QtTesting
import QtTestingImage

object1 = 'pqClientMainWindow/menubar'
QtTesting.playCommand(object1, 'activate', 'menuSources')
object2 = 'pqClientMainWindow/menubar/menuSources'
QtTesting.playCommand(object2, 'activate', 'SphereSource')
object3 = 'pqClientMainWindow/summaryDock/summaryWidget/1QFrame0/Accept'
QtTesting.playCommand(object3, 'activate', '')
QtTesting.playCommand(object1, 'activate', 'menuFilters')
object4 = 'pqClientMainWindow/menubar/menuFilters/Alphabetical'
QtTesting.playCommand(object4, 'activate', 'ElevationFilter')
QtTesting.playCommand(object3, 'activate', '')
QtTesting.playCommand(object1, 'activate', 'menuFilters')
object5 = 'pqClientMainWindow/menubar/menuFilters/DataAnalysis'
QtTesting.playCommand(object5, 'activate', 'PlotAttributes')
QtTesting.playCommand(object3, 'activate', '')
object6 = 'pqClientMainWindow/centralwidget/MultiViewWidget/CoreWidget/qt_tabwidget_stackedwidget/MultiViewWidget1/Splitter.0/Frame.2/SplitVertical'
QtTesting.playCommand(object6, 'activate', '')
object7 = 'pqClientMainWindow/centralwidget/MultiViewWidget/CoreWidget/qt_tabwidget_stackedwidget/MultiViewWidget1/Splitter.0/Splitter.2/Frame.6/EmptyView/scrollArea/qt_scrollarea_viewport/widgetFoo/ConvertActionsFrame/RenderView'
QtTesting.playCommand(object7, 'activate', '')
object8 = 'pqClientMainWindow/centralwidget/MultiViewWidget/CoreWidget/qt_tabwidget_stackedwidget/MultiViewWidget1/Splitter.0/Frame.1/SplitVertical'
QtTesting.playCommand(object8, 'activate', '')
object9 = 'pqClientMainWindow/centralwidget/MultiViewWidget/CoreWidget/qt_tabwidget_stackedwidget/MultiViewWidget1/Splitter.0/Splitter.2/Frame.5/TitleBar'
QtTesting.playCommand(object9, 'mousePress', '1,1,0,148,7')
QtTesting.playCommand(object9, 'mouseRelease', '1,0,0,148,7')
QtTesting.playCommand(object1, 'activate', 'menu_File')
object10 = 'pqClientMainWindow/menubar/menu_File'
QtTesting.playCommand(object10, 'activate', 'actionFileSaveScreenshot')
object11 = 'pqClientMainWindow/SaveSnapshotDialog/selectedViewOnly'
QtTesting.playCommand(object11, 'set_boolean', 'false')
object12 = 'pqClientMainWindow/SaveSnapshotDialog/width'
QtTesting.playCommand(object12, 'set_string', '900')
object13 = 'pqClientMainWindow/SaveSnapshotDialog/height'
QtTesting.playCommand(object13, 'set_string', '900')
object14 = 'pqClientMainWindow/SaveSnapshotDialog/ok'
QtTesting.playCommand(object14, 'activate', '')

object15 = 'pqClientMainWindow/FileSaveScreenshotDialog'
#remove old file, if any
QtTesting.playCommand(object15, 'remove', '$PARAVIEW_TEST_ROOT/savelargescreenshot.test.png')
QtTesting.playCommand(object15, 'filesSelected', '$PARAVIEW_TEST_ROOT/savelargescreenshot.test.png')

import time
print "Wait for 2 secs"
time.sleep(2);
QtTestingImage.compareImage('$PARAVIEW_TEST_ROOT/savelargescreenshot.test.png',
  'SaveLargeScreenshot.png');
