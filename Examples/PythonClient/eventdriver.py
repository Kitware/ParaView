from PyQt4 import QtCore, QtNetwork, QtGui

def exit():
    import sys
    sys.exit(0)

def sendMessage():
    global sock, timer

    bl = QtCore.QByteArray()
    out = QtCore.QDataStream(bl, QtCore.QIODevice.WriteOnly)
    out.writeDouble(10)

    sock.write(bl)

    timer.start(30)


app = QtGui.QApplication(['Event Driver'])

sock = QtNetwork.QTcpSocket()
sock.connectToHost("localhost", 12345)
sock.disconnected.connect(exit)

timer = QtCore.QTimer()
timer.setSingleShot(True)
timer.timeout.connect(sendMessage)
timer.start(30)

app.exec_()
