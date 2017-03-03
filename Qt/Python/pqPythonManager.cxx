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
// Include vtkPython.h first to avoid python??_d.lib not found linking error on
// Windows debug builds.
#include <vtkPython.h>

#include "pqPythonManager.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqObjectBuilder.h"
#include "pqOutputWindowAdapter.h"
#include "pqPythonDialog.h"
#include "pqPythonMacroSupervisor.h"
#include "pqPythonScriptEditor.h"
#include "pqPythonShell.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkPVConfig.h"
#include "vtkPythonInterpreter.h"
#include "vtkSMTrace.h"

#include <QApplication>
#include <QCursor>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QLayout>
#include <QMainWindow>
#include <QSplitter>
#include <QStatusBar>
#include <QTextStream>
#include <QTimer>

//-----------------------------------------------------------------------------
class pqPythonManager::pqInternal
{

  void importParaViewModule()
  {
    const char* command = "try:\n"
                          "  import paraview\n"
                          "except: pass\n";
    vtkPythonInterpreter::RunSimpleString(command);
  }

  vtkNew<vtkPythonInterpreter> DummyInterpreter;
  void interpreterEvents(vtkObject*, unsigned long eventid, void* calldata)
  {
    if (eventid == vtkCommand::EnterEvent)
    {
      importParaViewModule();
    }
    else if (eventid == vtkCommand::ErrorEvent)
    {
      const char* message = reinterpret_cast<const char*>(calldata);
      if (this->PythonDialog && this->PythonDialog->shell()->isExecuting())
      {
        this->PythonDialog->shell()->printString(message, pqPythonShell::ERROR);
      }
      else
      {
        qCritical() << message;
      }
    }
    else if (eventid == vtkCommand::SetOutputEvent)
    {
      const char* message = reinterpret_cast<const char*>(calldata);
      if (this->PythonDialog && this->PythonDialog->shell()->isExecuting())
      {
        this->PythonDialog->shell()->printString(message, pqPythonShell::OUTPUT);
      }
      else
      {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
        vtkOutputWindow::GetInstance()->DisplayText(message);
#else
        qInfo() << message;
#endif
      }
    }
  }

public:
  pqInternal()
    : Editor(NULL)
  {
    this->DummyInterpreter->AddObserver(
      vtkCommand::AnyEvent, this, &pqPythonManager::pqInternal::interpreterEvents);

    // import the paraview module now if Python was already
    // initialized (by a startup plugin, for example)
    if (vtkPythonInterpreter::IsInitialized())
    {
      importParaViewModule();
    }
  }
  ~pqInternal() { this->DummyInterpreter->RemoveObservers(vtkCommand::AnyEvent); }

  QTimer StatusBarUpdateTimer;
  QPointer<pqPythonDialog> PythonDialog;
  QPointer<pqPythonMacroSupervisor> MacroSupervisor;
  bool IsPythonTracing;
  QPointer<pqPythonScriptEditor> Editor;
};

//-----------------------------------------------------------------------------
pqPythonManager::pqPythonManager(QObject* _parent /*=null*/)
  : QObject(_parent)
{
  this->Internal = new pqInternal;
  pqApplicationCore* core = pqApplicationCore::instance();
  core->registerManager("PYTHON_MANAGER", this);

  // Create an instance of the macro supervisor
  this->Internal->MacroSupervisor = new pqPythonMacroSupervisor(this);
  this->connect(this->Internal->MacroSupervisor, SIGNAL(executeScriptRequested(const QString&)),
    SLOT(executeScriptAndRender(const QString&)));

  // Listen the signal when a macro wants to be edited
  QObject::connect(this->Internal->MacroSupervisor, SIGNAL(onEditMacro(const QString&)), this,
    SLOT(editMacro(const QString&)));

  // Listen for signal when server is about to be removed
  this->connect(core->getServerManagerModel(), SIGNAL(aboutToRemoveServer(pqServer*)), this,
    SLOT(onRemovingServer(pqServer*)));

  // Init Python tracing ivar
  this->Internal->IsPythonTracing = false;
  this->Internal->Editor = NULL;

  // Start StatusBar message update timer
  connect(
    &this->Internal->StatusBarUpdateTimer, SIGNAL(timeout()), this, SLOT(updateStatusMessage()));
  this->Internal->StatusBarUpdateTimer.start(5000); // 5 second
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
  if (this->Internal->Editor && !this->Internal->Editor->parent())
  {
    delete this->Internal->Editor;
  }
  delete this->Internal;
}

//-----------------------------------------------------------------------------
bool pqPythonManager::interpreterIsInitialized()
{
  return vtkPythonInterpreter::IsInitialized();
}

//-----------------------------------------------------------------------------
pqPythonDialog* pqPythonManager::pythonShellDialog()
{
  // Create the dialog and initialize the interpreter the first time this
  // method is called.
  if (!this->Internal->PythonDialog)
  {
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    vtkPythonInterpreter::Initialize();
    this->Internal->PythonDialog = new pqPythonDialog(pqCoreUtilities::mainWidget());
    this->Internal->PythonDialog->shell()->setupInterpreter();
    QApplication::restoreOverrideCursor();
  }
  return this->Internal->PythonDialog;
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
void pqPythonManager::executeScript(const QString& filename)
{
  pqPythonDialog* dialog = this->pythonShellDialog();
  dialog->runScript(QStringList(filename));
}

//-----------------------------------------------------------------------------
void pqPythonManager::executeScriptAndRender(const QString& filename)
{
  this->executeScript(filename);
  pqApplicationCore::instance()->render();
}

//-----------------------------------------------------------------------------
void pqPythonManager::onRemovingServer(pqServer* /*server*/)
{
  if (this->Internal->PythonDialog)
  {
    this->Internal->PythonDialog->shell()->reset();
  }
}

//----------------------------------------------------------------------------
QString pqPythonManager::getTraceString()
{
  return vtkSMTrace::GetActiveTracer() ? vtkSMTrace::GetActiveTracer()->GetCurrentTrace().c_str()
                                       : "";
}

//-----------------------------------------------------------------------------
void pqPythonManager::editTrace(const QString& txt, bool update)
{
  // Create the editor if needed and only the first time
  bool new_editor = this->Internal->Editor == NULL;
  if (!this->Internal->Editor)
  {
    this->Internal->Editor = new pqPythonScriptEditor(pqCoreUtilities::mainWidget());
    this->Internal->Editor->setPythonManager(this);
  }

  QString traceString = txt.isEmpty() ? this->getTraceString() : txt;
  this->Internal->Editor->show();
  if (new_editor || !update) // don't raise the window if we are just updating the trace.
  {
    this->Internal->Editor->raise();
    this->Internal->Editor->activateWindow();
  }
  if (update || this->Internal->Editor->newFile())
  {
    this->Internal->Editor->setText(traceString);
  }
}

//----------------------------------------------------------------------------
void pqPythonManager::updateMacroList()
{
  this->Internal->MacroSupervisor->updateMacroList();
}

//----------------------------------------------------------------------------
void pqPythonManager::addMacro(const QString& fileName)
{
  QString userMacroDir = pqCoreUtilities::getParaViewUserDirectory() + "/Macros";
  QDir dir;
  dir.setPath(userMacroDir);
  // Copy macro file to user directory
  if (!dir.exists(userMacroDir) && !dir.mkpath(userMacroDir))
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
  if (!this->Internal->Editor)
  {
    this->Internal->Editor = new pqPythonScriptEditor(pqCoreUtilities::mainWidget());
    this->Internal->Editor->setPythonManager(this);
  }

  this->Internal->Editor->show();
  this->Internal->Editor->raise();
  this->Internal->Editor->activateWindow();
  this->Internal->Editor->open(fileName);
}
//----------------------------------------------------------------------------
void pqPythonManager::updateStatusMessage()
{
  if (this->Internal->IsPythonTracing)
  {
    QMainWindow* mainWindow = qobject_cast<QMainWindow*>(pqCoreUtilities::mainWidget());
    if (mainWindow)
    {
      mainWindow->statusBar()->showMessage("Recording python trace...");
    }
  }
}
