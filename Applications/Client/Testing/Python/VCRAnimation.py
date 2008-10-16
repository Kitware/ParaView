#/usr/bin/env python

import QtTesting
import QtTestingImage
import time

object1 = 'pqClientMainWindow/mainToolBar/1QToolButton0'
QtTesting.playCommand(object1, 'activate', '')
object3 = 'pqClientMainWindow/FileOpenDialog'
QtTesting.playCommand(object3, 'filesSelected', '$PARAVIEW_DATA_ROOT/Data/dualSphereAnimation.pvd')
object4 = 'pqClientMainWindow/objectInspectorDock/1pqProxyTabWidget0/1QStackedWidget0/objectInspector/Accept'
QtTesting.playCommand(object4, 'activate', '')
object5 = 'pqClientMainWindow/VCRToolbar/1QToolButton2'
play_text = QtTesting.getProperty(object5, 'text')
print "Will use '%s' for play finished comparison" % play_text
QtTesting.playCommand(object5, 'activate', '')

time.sleep(3);
count = 0
while QtTesting.getProperty(object5, 'text') != play_text:
  print  "Label is '%s' (!= '%s') hence sleeping..." % \
    (QtTesting.getProperty(object5, 'text'), play_text)
  time.sleep(1)
  count = count + 1
  if count == 100:
    print "ERROR: Timing out. Waited for too long for the play button text to "\
      "change to indicate animation playing was over."

snapshotWidget = 'pqClientMainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/0/Viewport'
QtTestingImage.compareImage(snapshotWidget, 'VCRAnimation.png', 300, 300);

