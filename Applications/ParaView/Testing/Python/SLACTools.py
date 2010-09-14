#/usr/bin/env python

import QtTesting
import QtTestingImage


#############################################################################
# Load the SLACTools plugin.

object1 = 'pqClientMainWindow/menubar'
QtTesting.playCommand(object1, 'activate', 'menuTools')
object2 = 'pqClientMainWindow/menubar/menuTools'
QtTesting.playCommand(object2, 'activate', 'actionManage_Plugins')

object3 = 'pqClientMainWindow/PluginManagerDialog/localGroup/localPlugins'
QtTesting.playCommand(object3, 'setCurrent', 'SLACTools')
object4 = 'pqClientMainWindow/PluginManagerDialog/localGroup/loadSelected_Local'
QtTesting.playCommand(object4, 'activate', '')

object5 = 'pqClientMainWindow/PluginManagerDialog/buttonBox/1QPushButton0'
QtTesting.playCommand(object5, 'activate', '')


#############################################################################
# Load the pic-example SLAC files.

object6 = 'pqSLACActionHolder/actionDataLoadManager'
QtTesting.playCommand(object6, 'activate', '')

object7 = 'pqClientMainWindow/pqSLACDataLoadManager/meshFile/FileButton'
QtTesting.playCommand(object7, 'activate', '')
object8 = 'pqClientMainWindow/pqSLACDataLoadManager/meshFile/pqFileDialog'
QtTesting.playCommand(object8, 'filesSelected', '$PARAVIEW_DATA_ROOT/Data/SLAC/pic-example/mesh.ncdf')

object9 = 'pqClientMainWindow/pqSLACDataLoadManager/modeFile/FileButton'
QtTesting.playCommand(object9, 'activate', '')
object8 = 'pqClientMainWindow/pqSLACDataLoadManager/modeFile/pqFileDialog'
QtTesting.playCommand(object8, 'filesSelected', '$PARAVIEW_DATA_ROOT/Data/SLAC/pic-example/fields_..mod')

object10 = 'pqClientMainWindow/pqSLACDataLoadManager/particlesFile/FileButton'
QtTesting.playCommand(object10, 'activate', '')
object8 = 'pqClientMainWindow/pqSLACDataLoadManager/particlesFile/pqFileDialog'
QtTesting.playCommand(object8, 'filesSelected', '$PARAVIEW_DATA_ROOT/Data/SLAC/pic-example/particles_..ncdf')

object11 = 'pqClientMainWindow/pqSLACDataLoadManager/buttonBox/1QPushButton0'
QtTesting.playCommand(object11, 'activate', '')

#Image compare
snapshotWidget = 'pqClientMainWindow/centralwidget/MultiViewManager/SplitterFrame/MultiViewSplitter/0/Viewport'
# For some reason, the snapshot is being grabbed before the image is updated
# when the dialog closes.  To get around this issue, simulate a mouse click
# in the view.  I'm not really happy with this hack because it won't catch if
# accepting the dialog really doesn't render the final view.
QtTesting.playCommand(snapshotWidget, 'mousePress', '(0.5,0.5,1,1,0)')
QtTesting.playCommand(snapshotWidget, 'mouseRelease', '(0.5,0.5,1,0,0)')
QtTestingImage.compareImage(snapshotWidget, 'SLACToolsInitialLoad.png', 300, 300);


#############################################################################
# Show the magnetic (b) field and change the representation to solid+wireframe.

object1 = 'pqSLACActionHolder/actionShowBField'
QtTesting.playCommand(object1, 'activate', '')

object2 = 'pqSLACActionHolder/actionWireframeSolidMesh'
QtTesting.playCommand(object2, 'activate', '')

#Image compare
snapshotWidget = 'pqClientMainWindow/centralwidget/MultiViewManager/SplitterFrame/MultiViewSplitter/0/Viewport'
QtTestingImage.compareImage(snapshotWidget, 'SLACToolsBFieldWireSolid.png', 300, 300);

#############################################################################
# Rotate the camera, show front wireframe + solid back, show electric (e)
# field scaled to all time steps, turn off particles.

# Make window 300x300
object1 = 'pqClientMainWindow/menubar'
QtTesting.playCommand(object1, 'activate', 'menuTools')
object2 = 'pqClientMainWindow/menubar/menuTools'
QtTesting.playCommand(object2, 'activate', 'actionTesting_Window_Size')

object3 = 'pqClientMainWindow/centralwidget/MultiViewManager/SplitterFrame/MultiViewSplitter/0/Viewport'
QtTesting.playCommand(object3, 'mousePress', '(0.863333,0.543333,1,1,0)')
QtTesting.playCommand(object3, 'mouseMove', '(0.743333,0.633333,1,0,0)')
QtTesting.playCommand(object3, 'mouseRelease', '(0.743333,0.633333,1,0,0)')

# Restore window
QtTesting.playCommand(object1, 'activate', 'menuTools')
QtTesting.playCommand(object2, 'activate', 'actionTesting_Window_Size')

object4 = 'pqSLACActionHolder/actionWireframeAndBackMesh'
QtTesting.playCommand(object4, 'activate', '')

object5 = 'pqSLACActionHolder/actionShowEField'
QtTesting.playCommand(object5, 'activate', '')

object6 = 'pqSLACActionHolder/actionTemporalResetRange'
QtTesting.playCommand(object6, 'activate', '')

object7 = 'pqSLACActionHolder/actionShowParticles'
QtTesting.playCommand(object7, 'set_boolean', 'false')

#Image compare
snapshotWidget = 'pqClientMainWindow/centralwidget/MultiViewManager/SplitterFrame/MultiViewSplitter/0/Viewport'
QtTestingImage.compareImage(snapshotWidget, 'SLACToolsWireEFieldTempResetHideP.png', 300, 300);

#############################################################################
# Restore black background, restore solid, restore particles, restore camera

object1 = 'pqSLACActionHolder/actionToggleBackgroundBW'
QtTesting.playCommand(object1, 'activate', '')

object2 = 'pqSLACActionHolder/actionSolidMesh'
QtTesting.playCommand(object2, 'activate', '')

object3 = 'pqSLACActionHolder/actionShowParticles'
QtTesting.playCommand(object3, 'set_boolean', 'true')

object5 = 'pqSLACActionHolder/actionShowStandardViewpoint'
QtTesting.playCommand(object5, 'activate', '')

#Image compare
snapshotWidget = 'pqClientMainWindow/centralwidget/MultiViewManager/SplitterFrame/MultiViewSplitter/0/Viewport'
QtTestingImage.compareImage(snapshotWidget, 'SLACToolsBlackBackReset.png', 300, 300);

#############################################################################
# Plot over Z axis, change time.

object6 = 'pqSLACActionHolder/actionPlotOverZ'
QtTesting.playCommand(object6, 'activate', '')

object7 = 'pqClientMainWindow/VCRToolbar/actionVCRNextFrame'
QtTesting.playCommand(object7, 'activate', '')

#Image compare
snapshotWidget = 'pqClientMainWindow/centralwidget/MultiViewManager/SplitterFrame/MultiViewSplitter/1/1QVTKWidget0'
QtTestingImage.compareImage(snapshotWidget, 'SLACToolsPlotOverZ.png', 300, 300);
