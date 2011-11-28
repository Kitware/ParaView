#/usr/bin/env python

import QtTesting
import time

# objects on test widget
openDialog = 'main/OpenDialog'
returnLabel = 'main/EmitLabel'
filter = 'main/FileFilter'
mode = 'main/FileMode'
connection = 'main/ConnectionMode'

#objects in file dialog
fdName = 'main/pqFileDialog/mainSplitter/widget/FileName'
fdCancel = 'main/pqFileDialog/mainSplitter/widget/Cancel'
fdOk = 'main/pqFileDialog/mainSplitter/widget/OK'
fdFiles = 'main/pqFileDialog/mainSplitter/widget/Files'
fdUp = 'main/pqFileDialog/NavigateUp'
fdMsgOk = 'main/pqFileDialog/1QMessageBox0/qt_msgbox_buttonbox/1QPushButton0'

def checkFile(f):
  time.sleep(1)
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
  QtTesting.playCommand(fdName, 'set_string', 'Filea.png')
  QtTesting.playCommand(fdOk, 'activate', '')
  QtTesting.playCommand(fdMsgOk, 'activate', '')
  checkFile('Filea.png')
  QtTesting.playCommand(filter, 'set_string', '*.png')
  QtTesting.playCommand(mode, 'set_string', 'Existing File')
  QtTesting.playCommand(openDialog, 'activate', '')
  QtTesting.playCommand(fdFiles, 'currentChanged', '/0|0')
  QtTesting.playCommand(fdFiles, 'currentChanged', '/4|0')
  QtTesting.playCommand(fdOk, 'activate', '')
  checkFile('Fileb.png')
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
  QtTesting.playCommand(fdFiles, 'currentChanged', '/3|0')
  QtTesting.playCommand(fdName, 'set_string', 'Filec.png')
  QtTesting.playCommand(fdOk, 'activate', '')
  checkFile('Filec.png')
  QtTesting.playCommand(mode, 'set_string', 'Existing File')
  QtTesting.playCommand(openDialog, 'activate', '')
  QtTesting.playCommand(fdName, 'set_string', 'Filea')
  QtTesting.playCommand(fdOk, 'activate', '')
  QtTesting.playCommand(fdName, 'set_string', 'Filea.png')
  QtTesting.playCommand(fdOk, 'activate', '')
  QtTesting.playCommand(filter, 'set_string', 'Images (*.png;*.bmp)')
  QtTesting.playCommand(openDialog, 'activate', '')
  QtTesting.playCommand(fdFiles, 'currentChanged', '/0|0')
  QtTesting.playCommand(fdFiles, 'currentChanged', '/7|0')
  QtTesting.playCommand(fdOk, 'activate', '')
  checkFile('Filec.bmp')
  # switch to directory mode
  QtTesting.playCommand(filter, 'set_string', '')
  QtTesting.playCommand(mode, 'set_string', 'Directory')
  QtTesting.playCommand(openDialog, 'activate', '')
  QtTesting.playCommand(fdFiles, 'currentChanged', '/0|0')
  QtTesting.playCommand(fdFiles, 'currentChanged', '/3|0')
  QtTesting.playCommand(fdFiles, 'currentChanged', '/4|0')
  QtTesting.playCommand(fdOk, 'activate', '')
  QtTesting.playCommand(fdFiles, 'currentChanged', '/2|0')
  QtTesting.playCommand(fdOk, 'activate', '')
  checkFile('SubDir3')


QtTesting.playCommand(connection, 'set_string', 'Local')
runTests()
QtTesting.playCommand(connection, 'set_string', 'Remote')
runTests()


