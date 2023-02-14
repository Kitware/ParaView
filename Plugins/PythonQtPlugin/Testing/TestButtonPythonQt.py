import paraview.simple as smp

import PythonQt
from PythonQt import QtGui

def getMainWindow():
    for widget in QtGui.QApplication.topLevelWidgets():
        if isinstance(widget, PythonQt.private.ParaViewMainWindow):
            return widget

def makeCylinder():
    smp.Cylinder()
    smp.Show()
    smp.ResetCamera()
    smp.Render()

mainWindow = getMainWindow()
button = QtGui.QPushButton('push button')
button.connect('clicked()', makeCylinder)
button.show()

QtGui.QMessageBox.information(getMainWindow(), 'Hello', 'Hello PythonQt!')
