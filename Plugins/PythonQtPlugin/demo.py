import os
import PythonQt
from PythonQt import QtGui, QtCore
import paraview.simple as smp



def getPVApplicationCore():
    return PythonQt.paraview.pqPVApplicationCore.instance()


def getPVSettings():
    return getPVApplicationCore().settings()


def getServerManagerModel():
    return getPVApplicationCore().getServerManagerModel()


def getPQView(view):
    model = app.getServerManagerModel()
    return PythonQt.paraview.pqPythonQtMethodHelpers.findProxyItem(model, view.SMProxy)


def getRenderView():
    renderView = smp.GetRenderView()
    return getPQView(renderView)


def findQObjectByName(widgets, name):
    for w in widgets:
        if w.objectName == name:
            return w


def getMainWindow():
    topLevelWidgets = QtGui.QApplication.topLevelWidgets()
    for widget in QtGui.QApplication.topLevelWidgets():
        if isinstance(widget, PythonQt.private.ParaViewMainWindow):
            return widget
    #
    # alternate implementation:
    #return findQObjectByName(QtGui.QApplication.topLevelWidgets(), 'pqClientMainWindow')


def testButton():
    def makeSphere():
        smp.Sphere()
        smp.Show()
        smp.ResetCamera()
        smp.Render()
    global button
    button = QtGui.QPushButton('sphere')
    button.connect('clicked()', makeSphere)
    button.show()
    # note, the button was assigned to a global variable so that the
    # reference is not deleted when this function returns


def sayHello():
    QtGui.QMessageBox.information(getMainWindow(), 'Hello PythonQt!')


def testUserInput():
    fileName = QtGui.QFileDialog.getOpenFileName(getMainWindow(), 'Open file',)
    if fileName:
        smp.OpenDataFile(fileName, guiName=os.path.basename(fileName))
        smp.Show()
        smp.ResetCamera()
        smp.Render()
