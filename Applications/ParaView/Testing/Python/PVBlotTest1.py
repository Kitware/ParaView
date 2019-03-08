#/usr/bin/env python


blotScriptBody = \
"""
diskwrite off

detour
solid
rotate x 100
tmin 0.001
tmax 0.003
nintv 20
plot

tplot
VELX 1 to 1000 by 200
overlay
plot
reset

xyplot
displx 200 to 300 by 20
velx 200 to 300 by 20
plot

"""


import QtTesting
import sys

pluginFile = 'libpvblot.so'
if sys.platform == 'win32':
    pluginFile = 'pvblot.dll'

dataFile = '$PARAVIEW_DATA_ROOT/Testing/Data/can.ex2'

object1 = 'pqClientMainWindow/menubar/menuTools'
object2 = 'pqClientMainWindow/pqPluginDialog/localGroup/loadLocal'
object3 = 'pqClientMainWindow/pqPluginDialog/pqFileDialog'
object4 = 'pqClientMainWindow/pqPluginDialog/buttonBox/1QPushButton0'
object5 = 'pqClientMainWindow/pqFileDialog'
object6 = 'pqClientMainWindow/pqBlotDialog/buttons/runScript'
object7 = 'pqClientMainWindow/pqBlotDialog/BLOTShellRunScriptDialog'
object8 = 'pqClientMainWindow/pqBlotDialog/buttons/close'


testDir = QtTesting.getProperty('pqClientMainWindow', 'TestDirectory')


blotTestFile = "%s/PVBlotTest1.bl" % testDir

blotFile = open(blotTestFile, 'w')
blotFile.write(blotScriptBody)
blotFile.close()

QtTesting.playCommand(object1, 'activate', 'actionManage_Plugins')
QtTesting.playCommand(object2, 'activate', '')
QtTesting.playCommand(object3, 'filesSelected', pluginFile)
QtTesting.playCommand(object4, 'activate', '')
QtTesting.playCommand(object1, 'activate', 'PVBlot')
QtTesting.playCommand(object5, 'filesSelected', dataFile)
QtTesting.playCommand(object6, 'activate', '')
QtTesting.playCommand(object7, 'filesSelected', blotTestFile)
QtTesting.playCommand(object8, 'activate', '')

import time
time.sleep(10)

