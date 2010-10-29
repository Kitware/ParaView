#/usr/bin/env python

import QtTesting
import QtTestingImage

#----------------- NEXT TEST -----------
# Test isovolume of point data for vtkImageData.
object1 = 'pqClientMainWindow/menubar'
QtTesting.playCommand(object1, 'activate', 'menuSources')
object2 = 'pqClientMainWindow/menubar/menuSources'
QtTesting.playCommand(object2, 'activate', 'RTAnalyticSource')
object3 = 'pqClientMainWindow/proxyTabDock/proxyTabWidget/qt_tabwidget_stackedwidget/objectInspector/Accept'
QtTesting.playCommand(object3, 'activate', '')
QtTesting.playCommand(object1, 'activate', 'menuFilters')
object4 = 'pqClientMainWindow/menubar/menuFilters/Alphabetical'
QtTesting.playCommand(object4, 'activate', 'IsoVolume')
object5 = 'pqClientMainWindow/proxyTabDock/proxyTabWidget/qt_tabwidget_stackedwidget/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/ThresholdBetween_0/LineEdit'
object6 = 'pqClientMainWindow/proxyTabDock/proxyTabWidget/qt_tabwidget_stackedwidget/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/ThresholdBetween_1/LineEdit'
QtTesting.playCommand(object6, 'set_string', '150')
QtTesting.playCommand(object5, 'set_string', '140')
QtTesting.playCommand(object3, 'activate', '')
# DO_IMAGE_COMPARE
snapshotWidget = 'pqClientMainWindow/centralwidget/MultiViewManager/SplitterFrame/MultiViewSplitter/0/Viewport'
QtTestingImage.compareImage(snapshotWidget, 'TestIsoVolume1.png', 300, 300);

#----------------- NEXT TEST -----------
# Test isovolume of cell data for vtkImageData.

object7 = 'pqClientMainWindow/proxyTabDock/proxyTabWidget/qt_tabwidget_stackedwidget/objectInspector/Delete'
QtTesting.playCommand(object7, 'activate', '')
QtTesting.playCommand(object1, 'activate', 'menuFilters')
QtTesting.playCommand(object4, 'activate', 'PointDataToCellData')
QtTesting.playCommand(object3, 'activate', '')
QtTesting.playCommand(object1, 'activate', 'menuFilters')
QtTesting.playCommand(object4, 'activate', 'IsoVolume')
QtTesting.playCommand(object5, 'set_string', '150')
QtTesting.playCommand(object6, 'set_string', '170')
QtTesting.playCommand(object3, 'activate', '')
object9 = 'pqClientMainWindow/variableToolbar/displayColor/Variables'
QtTesting.playCommand(object9, 'set_string', 'RTData')
object9 = 'pqClientMainWindow/variableToolbar/actionScalarBarVisibility'
QtTesting.playCommand(object9, 'set_boolean', 'true')
object10 = 'pqClientMainWindow/variableToolbar/actionResetRange'

# DO_IMAGE_COMPARE
QtTestingImage.compareImage(snapshotWidget, 'TestIsoVolume2.png', 300, 300);

QtTesting.playCommand(object10, 'activate', '')
QtTesting.playCommand(object7, 'activate', '')
QtTesting.playCommand(object7, 'activate', '')
QtTesting.playCommand(object7, 'activate', '')

#-------------- NEXT TEST --------------
# Test isovolume on point-data on a vtkMultiBlockDataset
QtTesting.playCommand(object1, 'activate', 'menu_File')
object11 = 'pqClientMainWindow/menubar/menu_File'
QtTesting.playCommand(object11, 'activate', 'actionFileOpen')
object12 = 'pqClientMainWindow/FileOpenDialog'
QtTesting.playCommand(object12, 'filesSelected', '$PARAVIEW_DATA_ROOT/Data/disk_out_ref.ex2')
object13 = 'pqClientMainWindow/proxyTabDock/proxyTabWidget/qt_tabwidget_stackedwidget/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/Variables'
QtTesting.playCommand(object13, 'setCurrent', '0.0')
object14 = 'pqClientMainWindow/proxyTabDock/proxyTabWidget/qt_tabwidget_stackedwidget/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/Variables/1QHeaderView0'
QtTesting.playCommand(object14, 'mousePress', '1,1,0,0,0,0')
QtTesting.playCommand(object14, 'mouseRelease', '1,0,0,0,0,0')
QtTesting.playCommand(object3, 'activate', '')
QtTesting.playCommand(object1, 'activate', 'menuFilters')
QtTesting.playCommand(object4, 'activate', 'IsoVolume')
object16 = 'pqClientMainWindow/proxyTabDock/proxyTabWidget/qt_tabwidget_stackedwidget/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/ThresholdBetween_0/Slider'
QtTesting.playCommand(object16, 'set_int', '35')
object17 = 'pqClientMainWindow/proxyTabDock/proxyTabWidget/qt_tabwidget_stackedwidget/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/ThresholdBetween_1/Slider'
QtTesting.playCommand(object17, 'set_int', '74')
QtTesting.playCommand(object3, 'activate', '')
object18 = 'pqClientMainWindow/cameraToolbar/actionNegativeX'
QtTesting.playCommand(object18, 'activate', '')
object19 = 'pqClientMainWindow/menubar/menuFilters/pqProxyGroupMenuManager0/Cut'
QtTesting.playCommand(object19, 'activate', '')
QtTesting.playCommand(object3, 'activate', '')
object21 = 'pqClientMainWindow/variableToolbar/displayColor/Variables'
QtTesting.playCommand(object21, 'set_string', 'AsH3')
QtTesting.playCommand(object9, 'set_boolean', 'true')
object22 = 'pqClientMainWindow/proxyTabDock/proxyTabWidget/qt_tabwidget_stackedwidget/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/CutFunction/pqImplicitPlaneWidget/show3DWidget'
QtTesting.playCommand(object22, 'set_boolean', 'false')
# DO_IMAGE_COMPARE
QtTestingImage.compareImage(snapshotWidget, 'TestIsoVolume3.png', 300, 300);

QtTesting.playCommand(object7, 'activate', '')

#-------------- NEXT TEST --------------
# Test isovolume on cell-data on a vtkMultiBlockDataset
QtTesting.playCommand(object1, 'activate', 'menuFilters')
QtTesting.playCommand(object4, 'activate', 'PointDataToCellData')
QtTesting.playCommand(object3, 'activate', '')
QtTesting.playCommand(object1, 'activate', 'menuFilters')
QtTesting.playCommand(object4, 'activate', 'IsoVolume')
object22 = 'pqClientMainWindow/proxyTabDock/proxyTabWidget/qt_tabwidget_stackedwidget/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/SelectInputScalars'
QtTesting.playCommand(object22, 'set_string', 'Temp')
QtTesting.playCommand(object16, 'set_int', '25')
QtTesting.playCommand(object17, 'set_int', '72')
QtTesting.playCommand(object3, 'activate', '')
QtTesting.playCommand(object18, 'activate', '')
object24 = 'pqClientMainWindow/cameraToolbar/actionPositiveZ'
QtTesting.playCommand(object24, 'activate', '')
QtTesting.playCommand(object21, 'set_string', 'Temp')
QtTesting.playCommand(object9, 'set_boolean', 'true')
# DO_IMAGE_COMPARE
QtTestingImage.compareImage(snapshotWidget, 'TestIsoVolume4.png', 300, 300);
QtTesting.playCommand(object7, 'activate', '')
QtTesting.playCommand(object7, 'activate', '')
QtTesting.playCommand(object7, 'activate', '')
QtTesting.playCommand(object7, 'activate', '')

#-------------- NEXT TEST --------------
# Test isovolume on cell-data on a AMR dataset
QtTesting.playCommand(object1, 'activate', 'menu_File')
QtTesting.playCommand(object11, 'activate', 'actionFileOpen')
QtTesting.playCommand(object12, 'filesSelected', '$PARAVIEW_DATA_ROOT/Data/amr/spcth.0')
object25 = 'pqClientMainWindow/proxyTabDock/proxyTabWidget/qt_tabwidget_stackedwidget/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/CellArrayStatus'
QtTesting.playCommand(object25, 'setCurrent', '0.0')
object26 = 'pqClientMainWindow/proxyTabDock/proxyTabWidget/qt_tabwidget_stackedwidget/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/CellArrayStatus/1QHeaderView0'
QtTesting.playCommand(object26, 'mousePress', '1,1,0,0,0,0')
QtTesting.playCommand(object26, 'mouseRelease', '1,0,0,0,0,0')
QtTesting.playCommand(object3, 'activate', '')
object20 = 'pqClientMainWindow/pipelineBrowserDock/pipelineBrowser'
QtTesting.playCommand(object20, 'mousePress', '1,1,0,7,11,/0:0/0:1')
QtTesting.playCommand(object20, 'mouseRelease', '1,0,0,7,11,/0:0/0:1')
QtTesting.playCommand(object1, 'activate', 'menuFilters')
object27 = 'pqClientMainWindow/menubar/menuFilters/Recent'
QtTesting.playCommand(object27, 'activate', 'IsoVolume')
QtTesting.playCommand(object22, 'set_string', 'Material volume fraction - 1')
QtTesting.playCommand(object16, 'set_int', '44')
QtTesting.playCommand(object17, 'set_int', '77')
QtTesting.playCommand(object3, 'activate', '')
#QtTesting.playCommand(object21, 'set_string', 'Material volume fraction - 1 (partial)')
#QtTesting.playCommand(object9, 'set_boolean', 'true')
# DO_IMAGE_COMPARE
QtTestingImage.compareImage(snapshotWidget, 'TestIsoVolume5.png', 300, 300);
QtTesting.playCommand(object7, 'activate', '')
QtTesting.playCommand(object7, 'activate', '')
