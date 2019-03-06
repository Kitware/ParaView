#/usr/bin/env python

import QtTesting
import QtTestingImage

object1 = 'pqClientMainWindow/MainControlsToolbar/actionOpenData'
QtTesting.playCommand(object1, 'activate', '')
object2 = 'pqClientMainWindow/FileOpenDialog'
QtTesting.playCommand(object2, 'filesSelected', '$PARAVIEW_DATA_ROOT/Testing/Data/SPCTH/Dave_Karelitz_Small/spcth_a')
object3 = 'pqClientMainWindow/proxyTabDock/proxyTabWidget/qt_tabwidget_stackedwidget/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/CellArrayStatus/1QHeaderView0'
QtTesting.playCommand(object3, 'mousePress', '1,1,0,0,0,0')
QtTesting.playCommand(object3, 'mouseRelease', '1,0,0,0,0,0')
object4 = 'pqClientMainWindow/proxyTabDock/proxyTabWidget/qt_tabwidget_stackedwidget/objectInspector/Accept'
QtTesting.playCommand(object4, 'activate', '')
object5 = 'pqClientMainWindow/representationToolbar/displayRepresentation/comboBox'
QtTesting.playCommand(object5, 'set_string', 'Surface')
object6 = 'pqClientMainWindow/cameraToolbar/actionNegativeY'
QtTesting.playCommand(object6, 'activate', '')
object7 = 'pqClientMainWindow/variableToolbar/displayColor/Variables'
QtTesting.playCommand(object7, 'set_string', 'Pressure (dynes/cm^2^)')
object8 = 'pqClientMainWindow/variableToolbar/actionScalarBarVisibility'
QtTesting.playCommand(object8, 'set_boolean', 'false')
# DO_IMAGE_COMPARE
snapshotWidget = 'pqClientMainWindow/centralwidget/MultiViewWidget/CoreWidget/qt_tabwidget_stackedwidget/MultiViewWidget1/Container/Frame.0/Viewport'
QtTestingImage.compareImage(snapshotWidget, 'CTHAMRBaseline.png', 300, 300)


