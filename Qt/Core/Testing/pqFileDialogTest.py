#/usr/bin/env python

import QtTesting

object1 = 'main/OpenDialog'
object9 = 'main/pqFileDialog/FileName'
QtTesting.playCommand(object1, 'activate', '')
object2 = 'main/pqFileDialog/Cancel'
QtTesting.playCommand(object2, 'activate', '')
object3 = 'main/FileFilter'
QtTesting.playCommand(object3, 'set_string', 'File[12].?n?')
QtTesting.playCommand(object1, 'activate', '')
QtTesting.playCommand(object9, 'set_string', 'File1.png')
object4 = 'main/pqFileDialog/OK'
QtTesting.playCommand(object4, 'activate', '')
object5 = 'main/pqFileDialog/1QMessageBox0/qt_msgbox_buttonbox/1QPushButton0'
QtTesting.playCommand(object5, 'activate', '')
QtTesting.playCommand(object3, 'set_string', '*.png')
QtTesting.playCommand(object1, 'activate', '')
object6 = 'main/pqFileDialog/Files'
QtTesting.playCommand(object6, 'currentChanged', '/0|0')
QtTesting.playCommand(object6, 'currentChanged', '/1|0')
QtTesting.playCommand(object6, 'currentChanged', '/2|0')
QtTesting.playCommand(object6, 'currentChanged', '/3|0')
QtTesting.playCommand(object6, 'currentChanged', '/4|0')
QtTesting.playCommand(object6, 'currentChanged', '/5|0')
QtTesting.playCommand(object4, 'activate', '')
QtTesting.playCommand(object5, 'activate', '')
object7 = 'main/FileMode'
QtTesting.playCommand(object7, 'set_string', 'Existing File')
QtTesting.playCommand(object1, 'activate', '')
QtTesting.playCommand(object6, 'currentChanged', '/0|0')
QtTesting.playCommand(object6, 'currentChanged', '/4|0')
QtTesting.playCommand(object4, 'activate', '')
QtTesting.playCommand(object1, 'activate', '')
QtTesting.playCommand(object6, 'currentChanged', '/0|0')
QtTesting.playCommand(object6, 'currentChanged', '/2|0')
QtTesting.playCommand(object4, 'activate', '')
object8 = 'main/pqFileDialog/NavigateUp'
QtTesting.playCommand(object8, 'activate', '')
QtTesting.playCommand(object4, 'activate', '')
QtTesting.playCommand(object8, 'activate', '')
QtTesting.playCommand(object2, 'activate', '')
QtTesting.playCommand(object7, 'set_string', 'Existing Files')
QtTesting.playCommand(object1, 'activate', '')
QtTesting.playCommand(object6, 'currentChanged', '/0|0')
QtTesting.playCommand(object6, 'currentChanged', '/5|0')
QtTesting.playCommand(object9, 'set_string', 'File3')
QtTesting.playCommand(object4, 'activate', '')
QtTesting.playCommand(object4, 'activate', '')
QtTesting.playCommand(object9, 'set_string', 'File3.png')
QtTesting.playCommand(object4, 'activate', '')
QtTesting.playCommand(object7, 'set_string', 'Existing File')
QtTesting.playCommand(object1, 'activate', '')
QtTesting.playCommand(object9, 'set_string', 'File1')
QtTesting.playCommand(object4, 'activate', '')
QtTesting.playCommand(object9, 'set_string', 'File1.png')
QtTesting.playCommand(object4, 'activate', '')
QtTesting.playCommand(object3, 'set_string', 'Images (*.png;*.bmp)')
QtTesting.playCommand(object1, 'activate', '')
QtTesting.playCommand(object6, 'currentChanged', '/0|0')
QtTesting.playCommand(object6, 'currentChanged', '/8|0')
QtTesting.playCommand(object4, 'activate', '')
