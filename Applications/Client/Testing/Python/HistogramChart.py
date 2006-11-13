#/usr/bin/env python

import QtTesting
import QtTestingImage

object1 = 'MainWindow/menubar/menuFile'
QtTesting.playCommand(object1, 'activate', 'actionFileLoadServerState')
object2 = 'MainWindow/ServerStartupBrowser/connect'
QtTesting.playCommand(object2, 'activate', '')
object3 = 'MainWindow/FileLoadServerStateDialog'
QtTesting.playCommand(object3, 'filesSelected', '$PARAVIEW_DATA_ROOT/Data/LoadStateHistogram.pvsm')
object4 = 'MainWindow/pipelineBrowserDock/pipelineBrowser/PipelineView'
QtTesting.playCommand(object4, 'currentChanged', '/0/0/0|0')
object5 = 'MainWindow/objectInspectorDock/1pqProxyTabWidget0/1QTabBar0'
QtTesting.playCommand(object5, 'set_tab', '1')
QtTesting.playCommand(object5, 'set_tab', '0')
object6 = 'MainWindow/objectInspectorDock/1pqProxyTabWidget0/1QStackedWidget0/1QScrollArea0/qt_scrollarea_viewport/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/BinCount'
QtTesting.playCommand(object6, 'set_string', '1')
QtTesting.playCommand(object6, 'set_string', '16')
object7 = 'MainWindow/objectInspectorDock/1pqProxyTabWidget0/1QStackedWidget0/1QScrollArea0/qt_scrollarea_viewport/objectInspector/Accept'
QtTesting.playCommand(object7, 'activate', '')

object8 = 'MainWindow/HistogramViewModule1/1pqHistogramWidget0'
QtTestingImage.compareImage(object8, 'HistogramChart.png', 400, 200)

