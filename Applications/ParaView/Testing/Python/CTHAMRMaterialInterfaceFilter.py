#/usr/bin/env python

import QtTesting
import QtTestingImage

QtTesting.playCommand(object1, 'activate', '')
QtTesting.playCommand(object2, 'filesSelected', '$PARAVIEW_DATA_ROOT/Testing/Data/SPCTH/Dave_Karelitz_Small/spcth_a')
QtTesting.playCommand(object3, 'mousePress', '1,1,0,0,0,0')
QtTesting.playCommand(object3, 'mouseMove', '1,0,0,0,0,0')
QtTesting.playCommand(object3, 'mouseRelease', '1,0,0,0,0,0')
QtTesting.playCommand(object4, 'activate', '')
QtTesting.playCommand(object10, 'activate', 'menuFilters')
QtTesting.playCommand(object11, 'activate', 'MaterialInterfaceFilter')
object23 = 'pqClientMainWindow/proxyTabDock/proxyTabWidget/qt_tabwidget_stackedwidget/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/SelectMassArray/1QHeaderView0'
QtTesting.playCommand(object23, 'mousePress', '1,1,0,0,0,0')
QtTesting.playCommand(object23, 'mouseRelease', '1,0,0,0,0,0')
QtTesting.playCommand(object4, 'activate', '')
# DO_IMAGE_COMPARE
snapshotWidget = 'pqClientMainWindow/centralwidget/MultiViewWidget/CoreWidget/qt_tabwidget_stackedwidget/MultiViewWidget1/Container/Frame.0/Viewport'
QtTestingImage.compareImage(snapshotWidget, 'CTHAMRMaterialInterfaceFilter.png', 300, 300)
