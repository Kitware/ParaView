
//--------------------------------------------------------------------------
//
// This file is part of the Vistrails ParaView Plugin.
//
// This file may be used under the terms of the GNU General Public
// License version 2.0 as published by the Free Software Foundation
// and appearing in the file LICENSE.GPL included in the packaging of
// this file.  Please review the following to ensure GNU General Public
// Licensing requirements will be met:
// http://www.opensource.org/licenses/gpl-2.0.php
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
//
// Copyright (C) 2009 VisTrails, Inc. All rights reserved.
//
//--------------------------------------------------------------------------


#include "PluginMain.h"

#include "vtkSmartPointer.h"
#include "vtkUndoSet.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxy.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSMUndoRedoStateLoader.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include <vtksys/SystemTools.hxx>

#include "pqUndoStack.h"
#include "pqApplicationCore.h"
#include "pqServer.h"
#include "pqHelperProxyRegisterUndoElement.h"
#include "pqPendingDisplayUndoElement.h"
#include "pqProxyUnRegisterUndoElement.h"
#include "pqObjectBuilder.h"
#include "pqServerResources.h"

#include <QMessageBox>
#include <QTextStream>
#include <QByteArray>
#include <QHostAddress>
#include <QTcpServer>
#include <QDir>
#include <QStringList>
#include <QHostAddress>

//#include <iostream>
#include <vtksys/ios/sstream>
#include <vtkstd/string>


// we need ntohl() and htonl()
#ifdef WIN32
#include <WinSock.h>
#else
#include <arpa/inet.h>
#endif


// Don't print debug messages if we're in release mode
#ifdef NDEBUG
class NoOpStream {
public:
  NoOpStream() {}
  NoOpStream& operator<<(QString) { return *this; }
  NoOpStream& operator<<(const char*) { return *this; }
  NoOpStream& operator<<(int) { return *this; }
  NoOpStream& operator<<(float) { return *this; }
  NoOpStream& operator<<(double) { return *this; }
};
#define qDebug NoOpStream
#endif


// Commands that VisTrails can send to us.
const int pvSHUTDOWN = 0;
const int pvRESET = 1;
const int pvMODIFYSTACK = 2;

// Commands we can send to VisTrails.
const int vtSHUTDOWN = 0;
const int vtUNDO = 1;
const int vtREDO = 2;
const int vtNEW_VERSION = 3;
const int vtREQUEST_VERSION_DATA = 4;

// Where do we expect to find VisTrails listening for us.
QHostAddress vtHost(QHostAddress::LocalHost);
const int vtPort = 50007;

// The Port that we listen on.
const int pvPort = 50013;




/**
The PluginMain class is the main "public interface"
between the VisTrails client and ParaView.
*/
PluginMain::PluginMain() : QThread() {

  undoStack = pqApplicationCore::instance()->getUndoStack();

  versionStackIndex = 0;
  versionStack.push_back(0);

  ignoreStackSignal = false;
  quitting = false;
  vistrailsShutdown = false;

  activeServer = NULL;
  stateLoading = false;

  vtSender = NULL;

  mainThread = QThread::currentThread();
}


/**
ParaView calls Startup() when the plugin is loaded.  We use it to
spawn a VisTrails process, and start our "server" thread to listen
for a VisTrails connection.
*/
void PluginMain::Startup() {

  // Figure out which directory our vistrails directory is in.
  QString pluginPath(vtksys::SystemTools::GetEnv("PV_PLUGIN_PATH"));
  if (!QDir(pluginPath + "/VisTrailsPlugin/vistrails").exists()) {
    const char* path = vtkProcessModule::GetProcessModule()->GetOptions()->GetApplicationPath();
    vtksys::String appDir = vtksys::SystemTools::GetProgramPath(path);
    pluginPath = QString(appDir.c_str()) + QString("/plugins");

    if (!QDir(pluginPath + "/VisTrailsPlugin/vistrails/").exists()) {
      QMessageBox::critical(NULL, "Provenance Recorder", "couldn't find vistrails directory!");
      return;
    }
  }
#ifdef __APPLE__
        //On a Mac we will assume we will run from inside paraview bundle
        //We will also assume that there will be a python executable inside the bundle
        const char* path = vtkProcessModule::GetProcessModule()->GetOptions()->GetApplicationPath();
        vtksys::String appDir = vtksys::SystemTools::GetProgramPath(path);
  QString pycmd = QString(appDir.c_str()) + QString("/python ") + pluginPath + QString("/VisTrailsPlugin/vistrails/api/VisTrails.py") ;
#else
  QString pycmd = QString("pvpython ") + "\"" + pluginPath + QString("/VisTrailsPlugin/vistrails/api/VisTrails.py\"");
  //QMessageBox::information(NULL, "Menu", pycmd);
#endif

#ifdef NDEBUG
  QString shellcmd("");
#else
#ifdef WIN32
  QString shellcmd("cmd /c start ");
#else
  QString shellcmd("xterm -e ");
#endif
#endif

  qDebug()<<(shellcmd+pycmd);
  visTrails.start(shellcmd + pycmd);


  if (visTrails.waitForStarted()) {
    // Start our listener thread.  When it gets the connections sorted out,
    // it will finish the initialization (connecting to signals, etc..)
    start();
  } else {
    QMessageBox::critical(NULL, "Provenance Recorder", "VisTrails failed to start!");
  }
}


/**
ParaView calls Shutdown() when the plugin is unloaded or the app is closed.
We send a message to the vistrails process to close it down, and stop our 
listener thread.
*/
void PluginMain::Shutdown() {
  // This disconnect seems to crash??
  // we don't want messages anymore
  //  disconnect(this->undoStack, SIGNAL(stackChanged(bool,QString,bool,QString)), 
//    this, SLOT(handleStackChanged(bool,QString,bool,QString)));

  // Send the shutdown signal if vistrails is still running, and close the socket
  if (vtSender) {

    if (!vistrailsShutdown) {
      vtSender->writeInt(vtSHUTDOWN);
      vtSender->waitForBytesWritten(-1);

      int ack;
      if (!vtSender->readInt(ack))
        qCritical() << "socket error";
      if (ack != 0)
        qCritical() << "vistrail error";
    }

    vtSender->close();
  }


  // Wait for the listener to shut down.
  this->wait();

  // Wait for the vistrails process to shut down.
  visTrails.waitForFinished(-1);
}



void PluginMain::aboutToQuit() {
  quitting = true;
}

/**
This is the thread main function for the listener thread.  It first
starts the listener tcp server, then opens listener and sender connections
to VisTrails.  Then it enters the main message processing loop
to handle messages that VisTrails sends.
*/
void PluginMain::run() {

  qDebug() << "Listener starting...";

  // Open the port to listen for a VisTrails connection.
    QTcpServer serv;
  serv.listen(QHostAddress::LocalHost, pvPort);

  // Loop trying to open the sender and receiver sockets.
  SocketHelper *receiver = NULL;
  vtSender = NULL;
  while (receiver==NULL || vtSender==NULL) {

    if (vtSender==NULL) {
      qDebug() << "Opening connection to VisTrails server...";
      QTcpSocket *tmpSock = new QTcpSocket();
      tmpSock->connectToHost(vtHost, vtPort);
      if (tmpSock->waitForConnected(1000)) {
        // All interaction with this socket comes from the main thread.
        tmpSock->moveToThread(mainThread);
        vtSender = new SocketHelper(tmpSock);
        qDebug() << "connection open.";
      } else {
        delete tmpSock;
        qDebug() << "timed out";
      }
    }

    if (receiver==NULL) {
      qDebug() << "Waiting for incoming VisTrails connection...";
      if (serv.waitForNewConnection(1000)) {
        receiver = new SocketHelper(serv.nextPendingConnection());
        qDebug() << "found connection.";
      } else {
        qDebug() << "timed out.";
      }
    }
  }


  // connect together the listener thread to the slots that get called
  // from the main thread.
  qRegisterMetaType< QList<int> >("QList<int>");
  qRegisterMetaType<bool*>("bool*");
  connect(this, SIGNAL(modifyStackSignal(QList<int>,int)),
    this, SLOT(modifyStackSlot(QList<int>,int)),
    Qt::BlockingQueuedConnection);

  connect(this, SIGNAL(resetSignal(int)),
    this, SLOT(resetSlot(int)), 
    Qt::BlockingQueuedConnection);

  connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()),
    this, SLOT(aboutToQuit()));

  // Now link into ParaView's undo stack.
  connect(undoStack, SIGNAL(stackChanged(bool,QString,bool,QString)), 
        this, SLOT(handleStackChanged(bool,QString,bool,QString)));

  // track when state loading happens
  connect(pqApplicationCore::instance(), SIGNAL(stateLoaded()),
    this, SLOT(stateLoaded()));

  connect(&pqApplicationCore::instance()->serverResources(), SIGNAL(changed()),
    this, SLOT(serverResourcesChanged()));

  // track when servers are changed
  connect(pqApplicationCore::instance()->getServerManagerModel(), SIGNAL(serverAdded(pqServer*)),
    this, SLOT(serverAdded(pqServer*)));

  connect(pqApplicationCore::instance()->getServerManagerModel(), SIGNAL(aboutToRemoveServer(pqServer*)),
    this, SLOT(serverRemoved(pqServer*)));


  // We need to trigger a whole server state save.  For some reason, this
  // triggers something that sets some id's somewhere?  It's not clear
  // to me why this actually works...
  // This is ripped from pqMainWindowCore::onFileSaveServerState()
  // except it doesn't actually write the files out!
  vtkPVXMLElement *root = vtkPVXMLElement::New();
  root->SetName("ParaView");
  pqApplicationCore::instance()->saveState(root);
  //this->Implementation->MultiViewManager.saveState(root);
  //this->multiViewManager().saveState(root);
  root->Delete();

  // Go into the main listener loop.
  qDebug() << "Entering listener loop";
  while (1) {

    int msg;
    if (!receiver->readInt(msg)) {
      qCritical() << "socket error";
      break;
    }

    switch (msg) {
          case pvSHUTDOWN:
          {
            qDebug() << "received SHUTDOWN";
            receiver->close();

            if (!quitting) {
              disconnect(this->undoStack, SIGNAL(stackChanged(bool,QString,bool,QString)), 
                this, SLOT(handleStackChanged(bool,QString,bool,QString)));

              disconnect(pqApplicationCore::instance(), SIGNAL(stateLoaded()),
                this, SLOT(stateLoaded()));

              disconnect(&pqApplicationCore::instance()->serverResources(), SIGNAL(changed()),
                this, SLOT(serverResourcesChanged()));


              disconnect(pqApplicationCore::instance()->getServerManagerModel(), SIGNAL(serverAdded(pqServer*)),
                this, SLOT(serverAdded(pqServer*)));

              disconnect(pqApplicationCore::instance()->getServerManagerModel(), SIGNAL(aboutToRemoveServer(pqServer*)),
                this, SLOT(serverRemoved(pqServer*)));

            }
            vistrailsShutdown = true;

            return;
          }

        case pvRESET:
          {
            qDebug() << "received RESET";

            int maxId;
            if (!receiver->readInt(maxId))
              qCritical() << "socket error";

            emit resetSignal(maxId);

            // send an ack
            if (!receiver->writeInt(0))
              qCritical() << "socket error";
          }
          break;

        case pvMODIFYSTACK:
          {
            qDebug() << "received MODIFYSTACK";

            int numVersions;
            if (!receiver->readInt(numVersions))
              qCritical() << "socket error";

            int commonIndex;
            if (!receiver->readInt(commonIndex))
              qCritical() << "socket error";

            QList<int> versions;

            for (int i=0; i<numVersions; i++) {
              int v;
              if (!receiver->readInt(v))
                qCritical() << "socket error";
              versions.append(v);
            }

            emit modifyStackSignal(versions, commonIndex);

            // send an ack
            if (!receiver->writeInt(0))
              qCritical() << "socket error";
          }
          break;

        default:
          qCritical() << "received unknown message from vistrails:" << msg;
    }
  }
}


/**
Sometimes VisTrails tells us to change to a version that we can't get to
directly.  This function requests the deltas from VisTrails when this
happens.
*/
void PluginMain::fetchVersionDeltas(const QList<int> &versions, int commonIndex, int first,
                  QList<QString> &labels, QList<QString> &deltas) {

    // Send the message id.
  if (!vtSender->writeInt(vtREQUEST_VERSION_DATA))
    qCritical() << "socket error";

  // Send how many version deltas we'll be requesting.
  if (!vtSender->writeInt(versions.size()-first-1))
    qCritical() << "socket error";

  // If we're already past the change in directions, treat it as if the
  // change is where we currently are, since we never need the delta
  // to get to the current version.
  if (first > commonIndex)
    commonIndex = first;

  // We need the deltas for all versions from first on, except where we 
  // change directions from going up the version tree to going down.
  for (int i=first; i<versions.size(); i++) {
    if (i != commonIndex) {
      if (!vtSender->writeInt(versions[i]))
        qCritical() << "socket error";
    }
  }
  vtSender->waitForBytesWritten(-1);

  // Now read the results back in, make sure we start empty.
  labels = QList<QString>();
  deltas = QList<QString>();

  for (int i=0; i<versions.size(); i++) {

    // We didn't request deltas for the versions before first,
    // or for the version where we changed directions on the tree.
    if (i<first || i==commonIndex) {
      labels.push_back(QString());
      deltas.push_back(QString());
    }

    // Read the undo label and delta.
    else {
      QString label;
      if (!vtSender->readString(label))
        qCritical() << "socket error";
      labels.push_back(label);

      QString delta;
      if (!vtSender->readString(delta))
        qCritical() << "socket error";
      deltas.push_back(delta);
    }
  }
}


/**
This method handles the MODIFY_STACK message from vistrails.  It gives
a list of versions that we should traverse through to get to the version
the user wants.  The versions first go up the version tree, which we can
get to by doing undos.  They then down the version tree, which we can
sometimes get to with redos, but usually require fetching the delta.
*/
void PluginMain::modifyStackSlot(QList<int> versions, int commonIndex) {

  // Prevents the "stackChanged" signal from being handled
  // while we are fiddling with ParaViews undo stack.
    ignoreStackSignal = true;

  // Sanity check - make sure we're at the version the VisTrails
  // thinks we're at.
  if (versions[0] != versionStack[versionStackIndex]) {
    qCritical() << "received version doesn't match internal version!:" <<
    versions[0] << versionStack[versionStackIndex];
  }

  // If we can't perform the changes with ParaView's undo stack directly,
  // we'll need to request the delta's from VisTrails, which get put
  // in these lists.
  QList<QString> labels;
  QList<QString> deltas;
  bool fetchedDeltas = false;

  // Go up the version tree with undos.
  for (int i=0; i<commonIndex; i++) {

    // Just undo if we can - it will take us to the correct version.
    // FIXME - we used to try to be smart and use the built in undo if we
    // could.  This seems to get out of sync sometimes for some reason,
    // so for now we'll just always grab the xml from vistrails and apply
    // it manually.
    if (0&&undoStack->canUndo()) {
      undoStack->undo();
    }

    // Can't undo - probably due to limited size of the undo stack.
    else {
      undoStack->clear(); // having undo's available causes problems

      // Request the deltas from VisTrails if we haven't already.
      if (!fetchedDeltas) {
        fetchVersionDeltas(versions, commonIndex, i, labels, deltas);
        fetchedDeltas = true;
      }

      // We can't undo state loads - make sure it's an xml string
      char type = deltas[i][0].toAscii();
      QString thisxml = deltas[i].remove(0,1);

      if (type != 'x') {
        qCritical() << "undoing a non-xml delta??";
      }

      // Undo the xml to get to the next version.
      vtkPVXMLParser* xmlParser = vtkPVXMLParser::New();
      xmlParser->Parse(thisxml.toAscii().data());
      vtkPVXMLElement* root = xmlParser->GetRootElement();
      vtkUndoSet* uset = undoStack->getUndoSetFromXML(root);

      // We want to be able to undo/redo this later!
      // FIXME - it would be nice if this Push() didn't invalidate
      // ParaView's redo stack so we could go back down if we wanted to.
      undoStack->Push(labels[i].toAscii().data(), uset);
      undoStack->undo();

      // Let the objects go
      uset->Delete();
      xmlParser->Delete();

    }

    // We've only undone something, so the complete undo/redo stack hasn't
    // changed - we've just moved our position within it.
    versionStackIndex--;

    // Sanity check - make sure the undo version stack matches what we expect.
    if (versions[i+1] != versionStack[versionStackIndex]) {
      qCritical() << "received version doesn't match internal version!:" <<
      versions[i+1] << versionStack[versionStackIndex];
    }
  }


  // Go back down the version tree to the version we want.
  for (int i=commonIndex+1; i<versions.size(); i++) {

    // Just redo if we can and it will take us to the correct version.
    // FIXME - we used to try to be smart and use the built in undo if we
    // could.  This seems to get out of sync sometimes for some reason,
    // so for now we'll just always grab the xml from vistrails and apply
    // it manually.
    if (0&&undoStack->canRedo() && 
      versionStack.size() > versionStackIndex+1 &&
      versionStack[versionStackIndex+1] == versions[i]) {

      undoStack->redo();
    }

    // Can't redo - probably due to going down a different branch of 
    // the version tree, but also possibly due to limited size of the undo stack.
    else {
      undoStack->clear(); // having undo's available causes problems

      // Request the xml deltas from VisTrails if we haven't already.
      if (!fetchedDeltas) {
        fetchVersionDeltas(versions, commonIndex, i-1, labels, deltas);
        fetchedDeltas = true;
      }

      // We have to treat xml deltas and state loads differently
      char type = deltas[i][0].toAscii();
      QString thisdelta = deltas[i].remove(0,1);

      if (type == 'x') {

        // Redo the xml to get to the next version.
        vtkPVXMLParser* xmlParser = vtkPVXMLParser::New();
        xmlParser->Parse(thisdelta.toAscii().data());
        vtkPVXMLElement* root = xmlParser->GetRootElement();
        vtkUndoSet* uset = undoStack->getUndoSetFromXML(root);

        // we want to be able to undo/redo this later!
        undoStack->beginUndoSet(labels[i].toAscii().data());
        uset->Redo();
        undoStack->endUndoSet();

        // Let the objects go
        uset->Delete();
        xmlParser->Delete();
      }

      else if (type == 's') {

        // load the state file in thisdelta
        // this is taken from void pqMainWindowCore::onFileLoadServerState(const QStringList& files)

        // Read in the xml file to restore.
        vtkPVXMLParser *xmlParser = vtkPVXMLParser::New();
        xmlParser->SetFileName(thisdelta.toAscii().data());
        xmlParser->Parse();

        // Get the root element from the parser.
        vtkPVXMLElement *root = xmlParser->GetRootElement();
        if (root) {
          pqApplicationCore::instance()->loadState(root, activeServer);
                                              
          // Add this to the list of recent server resources ...
          pqServerResource resource;
          resource.setScheme("session");
          resource.setPath(thisdelta);
          resource.setSessionServer(activeServer->getResource());
          pqApplicationCore::instance()->serverResources().add(resource);
          pqApplicationCore::instance()->serverResources().save(*pqApplicationCore::instance()->settings());
        }

        else {
          qCritical("Root does not exist. Either state file could not be opened "
            "or it does not contain valid xml");
        }

        xmlParser->Delete();
      }

      else {
        qCritical() << "unknown delta type:" << type;
      }
    }


    // If this was supposed to be a redo, don't truncate the version undo
    // stack - just change position within it.
    if (versionStack.size() > versionStackIndex+1 &&
      versionStack[versionStackIndex+1] == versions[i]) {
      versionStackIndex++;
    }

    // Otherwise, we're going down a new version tree branch - truncate
    // the version stack so we don't think we can redo anything.
    else {
      versionStackIndex++;
      versionStack.erase(versionStack.begin()+versionStackIndex, versionStack.end());
      versionStack.push_back(versions[i]);
    }

    // Sanity check - make sure the undo version stack matches what we expect.
    if (versions[i] != versionStack[versionStackIndex]) {
      qCritical() << "received version doesn't match internal version!:" <<
      versions[i] << versionStack[versionStackIndex];
    }
  }


  // Start handling stack changed signals again.
    this->ignoreStackSignal = false;


  // These next few lines are boilerplate code that we borrowed from ParaView.
  // This tells ParaView to redraw everything I assume..
    vtkSMProxyManager::GetProxyManager()->UpdateRegisteredProxies("sources", 1);
    vtkSMProxyManager::GetProxyManager()->UpdateRegisteredProxies("lookup_tables", 1);
    vtkSMProxyManager::GetProxyManager()->UpdateRegisteredProxies("representations", 1);
    vtkSMProxyManager::GetProxyManager()->UpdateRegisteredProxies("scalar_bars", 1);
    vtkSMProxyManager::GetProxyManager()->UpdateRegisteredProxies(1);
    pqApplicationCore::instance()->render();
}


void PluginMain::resetSlot(int maxId) {

  // This prevents paraview from creating objects with id's that need to be
  // used if we go to a different part of the vistrail.
  vtkProcessModule::GetProcessModule()->ReserveID(vtkClientServerID(maxId));

  ignoreStackSignal = true;
  pqApplicationCore::instance()->getObjectBuilder()->destroyPipelineProxies();
  ignoreStackSignal = false;

  versionStackIndex = 0;
  versionStack.clear();
  versionStack.push_back(0);

  undoStack->clear();
}

/**
This is the slot for handling the signals when ParaView updates its 
version stack.  We determine if the change is from an undo, redo, or
simply a new operation being performed, and let VisTrails know what
happened.
*/
void PluginMain::handleStackChanged(bool canUndo, QString undoLabel, 
                  bool canRedo, QString redoLabel) {

  if (ignoreStackSignal) {
    return;
  }

  if (pqApplicationCore::instance()->isLoadingState()) {
    return;
  }

  // we should reset of we cant undo or redo anything
  if (!canUndo && !canRedo) {
    versionStackIndex = 0;
    versionStack.clear();
    versionStack.push_back(0);
    return;
  }

    // we need to handle undo's, redo's, and normal stack changes differently
  if (undoStack->getInUndo()) {

    vtSender->writeInt(vtUNDO);
    vtSender->waitForBytesWritten(-1);

    int ack;
    if (!vtSender->readInt(ack))
      qCritical() << "socket error";
    if (ack != 0)
      qCritical() << "vistrail error";

    versionStackIndex--;

  } else if (undoStack->getInRedo()) {

    vtSender->writeInt(vtREDO);
    vtSender->waitForBytesWritten(-1);

    int ack;
    if (!vtSender->readInt(ack))
      qCritical() << "socket error";
    if (ack != 0)
      qCritical() << "vistrail error";

    versionStackIndex++;

  } else {

    if (!canUndo) {
      qCritical() << "can't undo - not sending version!";
      return;
    }

    // Get the xml delta for the operation at the top of the undo stack.
    vtksys_ios::stringstream xmlStream;
    vtkstd::string xmlString;

    vtkUndoSet *uset = undoStack->getLastUndoSet();
    vtkPVXMLElement* xml = uset->SaveState(NULL);

    xml->PrintXML(xmlStream, vtkIndent());
    QString xmlStr(xmlStream.str().c_str());

    xml->Delete();
    uset->Delete();

    // This is an xml delta - make the first character start with an 'x'
    xmlStr = "x"+xmlStr;

    //Send VisTrails the xml representation of this version
    vtSender->writeInt(vtNEW_VERSION);
    vtSender->writeString(undoLabel);
    vtSender->writeString(xmlStr);
    vtSender->writeInt(vtkProcessModule::GetProcessModule()->GetUniqueID().ID+1);
    vtSender->waitForBytesWritten(-1);

    // VisTrails sends back the id of the new version.
    int version;
    if (!vtSender->readInt(version))
      qCritical() << "socket error";
    if (version < 0)
      qCritical() << "vistrail error";

    // Update the stack of version id's - truncate the version stack
    // since we can't redo anything now.
    versionStack.erase(versionStack.begin()+versionStackIndex+1, versionStack.end());
    versionStack.push_back(version);
    versionStackIndex++;
  }
}

/**
We need to know when a state file is loaded.  To do that, we need to keep track
of the current server.
*/
void PluginMain::serverAdded(pqServer *server) {
  activeServer = server;
}

void PluginMain::serverRemoved(pqServer *server) {
  activeServer = NULL;
}

void PluginMain::serverResourcesChanged() {
  if (stateLoading) {
    qDebug() << "state file:" << pqApplicationCore::instance()->serverResources().list()[0].path();
    stateLoading = false;


    // This is a state file delta - make the first character start with an 's'
    QString filename = "s"+pqApplicationCore::instance()->serverResources().list()[0].path();

    //Send VisTrails the filename representation of this version
    vtSender->writeInt(vtNEW_VERSION);
    vtSender->writeString("State Load");
    vtSender->writeString(filename);
    vtSender->writeInt(vtkProcessModule::GetProcessModule()->GetUniqueID().ID+1);
    vtSender->waitForBytesWritten(-1);

    // VisTrails sends back the id of the new version.
    int version;
    if (!vtSender->readInt(version))
      qCritical() << "socket error";
    if (version < 0)
      qCritical() << "vistrail error";

    // Update the stack of version id's - truncate the version stack
    // since we can't redo anything now.
    versionStack.erase(versionStack.begin()+versionStackIndex+1, versionStack.end());
    versionStack.push_back(version);
    versionStackIndex++;
  }
}

void PluginMain::stateLoaded() {

  if (ignoreStackSignal) {
    return;
  }

  qDebug() << "state loaded";
  stateLoading = true;
}



SocketHelper::SocketHelper(QTcpSocket *s) {
  sock = s;
}

bool SocketHelper::readData(int size, QByteArray &data) {

    // keep reading data until we have the length that we're looking for
  while (buffer.size() < size) {
    if (sock->waitForReadyRead(-1)) {
      buffer.append(sock->readAll());
    } else {
      qCritical() << "error reading from socket!";
      return false;
    }
  }

    data = buffer.left(size);
  buffer.remove(0, size);
  return true;
}

bool SocketHelper::readInt(int &i) {
  QByteArray raw;
  if (!readData(4, raw))
    return false;

  int neti = *(int*)raw.data();
  int hosti = ntohl(neti);

  i = hosti;
  return true;
}

bool SocketHelper::readString(QString &s) {
  int size;
  if (!readInt(size))
    return false;

  QByteArray raw;
  if (!readData(size, raw))
    return false;

  s = raw.data();
  return true;
}


bool SocketHelper::writeInt(int hosti) {
  int neti = htonl(hosti);
  if (sock->write((const char*)&neti, 4) != 4)
    return false;

  return true;
}

bool SocketHelper::writeString(QString s) {
  if (!writeInt(s.length()))
    return false;

  if (sock->write(s.toAscii().data(), s.length()) != s.length())
    return false;

  return true;
}


bool SocketHelper::waitForBytesWritten(int timeout) {
  return sock->waitForBytesWritten(timeout);
}

void SocketHelper::close() {
  sock->close();
}

void SocketHelper::moveToThread(QThread *thread) {
  sock->moveToThread(thread);
}
