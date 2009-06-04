#/usr/bin/env python

import QtTesting
import QtTestingImage

object1 = 'pqClientMainWindow/menubar/menuFile'
QtTesting.playCommand(object1, 'activate', 'actionFileLoadServerState')
##object2 = 'pqClientMainWindow/ServerStartupBrowser/connect'
#QtTesting.playCommand(object2, 'activate', '')
object3 = 'pqClientMainWindow/FileLoadServerStateDialog'
QtTesting.playCommand(object3, 'filesSelected', '$PARAVIEW_DATA_ROOT/Data/LoadStateMultiView.pvsm')
object4 = 'pqClientMainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/MultiViewSplitter:0/0/MultiViewFrameMenu/MaximizeButton'
QtTesting.playCommand(object4, 'activate', '')
object5 = 'pqClientMainWindow/MultiViewManager/MaximizeFrame/0/MultiViewFrameMenu/RestoreButton'
QtTesting.playCommand(object5, 'activate', '')
object6 = 'pqClientMainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/MultiViewSplitter:0/0/MultiViewFrameMenu/CloseButton'
QtTesting.playCommand(object6, 'activate', '')
object7 = 'pqClientMainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/0/MultiViewFrameMenu/CloseButton'
QtTesting.playCommand(object7, 'activate', '')
object8 = 'pqClientMainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/0/MultiViewFrameMenu/CloseButton'
QtTesting.playCommand(object8, 'activate', '')
object9 = 'pqClientMainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/0/MultiViewFrameMenu/SplitHorizontalButton'
QtTesting.playCommand(object9, 'activate', '')
object10 = 'pqClientMainWindow/pipelineBrowserDock/pipelineBrowser/PipelineView'
QtTesting.playCommand(object10, 'currentChanged', '/0/2|0')
#QtTesting.playCommand(object10, 'currentChanged', '/0/2|1')
object11 = 'pqClientMainWindow/objectInspectorDock/1pqProxyTabWidget0/1QTabBar0'
QtTesting.playCommand(object11, 'set_tab', '1')
QtTesting.playCommand(object1, 'activate', 'actionFileSaveServerState')
object13 = 'pqClientMainWindow/FileSaveServerStateDialog'
QtTesting.playCommand(object13, 'filesSelected', '$PARAVIEW_TEST_ROOT/TestMultiView.pvsm')
object14 = 'pqClientMainWindow/mainToolBar/1QToolButton3'
QtTesting.playCommand(object14, 'activate', '')
object14a ="pqClientMainWindow/1QMessageBox0/qt_msgbox_buttonbox/1QPushButton0" 
QtTesting.playCommand(object14a, 'activate', '')
QtTesting.playCommand(object1, 'activate', 'actionFileLoadServerState')
#QtTesting.playCommand(object2, 'activate', '')
QtTesting.playCommand(object3, 'filesSelected', '$PARAVIEW_TEST_ROOT/TestMultiView.pvsm')

snapshotWidget = 'pqClientMainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/0/Viewport'
QtTestingImage.compareImage(snapshotWidget, 'LoadStateMultiView.png', 200, 200);

