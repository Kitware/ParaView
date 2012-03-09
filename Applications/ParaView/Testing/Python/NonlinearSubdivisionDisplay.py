#/usr/bin/env python

import QtTesting
import QtTestingImage

# Always render on server.  Nonlinear wireframes do not work when delivering to client (bug #10676).
object1 = 'pqClientMainWindow/menubar'
QtTesting.playCommand(object1, 'activate', 'menu_Edit')
object2 = 'pqClientMainWindow/menubar/menu_Edit'
QtTesting.playCommand(object2, 'activate', 'actionEditSettings')
object3 = 'pqClientMainWindow/ApplicationSettings/PageNames'
QtTesting.playCommand(object3, 'expand', '4.0')
QtTesting.playCommand(object3, 'setCurrent', '4.0.2.0')
object4 = 'pqClientMainWindow/ApplicationSettings/Stack/pqGlobalRenderViewOptions/stackedWidget/Server/compositingParameters/compositeThreshold'
QtTesting.playCommand(object4, 'set_int', '0')
object5 = 'pqClientMainWindow/ApplicationSettings/CloseButton'
QtTesting.playCommand(object5, 'activate', '')

# Load the file.
object6 = 'pqClientMainWindow/MainControlsToolbar/actionOpenData'
QtTesting.playCommand(object6, 'activate', '')
object7 = 'pqClientMainWindow/FileOpenDialog'
QtTesting.playCommand(object7, 'filesSelected', '$PARAVIEW_DATA_ROOT/Data/quadraticTetra01.vtu')
object8 = 'pqClientMainWindow/objectInspectorDock/objectInspector/Accept'
QtTesting.playCommand(object8, 'activate', '')

# Edit the color map to capture negative values that occur in the nonlinear interpolation.
#object9 = 'pqClientMainWindow/variableToolbar/actionEditColorMap'
#QtTesting.playCommand(object9, 'activate', '')
#object10 = 'pqClientMainWindow/pqColorScaleDialog/ColorTabs/qt_tabwidget_stackedwidget/ScalePage/UseAutoRescale'
#QtTesting.playCommand(object10, 'set_boolean', 'false')
##object11 = 'pqClientMainWindow/pqColorScaleDialog/ColorTabs/qt_tabwidget_stackedwidget/ScalePage/RescaleButton'
##QtTesting.playCommand(object11, 'activate', '')
##object12 = 'pqClientMainWindow/pqColorScaleDialog/pqRescaleRangeDialog/MinimumScalar'
##QtTesting.playCommand(object12, 'set_string', '-.5')
##object13 = 'pqClientMainWindow/pqColorScaleDialog/pqRescaleRangeDialog/RescaleButton'
##QtTesting.playCommand(object13, 'activate', '')
#object14 = 'pqClientMainWindow/pqColorScaleDialog/CloseButton'
#QtTesting.playCommand(object14, 'activate', '')

# Change to surface with edges rendering mode
object15 = 'pqClientMainWindow/representationToolbar/displayRepresentation/comboBox'
QtTesting.playCommand(object15, 'set_string', 'Surface With Edges')
object16 = 'pqClientMainWindow/1QTabBar1'
QtTesting.playCommand(object16, 'set_tab_with_text', 'Display')

# Change subdivision and capture images.
# object17 = 'pqClientMainWindow/displayDock/displayWidgetFrame/displayScrollArea/qt_scrollarea_vcontainer/1QScrollBar0'
# QtTesting.playCommand(object17, 'mousePress', '1,1,0,9,64')
# QtTesting.playCommand(object17, 'mouseMove', '1,0,0,7,175')
# QtTesting.playCommand(object17, 'mouseRelease', '1,0,0,7,175')
subdivisionWidget = 'pqClientMainWindow/displayDock/displayWidgetFrame/displayScrollArea/qt_scrollarea_viewport/displayWidget/pqDisplayProxyEditor/StyleGroup/NonlinearSubdivisionLevel'
snapshotWidget = 'pqClientMainWindow/centralwidget/MultiViewWidget/CoreWidget/qt_tabwidget_stackedwidget/MultiViewWidget1/Frame.0/Viewport'

QtTesting.playCommand(subdivisionWidget, 'set_int', '0')
QtTestingImage.compareImage(snapshotWidget, 'NonlinearSubdivision0Display.png', 300, 300);

QtTesting.playCommand(subdivisionWidget, 'set_int', '1')
QtTestingImage.compareImage(snapshotWidget, 'NonlinearSubdivision1Display.png', 300, 300);

QtTesting.playCommand(subdivisionWidget, 'set_int', '2')
QtTestingImage.compareImage(snapshotWidget, 'NonlinearSubdivision2Display.png', 300, 300);

QtTesting.playCommand(subdivisionWidget, 'set_int', '3')
QtTestingImage.compareImage(snapshotWidget, 'NonlinearSubdivision3Display.png', 300, 300);

QtTesting.playCommand(subdivisionWidget, 'set_int', '4')
QtTestingImage.compareImage(snapshotWidget, 'NonlinearSubdivision4Display.png', 300, 300);

# Restore rendering mode for any subsequent tests.
QtTesting.playCommand(object1, 'activate', 'menu_Edit')
QtTesting.playCommand(object2, 'activate', 'actionEditSettings')
QtTesting.playCommand(object4, 'set_int', '30')
QtTesting.playCommand(object5, 'activate', '')
