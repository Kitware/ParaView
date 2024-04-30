// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
#include "vtkPythonInteractiveInterpreter.h"
#include "vtkPythonInterpreter.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QInputDialog>
#include <QLayout>
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

  pqPythonManagerOutputWindow(const pqPythonManagerOutputWindow&) = delete;
  void operator=(const pqPythonManagerOutputWindow&) = delete;
};
vtkStandardNewMacro(pqPythonManagerOutputWindow);

//-----------------------------------------------------------------------------
class pqPythonManagerStdIOHelper
{
  std::string LastOutputText;

public:
  //-----------------------------------------------------------------------------
  void rawOutput(vtkObject*, unsigned long, void* calldata)
  {
    auto* strData = reinterpret_cast<const char*>(calldata);
    if (strData != nullptr)
    {
      this->LastOutputText = strData;
    }
  }

  //-----------------------------------------------------------------------------
  void rawInput(vtkObject*, unsigned long, void* calldata)
  {
    std::string* strData = reinterpret_cast<std::string*>(calldata);
    bool ok;
    const std::string title = "Python script requested input";
    std::string label = "Input: ";
    if (!this->LastOutputText.empty())
    {
      label.swap(this->LastOutputText);
      label += ": ";
    }
    QString inputText = QInputDialog::getText(pqCoreUtilities::mainWidget(),
      QCoreApplication::translate("pqPythonManagerStdIOHelper", title.c_str()),
      QCoreApplication::translate("pqPythonManagerStdIOHelper", label.c_str()), QLineEdit::Normal,
      QString(), &ok);
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
pqPythonMacroSupervisor* pqPythonManager::macroSupervisor() const
{
  return this->Internal->MacroSupervisor;
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
void pqPythonManager::executeCode(
  const QByteArray& code, const QVector<QByteArray>& pre_push, const QVector<QByteArray>& post_push)
{
  pqPythonManagerStdIOHelper helper;

  // we capture messages from the script so that when the end up on the
  // terminal they are grouped as single message, otherwise they get split at
  // each "\n" since that how Python sends those messages over to us.
  vtkNew<pqPythonManagerOutputWindow> owindow;
  vtkSmartPointer<vtkOutputWindow> old = vtkOutputWindow::GetInstance();
  vtkOutputWindow::SetInstance(owindow);
  const bool prevCapture = vtkPythonInterpreter::GetCaptureStdin();
  vtkPythonInterpreter::SetCaptureStdin(true);

  vtkNew<vtkPythonInteractiveInterpreter> interp;
  interp->AddObserver(vtkCommand::SetOutputEvent, &helper, &pqPythonManagerStdIOHelper::rawOutput);
  interp->AddObserver(vtkCommand::UpdateEvent, &helper, &pqPythonManagerStdIOHelper::rawInput);
  for (const auto& instr : pre_push)
  {
    interp->Push(instr.data());
  }
  interp->RunStringWithConsoleLocals(code.data());
  for (const auto& instr : post_push)
  {
    interp->Push(instr.data());
  }
  vtkPythonInterpreter::SetCaptureStdin(prevCapture);
  vtkOutputWindow::SetInstance(old);
  interp->RemoveObservers(vtkCommand::UpdateEvent);

  auto txt = owindow->text();
  // vtkPythonInteractiveInterpreter always writes a newline, and if there
  // was no other output from the script, then txt could contain only a
  // newline here. If this is the case, do not display it to the user.
  if (!txt.empty() && txt != "\n")
  {
    vtkOutputWindowDisplayText(txt.c_str());
  }

  auto errorText = owindow->errorText();
  if (!errorText.empty() && errorText != "\n")
  {
    vtkOutputWindowDisplayErrorText(errorText.c_str());
  }
}

//-----------------------------------------------------------------------------
void pqPythonManager::executeScript(const QString& filename, vtkTypeUInt32 location)
{
  // read python script from file
  auto pxm = pqApplicationCore::instance()->getActiveServer()->proxyManager();
  const auto pyScript = pxm->LoadString(filename.toUtf8().data(), location);
  if (pyScript.empty())
  {
    qCritical() << "Failed to open file : " << filename;
    return;
  }

  const QByteArray code = QString(pyScript.c_str()).toLocal8Bit();
  const QVector<QByteArray> pre_cmd = { "import sys",
    QString("__file__ = r'%1'").arg(filename).toUtf8() };
  const QVector<QByteArray> post_cmd = { "del __file__" };
  this->executeCode(code, pre_cmd, post_cmd);
}

//-----------------------------------------------------------------------------
void pqPythonManager::executeScriptAndRender(const QString& filename, vtkTypeUInt32 location)
{
  this->executeScript(filename, location);
  pqApplicationCore::instance()->render();
}

//----------------------------------------------------------------------------
void pqPythonManager::updateMacroList()
{
  this->Internal->MacroSupervisor->updateMacroList();
}

//----------------------------------------------------------------------------
void pqPythonManager::addMacro(const QString& fileName, vtkTypeUInt32 location)
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

  // read python script from file
  auto pxm = pqApplicationCore::instance()->getActiveServer()->proxyManager();
  const auto pyScript = pxm->LoadString(fileName.toUtf8().data(), location);

  // write python script to file in user directory
  QFile file(expectedFilePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    qWarning() << "Could not create user Macro file:" << expectedFilePath;
    return;
  }
  QTextStream out(&file);
  out << pyScript.c_str();
  file.close();

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
