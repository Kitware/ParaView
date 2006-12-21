#/usr/bin/env python

import QtTesting

# objects on test widget
openDialog = 'main/OpenDialog'
returnLabel = 'main/ReturnLabel'
filter = 'main/FileFilter'
mode = 'main/FileMode'
connection = 'main/ConnectionMode'

#objects in file dialog
fdName = 'main/pqFileDialog/FileName'
fdCancel = 'main/pqFileDialog/Cancel'
fdOk = 'main/pqFileDialog/OK'
fdFiles = 'main/pqFileDialog/Files'
fdUp = 'main/pqFileDialog/NavigateUp'
fdMsgOk = 'main/pqFileDialog/1QMessageBox0/qt_msgbox_buttonbox/1QPushButton0'

def checkFile(f):
  QtTesting.wait(200)
  text = QtTesting.getProperty(returnLabel, 'text')
  if not text.endswith(f):
    raise ValueError('failed and got ' + text + ' instead of ' + f)

def runTests():
  #  initialize
  QtTesting.playCommand(filter, 'set_string', '')
  QtTesting.playCommand(mode, 'set_string', 'Any File')
  #
  QtTesting.playCommand(openDialog, 'activate', '')
  QtTesting.playCommand(fdCancel, 'activate', '')
  QtTesting.playCommand(filter, 'set_string', 'File[12].?n?')
  QtTesting.playCommand(openDialog, 'activate', '')
  QtTesting.playCommand(fdName, 'set_string', 'File1.png')
  QtTesting.playCommand(fdOk, 'activate', '')
  QtTesting.playCommand(fdMsgOk, 'activate', '')
  checkFile('File1.png')
  QtTesting.playCommand(filter, 'set_string', '*.png')
  QtTesting.playCommand(openDialog, 'activate', '')
  QtTesting.playCommand(fdFiles, 'currentChanged', '/0|0')
  QtTesting.playCommand(fdFiles, 'currentChanged', '/1|0')
  QtTesting.playCommand(fdFiles, 'currentChanged', '/2|0')
  QtTesting.playCommand(fdFiles, 'currentChanged', '/3|0')
  QtTesting.playCommand(fdFiles, 'currentChanged', '/4|0')
  QtTesting.playCommand(fdFiles, 'currentChanged', '/5|0')
  QtTesting.playCommand(fdOk, 'activate', '')
  QtTesting.playCommand(fdMsgOk, 'activate', '')
  checkFile('File3.png')
  QtTesting.playCommand(mode, 'set_string', 'Existing File')
  QtTesting.playCommand(openDialog, 'activate', '')
  QtTesting.playCommand(fdFiles, 'currentChanged', '/0|0')
  QtTesting.playCommand(fdFiles, 'currentChanged', '/4|0')
  QtTesting.playCommand(fdOk, 'activate', '')
  checkFile('File2.png')
  QtTesting.playCommand(openDialog, 'activate', '')
  QtTesting.playCommand(fdFiles, 'currentChanged', '/0|0')
  QtTesting.playCommand(fdFiles, 'currentChanged', '/2|0')
  QtTesting.playCommand(fdOk, 'activate', '')
  QtTesting.playCommand(fdUp, 'activate', '')
  QtTesting.playCommand(fdOk, 'activate', '')
  QtTesting.playCommand(fdUp, 'activate', '')
  QtTesting.playCommand(fdCancel, 'activate', '')
  QtTesting.playCommand(mode, 'set_string', 'Existing Files')
  QtTesting.playCommand(openDialog, 'activate', '')
  QtTesting.playCommand(fdFiles, 'currentChanged', '/0|0')
  QtTesting.playCommand(fdFiles, 'currentChanged', '/5|0')
  QtTesting.playCommand(fdName, 'set_string', 'File3')
  QtTesting.playCommand(fdOk, 'activate', '')
  QtTesting.playCommand(fdOk, 'activate', '')
  QtTesting.playCommand(fdName, 'set_string', 'File3.png')
  QtTesting.playCommand(fdOk, 'activate', '')
  checkFile('File3.png')
  QtTesting.playCommand(mode, 'set_string', 'Existing File')
  QtTesting.playCommand(openDialog, 'activate', '')
  QtTesting.playCommand(fdName, 'set_string', 'File1')
  QtTesting.playCommand(fdOk, 'activate', '')
  QtTesting.playCommand(fdName, 'set_string', 'File1.png')
  QtTesting.playCommand(fdOk, 'activate', '')
  QtTesting.playCommand(filter, 'set_string', 'Images (*.png;*.bmp)')
  QtTesting.playCommand(openDialog, 'activate', '')
  QtTesting.playCommand(fdFiles, 'currentChanged', '/0|0')
  QtTesting.playCommand(fdFiles, 'currentChanged', '/7|0')
  QtTesting.playCommand(fdOk, 'activate', '')
  checkFile('File3.bmp')



QtTesting.playCommand(connection, 'set_string', 'Local')
runTests()
QtTesting.playCommand(connection, 'set_string', 'Remote')
runTests()


