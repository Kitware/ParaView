#/usr/bin/env python

import QtTesting
import QtTestingImage

object1 = 'MainWindow/mainToolBar/1QToolButton2'
QtTesting.playCommand(object1, 'activate', '')
object2 = 'MainWindow/ServerStartupBrowser/connect'
QtTesting.playCommand(object2, 'activate', '')
object3 = 'MainWindow/mainToolBar/1QToolButton0'
QtTesting.playCommand(object3, 'activate', '')
object4 = 'MainWindow/FileOpenDialog'
QtTesting.playCommand(object4, 'filesSelected', '$PARAVIEW_DATA_ROOT/Data/tube.exo')
object5 = 'MainWindow/objectInspectorDock/1pqProxyTabWidget0/1QStackedWidget0/objectInspector/Accept'
QtTesting.playCommand(object5, 'activate', '')
object6 = 'MainWindow/objectInspectorDock/1pqProxyTabWidget0/1QStackedWidget0/objectInspector/ScrollArea/1QScrollBar0'

object7 = 'MainWindow/objectInspectorDock/1pqProxyTabWidget0/1QStackedWidget0/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/XMLFileName/FileLineEdit'

print 'TODO: Add tests to turn materials, hierarchies on/off.  \nTesting framework needs enhanced first.'
# for now, we'll just check that we have an XML file
xmlFilename = QtTesting.getProperty(object7, "text")
if(xmlFilename == ''):
  raise ValueError('XML file not read in')

snapshotWidget = 'MainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/0/Viewport'
QtTestingImage.compareImage(snapshotWidget, 'ExodusXML.png', 300, 300);



