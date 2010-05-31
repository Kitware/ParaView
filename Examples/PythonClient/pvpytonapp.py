from PyQt4 import QtCore, QtGui, QtNetwork
from paraview import simple as pvsimple
from vtk.qt4 import QVTKRenderWindowInteractor

class ParaViewClient(QtCore.QObject):

    def __init__(self, port, serverHost=None, serverPort=11111):

        self.tcpServer = QtNetwork.QTcpServer()
        if not self.tcpServer.listen(QtNetwork.QHostAddress.Any, port):
            print 'Could not list on port %d' % port
            return
        self.tcpServer.newConnection.connect(self.acceptClient)

        self.timer = QtCore.QTimer()
        self.timer.setSingleShot(True)
        self.timer.timeout.connect(self.render)

        if serverHost:
            pvsimple.Connect(serverHost, serverPort)
        self.createPipeline()

        self.setupManipulators()

        self.widget = \
            QVTKRenderWindowInteractor.QVTKRenderWindowInteractor(\
            rw=self.renderView.GetRenderWindow(),
            iren=self.renderView.GetInteractor())
        self.widget.Initialize()
        self.widget.Start()
        self.widget.show()

        pvsimple.Render(self.renderView)

    def setupManipulators(self):
        cm = self.renderView.GetProperty("CameraManipulators")
        cm.AddProxy(pvsimple.servermanager.rendering.TrackballRotate().SMProxy)

        zoom = pvsimple.servermanager.rendering.TrackballZoom()
        zoom.Button = 'Right Button'
        cm.AddProxy(zoom.SMProxy)

        pan = pvsimple.servermanager.rendering.TrackballPan1()
        pan.Button = 'Center Button'
        cm.AddProxy(pan.SMProxy)

        pan2 = pvsimple.servermanager.rendering.TrackballPan1()
        pan2.Button = 'Left Button'
        pan2.Shift = True
        pan2.Control = True
        cm.AddProxy(pan2.SMProxy)

        self.renderView.UpdateVTKObjects()

    def createPipeline(self):
        pass

    def acceptClient(self):
        self.connection = self.tcpServer.nextPendingConnection()
        self.connection.readyRead.connect(self.readData)
        self.tcpServer.close()

    def render(self):
        pvsimple.Render(self.renderView)

    def readData(self):
        instr = QtCore.QDataStream(self.connection)
        if self.connection.bytesAvailable >= 8:
            self.renderView.GetActiveCamera().Azimuth(instr.readDouble())
            if not self.timer.isActive():
                self.timer.start(30)
        if self.connection.bytesAvailable() >= 8:
            QtCore.QTimer.singleShot(30, self.readData)

if __name__ == '__main__':
    class SimpleClient(ParaViewClient):
        def createPipeline(self):
            self.renderView = pvsimple.CreateRenderView()

            pvsimple.Sphere()
            pvsimple.Show()
    app = QtGui.QApplication(['ParaView Python App'])
    #s = ParaViewClient(12345, 'localhost')
    s = SimpleClient(12345)
    app.exec_()
