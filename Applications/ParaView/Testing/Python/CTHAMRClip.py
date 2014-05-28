#/usr/bin/env python

import QtTesting
import QtTestingImage

object1 = 'pqClientMainWindow/MainControlsToolbar/actionOpenData'
QtTesting.playCommand(object1, 'activate', '')
object2 = 'pqClientMainWindow/FileOpenDialog'
QtTesting.playCommand(object2, 'filesSelected', '$PARAVIEW_DATA_ROOT/SPCTH/Dave_Karelitz_Small/spcth_a')
object3 = 'pqClientMainWindow/proxyTabDock/proxyTabWidget/qt_tabwidget_stackedwidget/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/CellArrayStatus/1QHeaderView0'
QtTesting.playCommand(object3, 'mousePress', '1,1,0,0,0,0')
QtTesting.playCommand(object3, 'mouseRelease', '1,0,0,0,0,0')
object4 = 'pqClientMainWindow/proxyTabDock/proxyTabWidget/qt_tabwidget_stackedwidget/objectInspector/Accept'
QtTesting.playCommand(object4, 'activate', '')
object5 = 'pqClientMainWindow/representationToolbar/displayRepresentation/comboBox'
QtTesting.playCommand(object5, 'set_string', 'Surface')
object6 = 'pqClientMainWindow/variableToolbar/displayColor/Variables'
QtTesting.playCommand(object6, 'set_string', 'Pressure (dynes/cm^2^)')
object7 = 'pqClientMainWindow/cameraToolbar/actionPositiveX'
QtTesting.playCommand(object7, 'activate', '')
object8 = 'pqClientMainWindow/menubar/menuFilters/pqProxyGroupMenuManager0/Cut'
QtTesting.playCommand(object8, 'activate', '')
QtTesting.playCommand(object4, 'activate', '')
object9 = 'pqClientMainWindow/proxyTabDock/proxyTabWidget/qt_tabwidget_stackedwidget/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/CutFunction/pqImplicitPlaneWidget/show3DWidget'
QtTesting.playCommand(object9, 'set_boolean', 'false')
# DO_IMAGE_COMPARE
snapshotWidget = 'pqClientMainWindow/centralwidget/MultiViewWidget/CoreWidget/qt_tabwidget_stackedwidget/MultiViewWidget1/Frame.0/Viewport'
QtTestingImage.compareImage(snapshotWidget, 'CTHAMRClip.png', 300, 300)
