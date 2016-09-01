#/usr/bin/env python

import QtTesting

object1 = 'pqClientMainWindow/menubar/menuSources'
QtTesting.playCommand(object1, 'activate', 'RTAnalyticSource')
object2 = 'pqClientMainWindow/propertiesDock/propertiesPanel/Accept'
QtTesting.playCommand(object2, 'activate', '')
object3 = 'pqClientMainWindow/menubar/menuFilters/Common'
QtTesting.playCommand(object3, 'activate', 'Contour')
QtTesting.playCommand(object2, 'activate', '')
object5 = 'pqClientMainWindow/propertiesDock/propertiesPanel/scrollArea/qt_scrollarea_viewport/scrollAreaWidgetContents/PropertiesFrame/ProxyPanel/ContourValues/ScalarValueList'
QtTesting.setProperty(object5, 'scalars', '120')
QtTesting.playCommand(object2, 'activate', '')
QtTesting.setProperty(object5, 'scalars', '120;130;140;150')
QtTesting.playCommand(object2, 'activate', '')
object1 = 'pqClientMainWindow/menubar/menu_Edit'
QtTesting.playCommand(object1, 'activate', 'actionEditUndo')
# Need to wait a moment to allow the GUI to update.
import time
time.sleep(1)
val = QtTesting.getProperty(object5, 'scalars')

if val != "120":
    import exceptions
    raise exceptions.RuntimeError ("Expecting 120, received: " + val)
else:
    print ("Value comparison successful -- Test passed.")
