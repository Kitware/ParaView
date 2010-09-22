#/usr/bin/env python

import QtTesting
import QtTestingImage

#############################################################################
# Load the SLAC pic-example data and fields.

object2 = 'pqClientMainWindow/MainControlsToolbar/actionOpenData'
QtTesting.playCommand(object2, 'activate', '')
object3 = 'pqClientMainWindow/FileOpenDialog'
QtTesting.playCommand(object3, 'filesSelected', '$PARAVIEW_DATA_ROOT/Data/SLAC/pic-example/mesh.ncdf')
object4 = 'pqClientMainWindow/pqSelectReaderDialog/listWidget'
QtTesting.playCommand(object4, 'mousePress', '1,1,0,66,16,/2:0')
QtTesting.playCommand(object4, 'mouseRelease', '1,0,0,66,16,/2:0')
object5 = 'pqClientMainWindow/pqSelectReaderDialog/okButton'
QtTesting.playCommand(object5, 'activate', '')
object6 = 'pqClientMainWindow/proxyTabDock/proxyTabWidget/qt_tabwidget_stackedwidget/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/ModeFileName/FileButton'
QtTesting.playCommand(object6, 'activate', '')
object7 = 'pqClientMainWindow/proxyTabDock/proxyTabWidget/qt_tabwidget_stackedwidget/objectInspector/ScrollArea/qt_scrollarea_viewport/PanelArea/Editor/ModeFileName/pqFileDialog'
QtTesting.playCommand(object7, 'filesSelected', '$PARAVIEW_DATA_ROOT/Data/SLAC/pic-example/fields_..mod')
object8 = 'pqClientMainWindow/proxyTabDock/proxyTabWidget/qt_tabwidget_stackedwidget/objectInspector/Accept'
QtTesting.playCommand(object8, 'activate', '')
object9 = 'pqClientMainWindow/variableToolbar/displayColor/Variables'
QtTesting.playCommand(object9, 'set_string', 'efield')

# Image compare for default colors.
snapshotWidget = 'pqClientMainWindow/centralwidget/MultiViewManager/SplitterFrame/MultiViewSplitter/0/Viewport'
QtTestingImage.compareImage(snapshotWidget, 'RenderNanDefaultColor.png', 300, 300);

#############################################################################
# Choose a preset color map and make sure the corresponding NaN color is loaded.

object1 = 'pqClientMainWindow/variableToolbar/actionEditColorMap'
QtTesting.playCommand(object1, 'activate', '')
object2 = 'pqClientMainWindow/pqColorScaleDialog/ColorTabs/qt_tabwidget_stackedwidget/ScalePage/PresetButton'
QtTesting.playCommand(object2, 'activate', '')
object3 = 'pqClientMainWindow/pqColorScaleDialog/pqColorPresetDialog/Gradients'
QtTesting.playCommand(object3, 'setCurrent', '1.0')
object4 = 'pqClientMainWindow/pqColorScaleDialog/pqColorPresetDialog/OkButton'
QtTesting.playCommand(object4, 'activate', '')
object5 = 'pqClientMainWindow/pqColorScaleDialog/CloseButton'
QtTesting.playCommand(object5, 'activate', '')

# Image compare for preset color.
snapshotWidget = 'pqClientMainWindow/centralwidget/MultiViewManager/SplitterFrame/MultiViewSplitter/0/Viewport'
QtTestingImage.compareImage(snapshotWidget, 'RenderNanPresetColor.png', 300, 300);


#############################################################################
# Change the NaN color via the button in the color scale edit dialog.

object1 = 'pqClientMainWindow/variableToolbar/actionEditColorMap'
QtTesting.playCommand(object1, 'activate', '')
object2 = 'pqClientMainWindow/pqColorScaleDialog/ColorTabs/qt_tabwidget_stackedwidget/ScalePage/NanColor'
QtTesting.playCommand(object2, 'setChosenColor', '255,0,255')
object3 = 'pqClientMainWindow/pqColorScaleDialog/CloseButton'
QtTesting.playCommand(object3, 'activate', '')

# Image compare for color selected via GUI.
snapshotWidget = 'pqClientMainWindow/centralwidget/MultiViewManager/SplitterFrame/MultiViewSplitter/0/Viewport'
QtTestingImage.compareImage(snapshotWidget, 'RenderNanGUIColor.png', 300, 300);
