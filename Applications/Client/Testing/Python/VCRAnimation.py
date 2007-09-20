#/usr/bin/env python

import QtTesting
import QtTestingImage
import time

object1 = 'MainWindow/mainToolBar/1QToolButton0'
QtTesting.playCommand(object1, 'activate', '')
object3 = 'MainWindow/FileOpenDialog'
QtTesting.playCommand(object3, 'filesSelected', '$PARAVIEW_DATA_ROOT/Data/dualSphereAnimation.pvd')
object4 = 'MainWindow/objectInspectorDock/1pqProxyTabWidget0/1QStackedWidget0/objectInspector/Accept'
QtTesting.playCommand(object4, 'activate', '')
object5 = 'MainWindow/VCRToolbar/1QToolButton2'
QtTesting.playCommand(object5, 'activate', '')

while QtTesting.getProperty(object5, 'text') != 'Play':
  time.sleep(0.2)

snapshotWidget = 'MainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/0/Viewport'
QtTestingImage.compareImage(snapshotWidget, 'VCRAnimation.png', 300, 300);

