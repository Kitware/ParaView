
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
""" File for the window used when opening VisTrails objects from the database

QOpenDBWindow
QDBConnectionList
QDBConnectionListItem
QDBObjectList
QDBObjectListItem
QConnectionDBSetupWindow

"""
from PyQt4 import QtCore, QtGui
from db import VistrailsDBException
import db.services.io
from core.external_connection import ExtConnectionList, DBConnection
from core.system import default_connections_file
from gui.theme import CurrentTheme
from gui.utils import show_warning, show_question, NO_BUTTON, YES_BUTTON
import gui.application

class QOpenDBWindow(QtGui.QDialog):
    """
    QOpenDBWindow is a dialog containing two panels. the left panel shows all
    the stored database connections and the right paanel shows the vistrails
    available on the selected database connection.

    """
    _instance = None
    def __init__(self, parent=None):
        """ __init__(parent: QWidget) -> QOpenDBWindow
        Construct the dialog with the two panels

        """
        QtGui.QDialog.__init__(self,parent)
        self.setWindowTitle("Choose a vistrail")
        self.save = False
        mainLayout = QtGui.QVBoxLayout()
        panelsLayout = QtGui.QGridLayout()

        self.createActions()
        self.saveasLayout = QtGui.QHBoxLayout()
        self.saveasLabel = QtGui.QLabel("Save As:")
        self.saveasEdt = QtGui.QLineEdit("")
        self.saveasEdt.setFixedWidth(200)
        self.saveasEdt.setSizePolicy(QtGui.QSizePolicy.Fixed,
                                     QtGui.QSizePolicy.Fixed)
        self.saveasLayout.addWidget(self.saveasLabel)
        self.saveasLayout.addWidget(self.saveasEdt)
        self.saveasLabel.setVisible(False)
        self.saveasEdt.setVisible(False)
        self.connectionList = QDBConnectionList(self)
        self.objectList = QDBObjectList(self)

        dbLabel = QtGui.QLabel("Databases:")
        self.vtLabel = QtGui.QLabel("Vistrails: ")
        
        panelsLayout.addWidget(dbLabel,0,0,1,1)
        panelsLayout.setColumnMinimumWidth(1,10)
        panelsLayout.addWidget(self.vtLabel,0,2,1,2)
        panelsLayout.addWidget(self.connectionList,1,0,1,1)
        panelsLayout.addWidget(self.objectList,1,2,1,2)

        self.addButton = QtGui.QToolButton()
        self.addButton.setDefaultAction(self.addAct)
        self.addButton.setAutoRaise(True)

        self.removeButton = QtGui.QToolButton()
        self.removeButton.setDefaultAction(self.removeAct)
        self.removeButton.setAutoRaise(True)
        self.removeButton.setEnabled(False)
        
        panelButtonsLayout = QtGui.QHBoxLayout()
        panelButtonsLayout.setMargin(0)
        panelButtonsLayout.setSpacing(0)
        panelButtonsLayout.addWidget(self.addButton)
        panelButtonsLayout.addWidget(self.removeButton)
        panelsLayout.addLayout(panelButtonsLayout,2,0,1,1,
                               QtCore.Qt.AlignLeft)
        buttonsLayout = QtGui.QHBoxLayout()
        self.cancelButton = QtGui.QPushButton('Cancel')
        self.cancelButton.setAutoDefault(False)
        self.openButton = QtGui.QPushButton('Open')
        self.openButton.setEnabled(False)
        self.openButton.setAutoDefault(True)
        
        buttonsLayout.addStretch(1)
        buttonsLayout.addWidget(self.cancelButton)
        buttonsLayout.addWidget(self.openButton)

        mainLayout.addLayout(self.saveasLayout)
        mainLayout.addLayout(panelsLayout)
        mainLayout.addLayout(buttonsLayout)
        mainLayout.setAlignment(self.saveasLayout, QtCore.Qt.AlignHCenter)
        self.setLayout(mainLayout)
        self.connectSignals()
        QOpenDBWindow._instance = self

    def createActions(self):
        """ createActions() -> None
        Create actions related to context menu 

        """
        self.addAct = QtGui.QAction("+", self)
        self.addAct.setStatusTip("Create a new connection")
        self.removeAct = QtGui.QAction("-", self)
        self.removeAct.setStatusTip("Remove the selected connection from list")
        
    def showEvent(self, e):
        """showEvent(e: QShowEvent) -> None
        If the use doesn't have any connection set up, we will ask him
        to create one.
        
        """
        if self.connectionList.count() == 0:
            text = "You don't seem to have any connection available. \
Would you like to create one?"
            res = show_question('Vistrails',
                                text, 
                                [NO_BUTTON, YES_BUTTON],
                                NO_BUTTON)
            if res == YES_BUTTON:
                self.showConnConfig()
        else:
            self.updateDBObjectsList()

    def connectSignals(self):
        """ connectSignals() -> None
        Map signals between GUI components        
        
        """
        self.connect(self.cancelButton,
                     QtCore.SIGNAL('clicked()'),
                     self.reject)
        self.connect(self.openButton,
                     QtCore.SIGNAL('clicked()'),
                     self.accept)
        self.connect(self.addAct,
                     QtCore.SIGNAL('triggered()'),
                     self.showConnConfig)
        self.connect(self.removeAct,
                     QtCore.SIGNAL('triggered()'),
                     self.connectionList.removeConnection)
        self.connect(self.connectionList,
                     QtCore.SIGNAL('itemSelectionChanged()'),
                     self.updateDBObjectsList)
        self.connect(self.connectionList,
                     QtCore.SIGNAL('itemSelectionChanged()'),
                     self.updateButtons)
        self.connect(self.connectionList,
                     QtCore.SIGNAL('itemClicked(QListWidgetItem *)'),
                     self.updateDBObjectsList)
        self.connect(self.connectionList,
                     QtCore.SIGNAL("reloadConnections"),
                     self.updateDBObjectsList)
        self.connect(self.objectList,
                     QtCore.SIGNAL('itemSelectionChanged()'),
                     self.updateButtons)
        self.connect(self.saveasEdt,
                     QtCore.SIGNAL('textChanged(QString)'),
                     self.updateButtons)
        self.connect(self.objectList,
                     QtCore.SIGNAL('itemDoubleClicked(QListWidgetItem *)'),
                     self.accept)
        
    def updateDBObjectsList(self):
        """ updateDBObjectsList() -> None
        It reloads the vistrails object list for the selected connection. 
        If nothing is selected, it will clear the list.

        """
        conn = self.connectionList.getCurrentItemId()
        self.objectList.updateContents(conn)
        self.updateEditButtons(conn)

    def updateButtons(self):
        """updateButtons() -> None
        It will enable the open button if a vistrail is selected or in case
        of saving a vistrail, if a connection is selected and the name is valid

        """
        vtlist = self.objectList
        if not self.save:
            if len(vtlist.selectedItems()) > 0:
                self.openButton.setEnabled(True)
            else:
                self.openButton.setEnabled(False)
        else:
            if (len(self.connectionList.selectedItems()) > 0 and
                self.saveasEdt.text() != '' and
                len(vtlist.findItems(self.saveasEdt.text(),
                                     QtCore.Qt.MatchFixedString)) == 0):
                self.openButton.setEnabled(True)
            else:
                self.openButton.setEnabled(False)
                
    def updateEditButtons(self, id):
        """updateEditButtons(id: int) -> None
        It will enable/disable the connections buttons according to the
        selection

        """
        if id != -1:
            self.removeButton.setEnabled(True)
        else:
            self.removeButton.setEnabled(False)

    def showConnConfig(self, *args, **keywords):
        """showConnConfig(*args, **keywords) -> None
        shows a window to configure the connection. The valid keywords
        are defined in QConnectionDBSetupWindow.__init__()
        
        """
        keywords["parent"] = self
        
        dialog = QConnectionDBSetupWindow(**keywords)
        if dialog.exec_() == QtGui.QDialog.Accepted:
            config = {'id': int(dialog.id),
                      'name': str(dialog.nameEdt.text()),
                      'host': str(dialog.hostEdt.text()),
                      'port': int(dialog.portEdt.value()),
                      'user': str(dialog.userEdt.text()),
                      'passwd': str(dialog.passwdEdt.text()),
                      'db': str(dialog.databaseEdt.text())}
            id = self.connectionList.setConnectionInfo(**config)
            self.connectionList.setCurrentId(id)
            return True
        else:
            return False

    def prepareForOpening(self, obj_type):
        """prepareForOpening() -> None
        It will prepare the dialog to be a Open Dialog
        
        """
        self.setWindowTitle("Choose a %s" % obj_type.capitalize())
        if obj_type != 'vistrail':
            self.vtLabel.text = 'VisTrails %ss' % obj_type.capitalize()
        else:
            self.vtLabel.text = 'Vistrails'
        self.save = False
        self.objectList.obj_type = obj_type
        self.objectList.setEnabled(True)
        self.saveasLabel.setVisible(False)
        self.saveasEdt.setVisible(False)
        self.openButton.setEnabled(False)
        self.openButton.setText("Open")

    def prepareForSaving(self, obj_type):
        """prepareForSaving() -> None
        It will prepare the dialog to be a save as dialog 
        
        """
        self.setWindowTitle("Save %s..." % obj_type.capitalize())
        if obj_type != 'vistrail':
            self.vtLabel.text = 'VisTrails %ss' % obj_type.capitalize()
        else:
            self.vtLabel.text = 'Vistrails'
        self.save = True
        self.objectList.obj_type = obj_type
        self.objectList.setEnabled(False)
        self.saveasLabel.setVisible(True)
        self.saveasEdt.setVisible(True)
        self.openButton.setText("Save")
        self.openButton.setEnabled(False)
        
    @staticmethod
    def getInstance():
        """getInstance() -> QOpenDBWindow
        Returns the current instance of the QOpenDBWindow

        """
        if QOpenDBWindow._instance is None:
            QOpenDBWindow._instance = QOpenDBWindow()
        return QOpenDBWindow._instance
    
    @staticmethod
    def getOpenDBObject(obj_type):
        """getOpenDBObject(obj_type : str) -> (dict,int)
        Creates a dialog for opening a vistrails object from the database. 
        It will return the selected connection configuration information and
        the object id.
        
        """
        if QOpenDBWindow._instance:
            dlg = QOpenDBWindow._instance
        else:
            dlg = QOpenDBWindow()

        dlg.prepareForOpening(obj_type)
        
        if dlg.exec_() == QtGui.QDialog.Accepted:
            return (dlg.connectionList.getCurrentConnConfig(),
                    dlg.objectList.currentItem().id)
        else:
            return({},-1)

    @staticmethod
    def getSaveDBObject(obj_type):
        """getSaveDBObject(obj_type : str) -> (dict, str)
        Creates a dialog for saving a vistrails object to the database. 
        It will return the selected connection configuration information 
        and the object name

        """
        if QOpenDBWindow._instance:
            dlg = QOpenDBWindow._instance
        else:
            dlg = QOpenDBWindow()

        dlg.prepareForSaving(obj_type)

        if dlg.exec_() == QtGui.QDialog.Accepted:
            return (dlg.connectionList.getCurrentConnConfig(),
                    str(dlg.saveasEdt.text()).strip(' \n\t'))
        else:
            return({},'')
        
################################################################################

class QDBConnectionList(QtGui.QListWidget):
    """
    QDBConnection list is a widget to show the available databases

    """
    def __init__(self, parent=None):
        QtGui.QListWidget.__init__(self,parent)
        self.__list = ExtConnectionList(default_connections_file())
        self.setSelectionMode(QtGui.QAbstractItemView.SingleSelection)
        self.setIconSize(QtCore.QSize(32,32))
        self.loadConnections()
        self.editAct = QtGui.QAction("Edit", self)
        self.editAct.setStatusTip("Edit the selected connection")
        self.connect(self.editAct,
                     QtCore.SIGNAL("triggered()"),
                     self.editConnection)
        
    def getCurrentItemId(self):
        """getCurrentItemId() -> int
        Returns the id of the selected item. If there is no item selected,
        it will return -1.

        """
        item = None
        if len(self.selectedItems()) > 0:
            item  = self.selectedItems()[0]
        if item != None:
            return int(item.id)
        else:
            return -1
        
    def contextMenuEvent(self, e):
        """contextMenuEvent(e: QContextMenuEvent) -> None
        Shows a popup menu for the connection

        """
        item = self.currentItem()
        if item:
            menu = QtGui.QMenu()
            menu.addAction(self.editAct)
            menu.exec_(e.globalPos())

    def editConnection(self):
        """editConnection() -> None
        Method called to edit a connection. It will get the information
        from the selected connection and show the dialog so the user can
        update the fields
        
        """
        conn_id = self.getCurrentItemId()
        config = self.getConnectionInfo(conn_id)
        if config != None:
            config["create"] = False
            self.parent().showConnConfig(**config)
            
    def updateGUI(self):
        """updateGUI() -> None
        Update GUI list to be consistent with the the list of connections

        """
        self.clear()
        for (id, c) in self.__list.items():
            cItem = QDBConnectionListItem(CurrentTheme.DB_ICON,
                                          int(id),
                                          str(c.name))
            self.addItem(cItem)
        self.emit(QtCore.SIGNAL("reloadConnections"))
        
    def loadConnections(self):
        """loadConnections() -> None
        Loads the internal connections and updates the GUI

        """
        self.__list.clear()
        self.__list.load_connections()
        self.updateGUI()

    def getConnectionInfo(self, id):
        """getConnectionInfo(id: int) -> dict
        Returns info of ExtConnection """
        conn = self.__list.get_connection(id)
        key = str(conn.id) + "." + conn.name + "." + conn.host
        passwd = gui.application.VistrailsApplication.keyChain.get_key(key)
        if conn != None:
            config = {'id': conn.id,
                      'name': conn.name,
                      'host': conn.host,
                      'port': conn.port,
                      'user': conn.user,
                      'passwd': passwd,
                      'db': conn.database}
        else:
            config = None
        return config

    def findConnectionInfo(self, host, port, db):
        """findConnection(host:str, port: int, db: str) -> dict
        Returns complete info of a connection with the given parameters

        """
        id = self.__list.find_db_connection(host,port,db)
        if id != -1:
            return self.getConnectionInfo(id)
        else:
            return None
        
    def removeConnection(self):
        """removeConnection() -> None
        Removes the selected connection

        """
        id = self.getCurrentItemId()
        self.takeItem(self.currentRow())
        self.__list.remove_connection(id)
        
    def setConnectionInfo(self, *args, **kwargs):
        """setConnectionInfo(id: int, name: str, host: str, port:int,
                     user:str, passwd:str, db:str) -> None
        If the connection exists it will update it, else it will add it

        """
        if kwargs.has_key("id"):
            id = kwargs["id"]
        if kwargs.has_key("name"):
            name = kwargs["name"]
        if kwargs.has_key("host"):
            host = kwargs["host"]
        if kwargs.has_key("port"):
            port = kwargs["port"]
        if kwargs.has_key("user"):
            user = kwargs["user"]
        if kwargs.has_key("passwd"):
            passwd = kwargs["passwd"]
        if kwargs.has_key("db"):
            db = kwargs["db"]

        conn = DBConnection(id=id,
                            name=name,
                            host=host,
                            port=port,
                            user=user,
                            passwd='',
                            database=db,
                            dbtype='MySQL')
        
        if self.__list.has_connection(id):    
            self.__list.set_connection(id,conn)
        else:
            if conn.id == -1:
                conn.id = self.__list.get_fresh_id()
            self.__list.add_connection(conn)
        self.updateGUI()
        key = str(conn.id) + "." + conn.name + "." + conn.host
        gui.application.VistrailsApplication.keyChain.set_key(key,passwd)
        return conn.id
            
    def setCurrentId(self, id):
        """setCurrentId(id: int) -> None
        set the connection with id 'id' to be the current selected connection

        """
        conn = self.__list.get_connection(id)
        
        for i in self.findItems(conn.name, QtCore.Qt.MatchFixedString):
            if i.id == id:
                self.setCurrentItem(i)
                break
        self.emit(QtCore.SIGNAL("reloadConnections"), id)

    def getCurrentConnConfig(self):
        """getCurrentConnConfig() -> dict
        Return dictionary of parameters of the current connection to pass
        to MySQLdb

        """
        conn_id = self.currentItem().id
        conn = self.__list.get_connection(conn_id)
        config = self.getConnectionInfo(conn_id)
        if conn.dbtype == 'MySQL':
            #removing extra keyword arguments for MySQldb
            del config['name']
        return config
    
    def getDBObjectList(self, conn_id, obj_type):
        """getDBObjectList(conn_id: int, obj_type : str) -> list
        Returns list of vistrails objects

        """
        conn = self.__list.get_connection(conn_id)
        config = self.getConnectionInfo(conn_id)
        if conn.dbtype == 'MySQL':
            #removing extra keyword arguments for MySQldb
            config_name = config['name']
            del config['name']
            config_id = config['id']
            del config['id']
        vt_list = db.services.io.get_db_object_list(config, obj_type)
        if conn.dbtype == 'MySQL':
            config['name'] = config_name
            config['id'] = config_id
        return vt_list
    
################################################################################
    
class QDBConnectionListItem(QtGui.QListWidgetItem):
    
    def __init__(self, icon, id, text, parent=None):
        """__init__(icon: QIcon, id: int, text: QString, parent: QListWidget)
                         -> QDBConnectionListItem
        Creates an item with id
        
        """
        QtGui.QListWidgetItem.__init__(self,icon, text, parent)
        self.id = id

################################################################################

class QDBObjectList(QtGui.QListWidget):
    """
    QDBObjectList is a widget to show the vistrails available in the selected
    database

    """
    def __init__(self, parent=None, obj_type='vistrail'):
        QtGui.QListWidget.__init__(self, parent)
        self.obj_type = obj_type

    def updateContents(self, conn_id=-1):
        """updateContents(connection_id: int) -> None
        Reloads vistrails from the given connection
        
        """
        self.clear()
        if conn_id != -1:
            parent = self.parent()
            try:
                objs = parent.connectionList.getDBObjectList(int(conn_id),
                                                             self.obj_type)
                
                for (id,obj,date) in objs:
                    item = QDBObjectListItem(CurrentTheme.FILE_ICON,
                                             int(id),
                                             str(obj),
                                             str(date))
                    self.addItem(item)
            except VistrailsDBException, e:
                #show connection setup
                config = parent.connectionList.getConnectionInfo(int(conn_id))
                if config != None:
                    config["create"] = False
                    parent.showConnConfig(**config)
                else:
                    raise e
            
################################################################################

class QDBObjectListItem(QtGui.QListWidgetItem):
    
    def __init__(self, icon, id, name, date, parent=None):
        """__init__(icon: QIcon, id: int, name: QString,
                    date: QString, user: QString, parent: QListWidget)
                         -> QDBObjectListItem
        Creates an item with id
        
        """
        QtGui.QListWidgetItem.__init__(self, icon, name, parent)
        self.id = id
        self.date = date
        self.setToolTip("Last Modified on %s" % date)

################################################################################

class QConnectionDBSetupWindow(QtGui.QDialog):
    """
    QConnectionDBSetupWindow is a dialog for creating a DB connection.
    
    """
    def __init__(self, parent=None, id=-1, name ='', host="", port=3306,
                 user="", passwd="", db="", create=True):
        """ __init__(parent: QWidget, id: int, name: str, host:str, port:int,
                     user:str, passwd:str, db:str, create:Boolean)
                                -> QConnectionDBSetupWindow
        Construct the dialog with the information provided
        create tells if the caption of the button is Create or Update

        """
        QtGui.QDialog.__init__(self,parent)
        if create:
            self.setWindowTitle("Create a new connection")
        else:
            self.setWindowTitle("Update a connection")
            
        mainLayout = QtGui.QVBoxLayout()
        infoLayout = QtGui.QGridLayout()
        self.id = id
        nameLabel = QtGui.QLabel("Save as Connection Name:", self)
        self.nameEdt = QtGui.QLineEdit(name, self)
        hostLabel = QtGui.QLabel("Server Hostname:", self)
        self.hostEdt = QtGui.QLineEdit(host, self)
        portLabel = QtGui.QLabel("Port:", self)
        self.portEdt = QtGui.QSpinBox(self)
        self.portEdt.setMaximum(65535)
        self.portEdt.setValue(port)
        userLabel = QtGui.QLabel("Username:", self)
        self.userEdt = QtGui.QLineEdit(user, self)
        passwdLabel = QtGui.QLabel("Password:", self)
        self.passwdEdt = QtGui.QLineEdit(passwd,self)
        self.passwdEdt.setEchoMode(QtGui.QLineEdit.Password)
        self.passwdEdt.setToolTip("For your protection, your "
                                  "password will not be saved.")
        databaseLabel = QtGui.QLabel("Database:", self)
        self.databaseEdt = QtGui.QLineEdit(db,self)
        mainLayout.addLayout(infoLayout)
        infoLayout.addWidget(nameLabel,0,0,1,1)
        infoLayout.addWidget(self.nameEdt,0,1,1,1)
        infoLayout.addWidget(hostLabel,1,0,1,1)
        infoLayout.addWidget(self.hostEdt,1,1,1,1)
        infoLayout.addWidget(portLabel,1,2,1,1)
        infoLayout.addWidget(self.portEdt,1,3,1,1)
        infoLayout.addWidget(userLabel,2,0,1,1)
        infoLayout.addWidget(self.userEdt,2,1,1,3)
        infoLayout.addWidget(passwdLabel,3,0,1,1)
        infoLayout.addWidget(self.passwdEdt,3,1,1,3)
        infoLayout.addWidget(databaseLabel,4,0,1,1)
        infoLayout.addWidget(self.databaseEdt,4,1,1,3)
        
        buttonsLayout = QtGui.QHBoxLayout()
        if create:
            caption = 'Create'
        else:
            caption = 'Update'
        self.createButton = QtGui.QPushButton(caption, self)
        self.createButton.setDefault(True)
        self.cancelButton = QtGui.QPushButton('Cancel', self)
        self.testButton = QtGui.QPushButton('Test', self)
        
        buttonsLayout.addStretch(1)
        buttonsLayout.addWidget(self.cancelButton)
        buttonsLayout.addWidget(self.testButton)
        buttonsLayout.addWidget(self.createButton)

        mainLayout.addLayout(buttonsLayout)

        self.setLayout(mainLayout)
        self.connectSignals()
        self.updateButtons()
        
    def connectSignals(self):
        """ connectSignals() -> None
        Map signals between GUI components        
        
        """
        self.connect(self.cancelButton,
                     QtCore.SIGNAL('clicked()'),
                     self.reject)
        self.connect(self.createButton,
                     QtCore.SIGNAL('clicked()'),
                     self.accept)
        self.connect(self.testButton,
                     QtCore.SIGNAL('clicked()'),
                     self.testConnection)
        self.connect(self.nameEdt,
                     QtCore.SIGNAL('textChanged(QString)'),
                     self.updateButtons)
        self.connect(self.hostEdt,
                     QtCore.SIGNAL('textChanged(QString)'),
                     self.updateButtons)
        self.connect(self.userEdt,
                     QtCore.SIGNAL('textChanged(QString)'),
                     self.updateButtons)
        self.connect(self.passwdEdt,
                     QtCore.SIGNAL('textChanged(QString)'),
                     self.updateButtons)
        self.connect(self.databaseEdt,
                     QtCore.SIGNAL('textChanged(QString)'),
                     self.updateButtons)
        self.connect(self.portEdt,
                     QtCore.SIGNAL('valueChanged(int)'),
                     self.updateButtons)

    def testConnection(self):
        """testConnection() -> None """
        config = {'host': str(self.hostEdt.text()),
                  'port': int(self.portEdt.value()),
                  'user': str(self.userEdt.text()),
                  'passwd': str(self.passwdEdt.text()),
                  'db': str(self.databaseEdt.text())}
        try:
            db.services.io.test_db_connection(config)
            gui.utils.show_info('Vistrails',"Connection succeeded!")
            
        except Exception, e:
            QtGui.QMessageBox.critical(None,
                                       'Vistrails',
                                       str(e))
    def updateButtons(self):
        """updateButtons() -> None
        enables button if there's enough information in the dialog

        """
        if (self.nameEdt.text() != "" and
            self.hostEdt.text() != "" and
            self.portEdt.value() != 0 and
            self.userEdt.text() != "" and
            self.databaseEdt.text() != ""):
            self.createButton.setEnabled(True)
        else:
            self.createButton.setEnabled(False)
