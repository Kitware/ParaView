// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
// Include vtkPython.h first to avoid python??_d.lib not found linking error on
// Windows debug builds.
#include "vtkPython.h"

#include "pqPythonShell.h"
#include "ui_pqPythonShell.h"

#include "pqApplicationCore.h"
#include "pqConsoleWidget.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqPythonShellCompleter.h"
#include "pqScopedOverrideCursor.h"
#include "pqServer.h"
#include "pqUndoStack.h"

#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPythonInteractiveInterpreter.h"
#include "vtkPythonInterpreter.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"
#include "vtkStringOutputWindow.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QCursor>
#include <QFile>
#include <QInputDialog>
#include <QPointer>
#include <QTextCharFormat>
#include <QtDebug>

#include <cassert>

QStringList pqPythonShell::Preamble;

//-----------------------------------------------------------------------------
class pqPythonShellOutputWindow : public vtkOutputWindow
{
  QPointer<pqPythonShell> Shell;

public:
  static pqPythonShellOutputWindow* New();
  vtkTypeMacro(pqPythonShellOutputWindow, vtkOutputWindow);

  void SetShell(pqPythonShell* shell) { this->Shell = shell; }

  void DisplayText(const char* txt) override
  {
    if (this->Shell)
    {
      this->Shell->printString(txt, pqPythonShell::OUTPUT);
    }
  }
  void DisplayErrorText(const char* txt) override
  {
    if (this->Shell)
    {
      this->Shell->printString(txt, pqPythonShell::ERROR);
    }
  }

protected:
  pqPythonShellOutputWindow() = default;
  ~pqPythonShellOutputWindow() override = default;

private:
  pqPythonShellOutputWindow(const pqPythonShellOutputWindow&) = delete;
  void operator=(const pqPythonShellOutputWindow&) = delete;
};

vtkStandardNewMacro(pqPythonShellOutputWindow);

//-----------------------------------------------------------------------------
class pqPythonShell::pqInternals
{
  QPointer<pqPythonShell> Parent;
  vtkNew<vtkPythonInteractiveInterpreter> Interpreter;
  vtkNew<pqPythonShellOutputWindow> MessageCapture;
  vtkSmartPointer<vtkOutputWindow> OldInstance;
  bool OldCapture;
  int ExecutionCounter;
  bool InterpreterInitialized;

public:
  std::string LastOutputText;
  Ui::PythonShell Ui;

  pqInternals(pqPythonShell* self)
    : Parent(self)
    , OldCapture(false)
    , ExecutionCounter(0)
    , InterpreterInitialized(false)
  {
    this->MessageCapture->SetShell(self);
    this->Ui.setupUi(self);
    self->connect(this->Ui.clearButton, SIGNAL(clicked()), SLOT(clear()));
    self->connect(this->Ui.resetButton, SIGNAL(clicked()), SLOT(reset()));
    self->connect(this->Ui.runScriptButton, SIGNAL(clicked()), SLOT(runScript()));
  }

  /**
   * Must be called before executing a Python snippet in the shell.
   * Does several things:
   * 1. ensures that Python interpreter is initialized.
   * 2. ensures that the input/output streams are captured.
   */
  void begin()
  {
    assert(this->ExecutionCounter >= 0);
    if (this->ExecutionCounter == 0)
    {
      assert(this->OldInstance == nullptr);
      Q_EMIT this->Parent->executing(true);

      if (this->isInterpreterInitialized() == false)
      {
        this->initializeInterpreter();
      }

      this->OldInstance = vtkOutputWindow::GetInstance();
      vtkOutputWindow::SetInstance(this->MessageCapture);
      this->OldCapture = vtkPythonInterpreter::GetCaptureStdin();
      vtkPythonInterpreter::SetCaptureStdin(true);
    }
    this->ExecutionCounter++;
  }

  /**
   * Must match a `begin` and should be called when done with Python snippet
   * processing. Undoes all overrides set up in `begin`.
   */
  void end()
  {
    this->ExecutionCounter--;
    assert(this->ExecutionCounter >= 0);
    if (this->ExecutionCounter == 0)
    {
      vtkPythonInterpreter::SetCaptureStdin(this->OldCapture);
      this->OldCapture = false;
      vtkOutputWindow::SetInstance(this->OldInstance);
      this->OldInstance = nullptr;
      Q_EMIT this->Parent->executing(false);
    }
  }

  bool isExecuting() const { return this->ExecutionCounter > 0; }
  bool isInterpreterInitialized() const { return this->InterpreterInitialized; }

  /**
   * Resets the interpreter. Unlike `initializeInterpreter`, this will have no
   * effect if the interpreter hasn't been initialized yet.
   */
  void reset()
  {
    if (this->isInterpreterInitialized())
    {
      this->Parent->printString(QString("\n%1 ...\n").arg(tr("resetting")), pqPythonShell::ERROR);
      this->initializeInterpreter();
    }
  }

  vtkPythonInteractiveInterpreter* interpreter() const { return this->Interpreter; }

  QColor foregroundColor(pqPythonShell::PrintMode mode) const
  {
    switch (mode)
    {
      case pqPythonShell::OUTPUT:
        return (pqCoreUtilities::isDarkTheme() ? QColor(Qt::green) : QColor(Qt::darkGreen));
      case pqPythonShell::ERROR:
        return (pqCoreUtilities::isDarkTheme() ? QColor(Qt::red) : QColor(Qt::darkRed));
      case pqPythonShell::STATUS:
      default:
        return pqCoreUtilities::isDarkTheme() ? QColor(Qt::cyan)
                                              : QApplication::palette().color(QPalette::Highlight);
    }
  }

private:
  /**
   * Will initialize (or re-initialize) the interpreter.
   */
  void initializeInterpreter()
  {
    pqScopedOverrideCursor scopedWaitCursor(Qt::WaitCursor);

    vtkPythonInterpreter::Initialize();
    assert(vtkPythonInterpreter::IsInitialized());

    // Print the default Python interpreter greeting.
    this->Parent->printString(
      tr("\nPython %1 on %2\n").arg(Py_GetVersion()).arg(Py_GetPlatform()), OUTPUT);

    // Note that we assume each line of the preamble is a complete statement
    // (i.e., no multi-line statements):
    for (const QString& line : pqPythonShell::preamble())
    {
      this->Parent->prompt();
      this->Parent->printString(line + "\n");
      this->Interpreter->Push(line.toUtf8().data());
    }
    this->Parent->prompt();

    const Ui::PythonShell& ui = this->Ui;
    ui.clearButton->setEnabled(true);
    ui.resetButton->setEnabled(true);
    this->InterpreterInitialized = true;
  }
};

//-----------------------------------------------------------------------------
pqPythonShell::pqPythonShell(QWidget* parentObject, Qt::WindowFlags _flags)
  : Superclass(parentObject, _flags)
  , ConsoleWidget(nullptr)
  , Prompt(pqPythonShell::PS1())
  , Prompted(false)
  , Internals(new pqPythonShell::pqInternals(this))
{
  // The default preamble loads paraview.simple:
  if (pqPythonShell::Preamble.empty())
  {
    pqPythonShell::Preamble += "from paraview.simple import *";
  }

  Ui::PythonShell& ui = this->Internals->Ui;

  // install event filter to initialize Python on request.
  // we use queued connection so that the cursor ends up after the prompt.
  // Otherwise if user clicked for focus, the cursor will end up where ever the
  // user clicked.
  this->connect(
    ui.consoleWidget, SIGNAL(consoleFocusInEvent()), SLOT(initialize()), Qt::QueuedConnection);

  // Setup completer for the console widget.
  pqPythonShellCompleter* completer =
    new pqPythonShellCompleter(this, this->Internals->interpreter());
  ui.consoleWidget->setCompleter(completer);

  // Accept user input from the console and push it into the Python interpreter.
  this->connect(
    ui.consoleWidget, SIGNAL(executeCommand(const QString&)), SLOT(pushScript(const QString&)));

  this->Internals->interpreter()->AddObserver(
    vtkCommand::AnyEvent, this, &pqPythonShell::HandleInterpreterEvents);

  // show the prompt so user knows that there's a Python shell to use.
  this->prompt();
}

//-----------------------------------------------------------------------------
pqPythonShell::~pqPythonShell()
{
  this->Internals->interpreter()->RemoveObservers(vtkCommand::AnyEvent);
}

//-----------------------------------------------------------------------------
bool pqPythonShell::isExecuting() const
{
  return this->Internals->isExecuting();
}

//-----------------------------------------------------------------------------
void pqPythonShell::initialize()
{
  if (!this->Internals->isInterpreterInitialized())
  {
    this->Internals->begin();
    this->Internals->end();
  }
}

//-----------------------------------------------------------------------------
void pqPythonShell::reset()
{
  this->Internals->reset();
}

//-----------------------------------------------------------------------------
void pqPythonShell::printString(const QString& text, pqPythonShell::PrintMode mode)
{
  pqConsoleWidget* consoleWidget = this->Internals->Ui.consoleWidget;
  const QString& string = text;
  if (!string.isEmpty())
  {
    QTextCharFormat format = consoleWidget->getFormat();
    format.setForeground(this->Internals->foregroundColor(mode));
    consoleWidget->setFormat(format);
    consoleWidget->printString(string);
    format.setForeground(QApplication::palette().windowText().color());
    consoleWidget->setFormat(format);

    this->Prompted = false;

    // printString by itself should never affect the Prompt, just whether it
    // needs to be shown.
  }
}
//-----------------------------------------------------------------------------
void pqPythonShell::setPreamble(const QStringList& statements)
{
  pqPythonShell::Preamble = statements;
}
//-----------------------------------------------------------------------------
const QStringList& pqPythonShell::preamble()
{
  return pqPythonShell::Preamble;
}

//-----------------------------------------------------------------------------
bool pqPythonShell::prompt(const QString& indent)
{
  if (!this->Prompted)
  {
    this->Prompted = true;

    Ui::PythonShell& ui = this->Internals->Ui;
    QTextCharFormat format = ui.consoleWidget->getFormat();
    format.setForeground(QApplication::palette().windowText().color());
    ui.consoleWidget->setFormat(format);
    ui.consoleWidget->prompt(this->Prompt);
    ui.consoleWidget->printCommand(indent);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void pqPythonShell::printMessage(const QString& text)
{
  this->printString(text, STATUS);
  this->prompt();
}

//-----------------------------------------------------------------------------
void pqPythonShell::clear()
{
  this->Internals->Ui.consoleWidget->clear();
  this->Prompted = false;
  this->prompt();
}

//-----------------------------------------------------------------------------
void pqPythonShell::executeScript(const QString& script)
{

  QString command = script;
  command.replace("\r\n", "\n");
  command.replace("\r", "\n");

  this->Internals->begin();
  this->Internals->interpreter()->RunStringWithConsoleLocals(command.toUtf8().data());
  this->Internals->end();

  CLEAR_UNDO_STACK();
  this->prompt();
}

//-----------------------------------------------------------------------------
void pqPythonShell::pushScript(const QString& script)
{
  QString command = script;
  command.replace("\r\n", "\n");
  command.replace("\r", "\n");
  QStringList lines = script.split("\n");

  this->Prompted = false;
  this->Internals->begin();
  Q_FOREACH (QString line, lines)
  {
    bool isMultilineStatement = this->Internals->interpreter()->Push(line.toUtf8().data());
    this->Prompt = isMultilineStatement ? pqPythonShell::PS2() : pqPythonShell::PS1();
  }
  this->Internals->end();
  this->prompt();
  CLEAR_UNDO_STACK();
}

//-----------------------------------------------------------------------------
void* pqPythonShell::consoleLocals()
{
  // this ensures that the interpreter is initialized before we access its
  // locals.
  this->initialize();
  return this->Internals->interpreter()->GetInteractiveConsoleLocalsPyObject();
}

//-----------------------------------------------------------------------------
void pqPythonShell::setFontSize(int fontSize)
{
  pqConsoleWidget* consoleWidget = this->Internals->Ui.consoleWidget;
  consoleWidget->setFontSize(fontSize);
}

//-----------------------------------------------------------------------------
void pqPythonShell::HandleInterpreterEvents(vtkObject*, unsigned long eventid, void* calldata)
{
  if (!this->isExecuting())
  {
    // not our event. ignore it.
    return;
  }

  switch (eventid)
  {
    case vtkCommand::SetOutputEvent:
    {
      auto* strData = reinterpret_cast<const char*>(calldata);
      if (strData != nullptr)
      {
        this->Internals->LastOutputText = strData;
      }
      break;
    }
    case vtkCommand::UpdateEvent:
    {
      std::string* strData = reinterpret_cast<std::string*>(calldata);
      bool ok;
      const std::string title = "Python script requested input";
      std::string label = "Input: ";
      if (!this->Internals->LastOutputText.empty())
      {
        label.swap(this->Internals->LastOutputText);
        label += ": ";
      }
      QString inputText = QInputDialog::getText(
        this, tr(title.c_str()), tr(label.c_str()), QLineEdit::Normal, QString(), &ok);
      if (ok)
      {
        *strData = inputText.toStdString();
      }
    }
    break;
    default:
      break;
  }
}

//-----------------------------------------------------------------------------
void pqPythonShell::runScript()
{
  pqServer* server = pqApplicationCore::instance()->getActiveServer();
  pqFileDialog dialog(server, this, tr("Run Script"), QString(),
    tr("Python Files") + QString(" (*.py);;") + tr("All Files") + QString(" (*)"), false, false);
  dialog.setObjectName("PythonShellRunScriptDialog");
  dialog.setFileMode(pqFileDialog::ExistingFile);
  if (dialog.exec() == QDialog::Accepted)
  {
    const auto pxm = server->proxyManager();
    const vtkTypeUInt32 location = dialog.getSelectedLocation();
    for (const QString& filename : dialog.getSelectedFiles())
    {
      const std::string contents = pxm->LoadString(filename.toStdString().c_str(), location);
      if (!contents.empty())
      {
        QString code;
        // First inject code to let the script know its own path
        code.append(QString("__file__ = r'%1'\n").arg(filename));
        // Then append the file content
        code.append(contents.c_str());
        code.append("\n");
        code.append("del __file__\n");
        this->executeScript(code.toUtf8().data());
      }
      else
      {
        qCritical() << tr("Error: script ") << filename << tr(" was empty or could not be opened.");
      }
    }
  }
}
