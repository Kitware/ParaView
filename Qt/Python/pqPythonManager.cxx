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
#include <vtkPython.h> // Python first
#include "pqPythonManager.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqPythonDialog.h"
#include "pqPythonMacroSupervisor.h"
#include "pqPythonShell.h"
#include "pqPythonScriptEditor.h"
#include "pqSettings.h"
#include "pqServerStartups.h"

// These includes are so that we can listen for server creation/removal
// and reset the python interpreter when it happens.
#include "pqServer.h"
#include "pqObjectBuilder.h"
#include "pqServerManagerModel.h"

#include <QApplication>
#include <QMainWindow>
#include <QStatusBar>
#include <QCursor>
#include <QDebug>
#include <QLayout>
#include <QSplitter>

#include <QFileDialog>
#include <QFile>
#include <QDir>
#include <QTextStream>

#include <QTimer>

//-----------------------------------------------------------------------------
class pqPythonManager::pqInternal
{
public:
  QTimer                              StatusBarUpdateTimer;
  QPointer<pqPythonDialog>            PythonDialog;
  QPointer<pqPythonMacroSupervisor>   MacroSupervisor;
  QPointer<pqServer>                  ActiveServer;
  bool                                IsPythonTracing;
  pqPythonScriptEditor*               Editor;
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

  // Listen the signal when a macro wants to be edited
  QObject::connect(this->Internal->MacroSupervisor,
    SIGNAL(onEditMacro(const QString&)),
    this,
    SLOT(editMacro(const QString&)));

  // Listen for signal when server is about to be removed
  this->connect(core->getServerManagerModel(),
      SIGNAL(aboutToRemoveServer(pqServer*)),
      this, SLOT(onRemovingServer(pqServer*)));

  // Listen for signal when server is finished being created
  this->connect(core->getObjectBuilder(), 
    SIGNAL(finishedAddingServer(pqServer*)),
    this, SLOT(onServerCreationFinished(pqServer*)));

  // Init Python tracing ivar
  this->Internal->IsPythonTracing = false;
  this->Internal->Editor          = NULL;

  // Start StatusBar message update timer
  connect( &this->Internal->StatusBarUpdateTimer, SIGNAL(timeout()),
           this, SLOT(updateStatusMessage()));
  this->Internal->StatusBarUpdateTimer.start(5000); // 1 second
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
  // Make sure the python editor is cleaned up in case it was never
  // given a parent.
  if(this->Internal->Editor && !this->Internal->Editor->parent())
    {
    delete this->Internal->Editor;
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
void pqPythonManager::addWidgetForRunMacros(QWidget* widget)
{
  this->Internal->MacroSupervisor->addWidgetForRunMacros(widget);
}
//-----------------------------------------------------------------------------
void pqPythonManager::addWidgetForEditMacros(QWidget* widget)
{
  this->Internal->MacroSupervisor->addWidgetForEditMacros(widget);
}
//-----------------------------------------------------------------------------
void pqPythonManager::addWidgetForDeleteMacros(QWidget* widget)
{
  this->Internal->MacroSupervisor->addWidgetForDeleteMacros(widget);
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
//-----------------------------------------------------------------------------
bool pqPythonManager::canStartTrace()
{
  return !this->Internal->IsPythonTracing;
}
//-----------------------------------------------------------------------------
bool pqPythonManager::canStopTrace()
{
  return this->Internal->IsPythonTracing;
}
//-----------------------------------------------------------------------------
void pqPythonManager::startTrace()
{
  pqPythonShell* shell = this->pythonShellDialog()->shell();

  if(shell)
    {
    QString script = "from paraview import smtrace\nsmtrace.start_trace()\nprint 'Trace started.'\n";
    shell->executeScript(script);

    // Update internal state
    this->Internal->IsPythonTracing = true;

    // Emit signals
    emit startTraceDone();
    emit canStartTrace(canStartTrace());
    emit canStopTrace(canStopTrace());
    }
}
//-----------------------------------------------------------------------------
void pqPythonManager::stopTrace()
{
  pqPythonShell* shell = this->pythonShellDialog()->shell();

  if(shell)
    {
    QString script = "from paraview import smtrace\nsmtrace.stop_trace()\nprint 'Trace stopped.'\n";
    shell->executeScript(script);

    // Update internal state
    this->Internal->IsPythonTracing = false;

    // Emit signals
    emit stopTraceDone();
    emit canStartTrace(canStartTrace());
    emit canStopTrace(canStopTrace());
    }
}
//----------------------------------------------------------------------------
QString pqPythonManager::getTraceString()
{
  QString traceString;
  pqPythonDialog* pyDiag = this->pythonShellDialog();
  if (pyDiag)
    {
    pyDiag->runString("from paraview import smtrace\n"
                      "__smtraceString = smtrace.get_trace_string()\n");
    pyDiag->shell()->makeCurrent();
    PyObject* main_module = PyImport_AddModule((char*)"__main__");
    PyObject* global_dict = PyModule_GetDict(main_module);
    PyObject* string_object = PyDict_GetItemString(
      global_dict, "__smtraceString");
    char* string_ptr = string_object ? PyString_AsString(string_object) : 0;
    if (string_ptr)
      {
      traceString = string_ptr;
      }
    pyDiag->shell()->releaseControl();
    }
  return traceString;
}
//-----------------------------------------------------------------------------
void pqPythonManager::editTrace()
{
  // Create the editor if needed and only the first time
  if(!this->Internal->Editor)
    {
    this->Internal->Editor = new pqPythonScriptEditor(pqCoreUtilities::mainWidget());
    }

  QString traceString = this->getTraceString();
  this->Internal->Editor->show();
  this->Internal->Editor->raise();
  this->Internal->Editor->activateWindow();
  if (this->Internal->Editor->newFile())
    {
    this->Internal->Editor->setText(traceString);
    }

}
//----------------------------------------------------------------------------
QString pqPythonManager::getPVModuleDirectory()
{
  QString dirString;
  pqPythonDialog* pyDiag = this->pythonShellDialog();
  if (pyDiag)
    {
    pyDiag->runString("import os\n"
                      "__pvModuleDirectory = os.path.dirname(paraview.__file__)\n");
    pyDiag->shell()->makeCurrent();
    PyObject* main_module = PyImport_AddModule((char*)"__main__");
    PyObject* global_dict = PyModule_GetDict(main_module);
    PyObject* string_object = PyDict_GetItemString(
      global_dict, "__pvModuleDirectory");
    char* string_ptr = string_object ? PyString_AsString(string_object) : 0;
    if (string_ptr)
      {
      dirString = string_ptr;
      }
    pyDiag->shell()->releaseControl();
    }
  return dirString;
}

//----------------------------------------------------------------------------
void pqPythonManager::saveTrace()
{
  // Get the script directory
  QString scriptDir;
  pqSettings* settings = pqApplicationCore::instance()->settings();
  if (settings->contains("pqPythonToolsWidget/ScriptDirectory"))
    {
    scriptDir = pqApplicationCore::instance()->settings()->value(
      "pqPythonToolsWidget/ScriptDirectory").toString();
    }
  else
    {
    scriptDir = this->getPVModuleDirectory();
    if (scriptDir.size())
      {
      scriptDir += QDir::separator() + QString("demos");
      }
    }

  QString traceString = this->getTraceString();
  QString fileName = QFileDialog::getSaveFileName(pqCoreUtilities::mainWidget(), tr("Save File"),
                                                  scriptDir,
                                                  tr("Python script (*.py)"));
  if (fileName.isEmpty())
    {
    return;
    }
  if (!fileName.endsWith(".py"))
    {
    fileName.append(".py");
    }

  QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
    qWarning() << "Could not open file:" << fileName;
    return;
    }

  QTextStream out(&file);
  out << traceString;
}
//----------------------------------------------------------------------------
void pqPythonManager::saveTraceState(const QString& fileName)
{
  pqPythonDialog* pyDiag = this->pythonShellDialog();
  if (pyDiag)
    {
    pyDiag->runString("from paraview import smstate\n"
                      "smstate.run()\n");
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
      {
      qWarning() << "Could not open file:" << fileName;
      return;
      }

    QString traceString = this->getTraceString();
    QTextStream out(&file);
    out << traceString;
    }
}
//----------------------------------------------------------------------------
void pqPythonManager::addMacro(const QString& fileName)
{
  QString userMacroDir = pqCoreUtilities::getParaViewUserDirectory() + "/Macros";
  QDir dir;
  dir.setPath(userMacroDir);
  // Copy macro file to user directory
  if(!dir.exists(userMacroDir) && !dir.mkpath(userMacroDir))
    {
    qWarning() << "Could not create user Macro directory:" << userMacroDir;
    return;
    }

  QString expectedFilePath = userMacroDir + "/" + QFileInfo(fileName).fileName();
  expectedFilePath = pqCoreUtilities::getNoneExistingFileName(expectedFilePath);

  QFile::copy(fileName, expectedFilePath);

  // Register the inner one
  this->Internal->MacroSupervisor->addMacro(expectedFilePath);
}
//----------------------------------------------------------------------------
void pqPythonManager::editMacro(const QString& fileName)
{
  // Create the editor if needed and only the first time
  if(!this->Internal->Editor)
    {
    this->Internal->Editor = new pqPythonScriptEditor(pqCoreUtilities::mainWidget());
    }

  this->Internal->Editor->show();
  this->Internal->Editor->raise();
  this->Internal->Editor->activateWindow();
  this->Internal->Editor->open(fileName);
}
//----------------------------------------------------------------------------
void pqPythonManager::updateStatusMessage()
{
  if(this->Internal->IsPythonTracing)
    {
    QMainWindow *mainWindow = qobject_cast<QMainWindow *>(pqCoreUtilities::mainWidget());
    if(mainWindow)
      {
      mainWindow->statusBar()->showMessage("Recording python trace...");
      }
    }
}
