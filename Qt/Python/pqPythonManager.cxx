/*=========================================================================

   Program: ParaView
   Module:    pqPythonManager.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "pqPythonManager.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqPythonDialog.h"
#include "pqPythonMacroSupervisor.h"
#include "pqPythonToolsWidget.h"

// These includes are so that we can listen for server creation/removal
// and reset the python interpreter when it happens.
#include "pqServer.h"
#include "pqObjectBuilder.h"
#include "pqServerManagerModel.h"

#include <QApplication>
#include <QCursor>
#include <QDebug>
#include <QLayout>
#include <QSplitter>

//-----------------------------------------------------------------------------
class pqPythonManager::pqInternal
{
public:
  QPointer<pqPythonDialog>            PythonDialog;
  QPointer<pqPythonToolsWidget>       ToolsWidget;
  QPointer<pqPythonMacroSupervisor>   MacroSupervisor;
  QPointer<pqServer>                  ActiveServer;
};

//-----------------------------------------------------------------------------
pqPythonManager::pqPythonManager(QObject* parent/*=null*/) :
  QObject(parent)
{
  this->Internal = new pqInternal;
  pqApplicationCore* core = pqApplicationCore::instance();
  core->registerManager("PYTHON_MANAGER", this);

  // Create an instance of the macro supervisor
  this->Internal->MacroSupervisor = new pqPythonMacroSupervisor(this);
  this->connect(this->Internal->MacroSupervisor,
    SIGNAL(executeScriptRequested(const QString&)),
    SLOT(executeScript(const QString&)));

  // Listen for signal when server is about to be removed
  this->connect(core->getServerManagerModel(),
      SIGNAL(aboutToRemoveServer(pqServer*)),
      this, SLOT(onRemovingServer(pqServer*)));

  // Listen for signal when server is finished being created
  this->connect(core->getObjectBuilder(), 
    SIGNAL(finishedAddingServer(pqServer*)),
    this, SLOT(onServerCreationFinished(pqServer*)));
}

//-----------------------------------------------------------------------------
pqPythonManager::~pqPythonManager()
{
  pqApplicationCore::instance()->unRegisterManager("PYTHON_MANAGER");
  // Make sure the python dialog is cleaned up in case it was never
  // given a parent.
  if (this->Internal->PythonDialog && !this->Internal->PythonDialog->parent())
    {
    delete this->Internal->PythonDialog;
    }
  delete this->Internal;
}

//-----------------------------------------------------------------------------
bool pqPythonManager::interpreterIsInitialized()
{
  return (this->Internal->PythonDialog != NULL);
}

//-----------------------------------------------------------------------------
pqPythonDialog* pqPythonManager::pythonShellDialog()
{
  // Create the dialog and initialize the interpreter the first time this
  // method is called.
  if (!this->Internal->PythonDialog)
    {
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    this->Internal->PythonDialog =
      new pqPythonDialog(pqCoreUtilities::mainWidget());

    // Initialize the interpreter and then import paraview modules
    this->Internal->PythonDialog->initializeInterpretor();
    this->initializeParaviewPythonModules();

    // Listen for the signal when the interpreter is reset so we can
    // reimport the paraview modules.  Note, this signal is connected
    // AFTER the two initialization calls above.
    this->connect(this->Internal->PythonDialog,
      SIGNAL(interpreterInitialized()),
      SLOT(onPythonInterpreterInitialized()));

    // Create the python tools widget and embed it in the python dialog.
    // Due to some buggy layout issues with QSplitter, wrap the tools widget
    // in a qwidget before placing it in the splitter.
    QSplitter* splitter = this->Internal->PythonDialog->splitter();
    QWidget* w = new QWidget(splitter);
    QVBoxLayout* layout = new QVBoxLayout(w);
    layout->setSpacing(0);
    layout->setMargin(0);
    this->Internal->ToolsWidget = new pqPythonToolsWidget(w);
    w->layout()->addWidget(this->Internal->ToolsWidget);
    splitter->addWidget(w);
    splitter->setStretchFactor(0, 3); // (widget_index, stetch_factor)
    splitter->setStretchFactor(1, 2);
    this->Internal->PythonDialog->restoreSplitterState();

    // Connect signals from ToolsWidget to the pqPythonMacroSupervisor
    QObject::connect(this->Internal->ToolsWidget,
      SIGNAL(addMacroRequested(const QString&, const QString&)),
      this->Internal->MacroSupervisor,
      SLOT(addMacro(const QString&, const QString&)));
    QObject::connect(this->Internal->ToolsWidget,
      SIGNAL(removeMacroRequested(const QString&)),
      this->Internal->MacroSupervisor,
      SLOT(removeMacro(const QString&)));

    QApplication::restoreOverrideCursor();
    }
  return this->Internal->PythonDialog;
}

//-----------------------------------------------------------------------------
void pqPythonManager::initializeParaviewPythonModules()
{
  // If there is an active server, import the paraview modules and initialize
  // the servermanager.ActiveConnection object.
  pqServer* activeServer = this->Internal->ActiveServer;
  if (activeServer)
    {
    int cid = static_cast<int>(activeServer->GetConnectionID());
    pqServerResource serverRes = activeServer->getResource();
    int reversed = (serverRes.scheme() == "csrc" ||
      serverRes.scheme() == "cdsrsrc") ? 1 : 0;
    QString dsHost(""), rsHost("");
    int dsPort = 0, rsPort = 0;
    QString strURI = serverRes.toURI();
    if(strURI != "builtin:")
      {
      dsHost = serverRes.dataServerHost().isEmpty() ?
        serverRes.host() : serverRes.dataServerHost();
      dsPort = serverRes.dataServerPort() < 0 ? 
        serverRes.port() : serverRes.dataServerPort();
      rsHost = serverRes.renderServerHost();
      rsPort = serverRes.renderServerPort() < 0 ? 
        rsPort : serverRes.renderServerPort();
      }
      
    QString initStr = QString(
      "import paraview\n"
      "paraview.compatibility.major = 3\n"
      "paraview.compatibility.minor = 5\n"
      "from paraview import servermanager\n"
      "servermanager.ActiveConnection = servermanager.Connection(%1)\n"
      "servermanager.ActiveConnection.SetHost(\"%2\", %3, \"%4\", %5, %6)\n"
      "servermanager.ToggleProgressPrinting()\n"
      "servermanager.fromGUI = True\n"
      "from paraview.simple import *\n"
      "active_objects.view = servermanager.GetRenderView()")
      .arg(cid)
      .arg(dsHost)
      .arg(dsPort)
      .arg(rsHost)
      .arg(rsPort)
      .arg(reversed);
    this->Internal->PythonDialog->print(
      "from paraview.simple import *");
    this->Internal->PythonDialog->runString(initStr);
    emit this->paraviewPythonModulesImported();
    }
}

//-----------------------------------------------------------------------------
void pqPythonManager::addWidgetForMacros(QWidget* widget)
{
  this->Internal->MacroSupervisor->addWidgetForMacros(widget);
}

//-----------------------------------------------------------------------------
void pqPythonManager::onPythonInterpreterInitialized()
{
  this->initializeParaviewPythonModules();
}

//-----------------------------------------------------------------------------
void pqPythonManager::executeScript(const QString & filename)
{
  pqPythonDialog* dialog = this->pythonShellDialog();
  dialog->runScript(QStringList(filename));
}

//-----------------------------------------------------------------------------
void pqPythonManager::onRemovingServer(pqServer* /*server*/)
{
  // Clear our stored pointer to the active server.
  this->Internal->ActiveServer = 0;

  // If the interpreter has been initialized then we need to reset it because
  // the active server is about to be destroyed.  Later, when a new server
  // is created we will catch the signal in onServerCreationFinished and then
  // re-import the paraview modules.
  if (this->interpreterIsInitialized())
    {
    this->pythonShellDialog()->initializeInterpretor();
    }
}

//-----------------------------------------------------------------------------
void pqPythonManager::onServerCreationFinished(pqServer* server)
{
  this->Internal->ActiveServer = server;
  // Initialize interpretor using the new server connection.
  if (this->interpreterIsInitialized())
    {
    this->initializeParaviewPythonModules();
    }
}
