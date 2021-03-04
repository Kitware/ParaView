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
#include "pqPythonMacroSupervisor.h"
#include "pqPythonScriptEditor.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"
#include "vtkPVConfig.h"
#include "vtkPythonInteractiveInterpreter.h"
#include "vtkPythonInterpreter.h"
#include "vtkSmartPointer.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QLayout>
#include <QMainWindow>
#include <QSplitter>
#include <QStatusBar>
#include <QTextStream>
#include <sstream>

//-----------------------------------------------------------------------------
struct pqPythonManager::pqInternal
{
  QPointer<pqPythonMacroSupervisor> MacroSupervisor;
};

//-----------------------------------------------------------------------------
class pqPythonManagerOutputWindow : public vtkOutputWindow
{
  std::ostringstream TextStream;
  std::ostringstream ErrorStream;

public:
  static pqPythonManagerOutputWindow* New();
  vtkTypeMacro(pqPythonManagerOutputWindow, vtkOutputWindow);

  void DisplayText(const char* txt) override { this->TextStream << txt; }

  void DisplayErrorText(const char* txt) override { this->ErrorStream << txt; }

  std::string text() const { return this->TextStream.str(); }
  std::string errorText() const { return this->ErrorStream.str(); }
private:
  pqPythonManagerOutputWindow() = default;
  ~pqPythonManagerOutputWindow() override = default;

private:
  pqPythonManagerOutputWindow(const pqPythonManagerOutputWindow&) = delete;
  void operator=(const pqPythonManagerOutputWindow&) = delete;
};
vtkStandardNewMacro(pqPythonManagerOutputWindow);

//-----------------------------------------------------------------------------
class pqPythonManagerRawInputHelper
{
public:
  void rawInput(vtkObject*, unsigned long, void* calldata)
  {
    std::string* strData = reinterpret_cast<std::string*>(calldata);
    bool ok;
    QString inputText = QInputDialog::getText(pqCoreUtilities::mainWidget(),
      QCoreApplication::translate("pqPythonManager", "Enter Input requested by Python"),
      QCoreApplication::translate("pqPythonManager", "Input: "), QLineEdit::Normal, QString(), &ok);
    if (ok)
    {
      *strData = inputText.toStdString();
    }
  }
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
}

//-----------------------------------------------------------------------------
pqPythonManager::~pqPythonManager()
{
  pqApplicationCore::instance()->unRegisterManager("PYTHON_MANAGER");
  delete this->Internal;
}

//-----------------------------------------------------------------------------
bool pqPythonManager::initializeInterpreter()
{
  return vtkPythonInterpreter::Initialize();
}

//-----------------------------------------------------------------------------
bool pqPythonManager::interpreterIsInitialized()
{
  return vtkPythonInterpreter::IsInitialized();
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
  QFile file(filename);
  if (file.open(QIODevice::ReadOnly))
  {
    const QByteArray code = file.readAll();
    pqPythonManagerRawInputHelper helper;

    // we capture messages from the script so that when the end up on the
    // terminal they are grouped as single message, otherwise they get split at
    // each "\n" since that how Python sends those messages over to us.
    vtkNew<pqPythonManagerOutputWindow> owindow;
    vtkSmartPointer<vtkOutputWindow> old = vtkOutputWindow::GetInstance();
    vtkOutputWindow::SetInstance(owindow);
    const bool prevCapture = vtkPythonInterpreter::GetCaptureStdin();
    vtkPythonInterpreter::SetCaptureStdin(true);

    vtkNew<vtkPythonInteractiveInterpreter> interp;
    interp->AddObserver(vtkCommand::UpdateEvent, &helper, &pqPythonManagerRawInputHelper::rawInput);
    interp->Push("import sys");
    interp->Push(QString("__file__ = r'%1'").arg(filename).toLocal8Bit().data());
    interp->RunStringWithConsoleLocals(code.data());
    interp->Push("del __file__");
    vtkPythonInterpreter::SetCaptureStdin(prevCapture);
    vtkOutputWindow::SetInstance(old);
    interp->RemoveObservers(vtkCommand::UpdateEvent);

    auto txt = owindow->text();
    if (txt.size())
    {
      vtkOutputWindowDisplayText(txt.c_str());
    }

    auto errorText = owindow->errorText();
    if (errorText.size())
    {
      vtkOutputWindowDisplayErrorText(errorText.c_str());
    }
  }
  else
  {
    qWarning() << "Error opening '" << filename << "'.";
  }
}

//-----------------------------------------------------------------------------
void pqPythonManager::executeScriptAndRender(const QString& filename)
{
  this->executeScript(filename);
  pqApplicationCore::instance()->render();
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
  pqPythonScriptEditor* pyEditor = pqPythonScriptEditor::getUniqueInstance();

  pyEditor->setPythonManager(this);
  pyEditor->show();
  pyEditor->raise();
  pyEditor->activateWindow();
  pyEditor->open(fileName);
}
