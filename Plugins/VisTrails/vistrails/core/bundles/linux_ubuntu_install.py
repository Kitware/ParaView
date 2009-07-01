
############################################################################
##
## This file is part of the Vistrails ParaView Plugin.
##
## This file may be used under the terms of the GNU General Public
## License version 2.0 as published by the Free Software Foundation
## and appearing in the file LICENSE.GPL included in the packaging of
## this file.  Please review the following to ensure GNU General Public
## Licensing requirements will be met:
## http://www.opensource.org/licenses/gpl-2.0.php
##
## This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
## WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
##
############################################################################

############################################################################
##
## Copyright (C) 2006, 2007, 2008 University of Utah. All rights reserved.
##
############################################################################
############################################################################

# Installs a package through APT, showing progress.

if __name__ != '__main__':
    import tests
    raise tests.NotModule('This should not be imported as a module')

import apt
import apt_pkg
import sys, os
import copy
import time

package_name = sys.argv[1]

from apt.progress import InstallProgress
from apt.progress import FetchProgress

cache = apt.Cache(apt.progress.OpTextProgress())

try:
    pkg = cache[package_name]
except KeyError:
    sys.exit(1)

if pkg.isInstalled:
    print "Package '%s' already installed" % package_name
    sys.exit(0)


##############################################################################

from PyQt4 import QtCore, QtGui

class GUIInstallProgress(InstallProgress):
    def __init__(self, pbar, status_label):
        apt.progress.InstallProgress.__init__(self)
        self.pbar = pbar
        self.status_label = status_label
        self.last = 0.0
    def updateInterface(self):
        InstallProgress.updateInterface(self)
        if self.last >= self.percent:
            return
        self.status_label.setText(self.status)
        self.pbar.setValue(int(self.percent))
        self.last = self.percent
        QtGui.qApp.processEvents()
    def pulse(self):
        QtGui.qApp.processEvents()
        return InstallProgress.pulse(self)
    def conffile(self,current,new):
        print "conffile prompt: %s %s" % (current,new)
    def error(self, errorstr):
        print "got dpkg error: '%s'" % errorstr

class GUIFetchProgress(FetchProgress):

    def __init__(self, pbar, status_label):
        apt.progress.FetchProgress.__init__(self)
        self.pbar = pbar
        self.status_label = status_label

    def pulse(self):
        FetchProgress.pulse(self)
        if self.currentCPS > 0:
            s = "%sB/s %s" % (apt_pkg.SizeToStr(int(self.currentCPS)),
                              apt_pkg.TimeToStr(int(self.eta)))
        else:
            s = "[Working..]"
        self.status_label.setText(s)
        self.pbar.setValue(int(self.percent))
        QtGui.qApp.processEvents()
        return True

    def stop(self):
        self.status_label.setText("Finished downloading.")
        QtGui.qApp.processEvents()
    
    def updateStatus(self, uri, descr, shortDescr, status):
        if status != self.dlQueued:
            print "\r%s %s" % (self.dlStatusStr[status], descr)
    
        
class Window(QtGui.QWidget):

    def __init__(self, parent=None):
        QtGui.QMainWindow.__init__(self, parent)
        
        mainlayout = QtGui.QVBoxLayout()
        self.setLayout(mainlayout)
        desktop = QtGui.qApp.desktop()
        print desktop.isVirtualDesktop()
        geometry = desktop.screenGeometry(self)
        h = 200
        w = 300
        self.setGeometry(geometry.left() + (geometry.width() - w)/2,
                         geometry.top() + (geometry.height() - h)/2,
                         w, h)
        self.setWindowTitle('VisTrails APT interface')
        lbl = QtGui.QLabel(self)
        mainlayout.addWidget(lbl)
        lbl.setText("VisTrails wants to use APT to install\
 package '%s'. Do you want to allow this?" % package_name) 
        lbl.resize(self.width(), 150)
        lbl.setAlignment(QtCore.Qt.AlignHCenter)
        lbl.setWordWrap(True)
        layout = QtGui.QHBoxLayout()
        self.allowBtn = QtGui.QPushButton("Yes, allow")
        self.denyBtn = QtGui.QPushButton("No, deny")
        layout.addWidget(self.allowBtn)
        layout.addWidget(self.denyBtn)
        self.layout().addLayout(layout)

        self.connect(self.allowBtn, QtCore.SIGNAL("clicked()"),
                     self.perform_install)
        self.connect(self.denyBtn, QtCore.SIGNAL("clicked()"),
                     QtGui.qApp, QtCore.SLOT("quit()"))

        pbarlayout = QtGui.QVBoxLayout()
        pbar = QtGui.QProgressBar()
        pbar.setMinimum(0)
        pbar.setMaximum(100)
        pbarlayout.addWidget(pbar)
        self.layout().addLayout(pbarlayout)
        pbar.show()
        self.pbar = pbar
        self.pbar.setValue(0)
        self.status_label = QtGui.QLabel(self)
        mainlayout.addWidget(self.status_label)
        self.status_label.setText('Waiting for decision...')
        self.layout().addStretch()

    def perform_install(self):
        pkg.markInstall()
        self.allowBtn.setEnabled(False)
        self.denyBtn.setEnabled(False)
        fprogress = GUIFetchProgress(self.pbar, self.status_label)
        iprogress = GUIInstallProgress(self.pbar, self.status_label)
        try:
            cache.commit(fprogress, iprogress)
        except OSError, e:
            pass
        except Exception, e:
            
            self._timeout = QtCore.QTimer()
            self.connect(self._timeout, QtCore.SIGNAL("timeout()"),
                         QtGui.qApp, QtCore.SLOT("quit()"))
            self._timeout.start(3000)
            self.status_label.setText("Success, exiting in 3 seconds.")

app = QtGui.QApplication(sys.argv)

window = Window()
window.show()
print app.exec_()
sys.exit(0)

