#/usr/bin/env python

import QtTesting
import QtTestingImage

object1 = 'pqClientMainWindow/menubar/menuFile'
QtTesting.playCommand(object1, 'activate', 'actionFileLoadServerState')
object2 = 'pqClientMainWindow/ServerStartupBrowser/connect'
QtTesting.playCommand(object2, 'activate', '')
object3 = 'pqClientMainWindow/FileLoadServerStateDialog'
QtTesting.playCommand(object3, 'filesSelected', '$PARAVIEW_DATA_ROOT/Data/LoadStateHistogram.pvsm')
object4 = 'pqClientMainWindow/pipelineBrowserDock/pipelineBrowser/PipelineView'
QtTesting.playCommand(object4, 'currentChanged', '/0/0/0|0')
object5 = 'pqClientMainWindow/objectInspectorDock/1pqProxyTabWidget0/1QTabBar0'
QtTesting.playCommand(object5, 'set_tab', '1')
QtTesting.playCommand(object5, 'set_tab', '0')
object6 = 'pqClientMainWindow/objectInspectorDock/1pqProxyTabWidget0/1QStackedWidget0/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/BinCount'
QtTesting.playCommand(object6, 'set_string', '1')
QtTesting.playCommand(object6, 'set_string', '16')
object7 = 'pqClientMainWindow/objectInspectorDock/1pqProxyTabWidget0/1QStackedWidget0/objectInspector/Accept'
QtTesting.playCommand(object7, 'activate', '')

object8 = 'pqClientMainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/1/PlotWidget'
QtTestingImage.compareImage(object8, 'HistogramChart.png', 400, 200)

