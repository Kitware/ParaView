#/usr/bin/env python

import QtTesting
import QtTestingImage

QtTesting.playCommand(object1, 'activate', '')
QtTesting.playCommand(object2, 'filesSelected', '$PARAVIEW_DATA_ROOT/Testing/Data/SPCTH/Dave_Karelitz_Small/spcth_a')
QtTesting.playCommand(object3, 'mousePress', '1,1,0,0,0,0')
QtTesting.playCommand(object3, 'mouseRelease', '1,0,0,0,0,0')
QtTesting.playCommand(object4, 'activate', '')
QtTesting.playCommand(object5, 'set_string', 'Surface')
QtTesting.playCommand(object6, 'activate', '')
QtTesting.playCommand(object7, 'set_string', 'Pressure (dynes/cm^2^)')
QtTesting.playCommand(object8, 'activate', 'menuFilters')
QtTesting.playCommand(object9, 'activate', 'AMRDualClip')
QtTesting.playCommand(object10, 'mousePress', '1,1,0,0,0,0')
QtTesting.playCommand(object10, 'mouseRelease', '1,0,0,0,0,0')
QtTesting.playCommand(object4, 'activate', '')
QtTesting.playCommand(object7, 'set_string', 'Pressure (dynes/cm^2^) (partial)')
# DO_IMAGE_COMPARE
snapshotWidget = 'pqClientMainWindow/centralwidget/MultiViewWidget/CoreWidget/qt_tabwidget_stackedwidget/MultiViewWidget1/Container/Frame.0/Viewport'
QtTestingImage.compareImage(snapshotWidget, 'CTHAMRDualClip.png', 300, 300)
