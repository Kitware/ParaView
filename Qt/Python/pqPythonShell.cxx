/*=========================================================================

   Program: ParaView
   Module:    pqPythonShell.cxx

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
#include "vtkPython.h"

#include "pqPythonShell.h"
#include "ui_pqPythonShell.h"

#include "pqApplicationCore.h"
#include "pqConsoleWidget.h"
#include "pqFileDialog.h"
#include "pqPythonShellCompleter.h"
#include "pqUndoStack.h"

#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVOptions.h"

#include "vtkPythonCompatibility.h"
#include "vtkPythonInteractiveInterpreter.h"
#include "vtkPythonInterpreter.h"
#include "vtkSmartPointer.h"
#include "vtkStringOutputWindow.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QCursor>
#include <QFile>
#include <QInputDialog>
#include <QPointer>
#include <QStringListModel>
#include <QTextCharFormat>
#include <QVBoxLayout>
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
  pqPythonShellOutputWindow() {}
  ~pqPythonShellOutputWindow() override {}
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
  Ui::PythonShell Ui;

  pqInternals(pqPythonShell* self)
    : Parent(self)
    , Interpreter()
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
      this->Parent->printString("\nresetting ...\n", pqPythonShell::ERROR);
      this->initializeInterpreter();
    }
  }

  vtkPythonInteractiveInterpreter* interpreter() const { return this->Interpreter; }

private:
  /**
   * Will initialize (or re-initialize) the interpreter.
   */
  void initializeInterpreter()
  {
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    vtkPythonInterpreter::Initialize();
    assert(vtkPythonInterpreter::IsInitialized());

    // Print the default Python interpreter greeting.
    this->Parent->printString(
      QString("\nPython %1 on %2\n").arg(Py_GetVersion()).arg(Py_GetPlatform()), OUTPUT);

    // Note that we assume each line of the preamble is a complete statement
    // (i.e., no multi-line statements):
    for (const QString& line : pqPythonShell::preamble())
    {
      this->Parent->prompt();
      this->Parent->printString(line + "\n");
      this->Interpreter->Push(line.toLocal8Bit().data());
    }
    this->Parent->prompt();

    const Ui::PythonShell& ui = this->Ui;
    ui.clearButton->setEnabled(true);
    ui.resetButton->setEnabled(true);
    this->InterpreterInitialized = true;

    QApplication::restoreOverrideCursor();
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
  QString string = text;
  if (!string.isEmpty())
  {
    QTextCharFormat format = consoleWidget->getFormat();
    switch (mode)
    {
      case OUTPUT:
        format.setForeground(QColor(0, 150, 0));
        break;

      case ERROR:
        format.setForeground(QColor(255, 0, 0));
        break;

      case STATUS:
      default:
        format.setForeground(QApplication::palette().highlight().color());
    }
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
  this->Internals->interpreter()->RunStringWithConsoleLocals(command.toLocal8Bit().data());
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
  foreach (QString line, lines)
  {
    bool isMultilineStatement = this->Internals->interpreter()->Push(line.toLocal8Bit().data());
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
    case vtkCommand::UpdateEvent:
    {
      std::string* strData = reinterpret_cast<std::string*>(calldata);
      bool ok;
      QString inputText = QInputDialog::getText(this, tr("Enter Input requested by Python"),
        tr("Input: "), QLineEdit::Normal, QString(), &ok);
      if (ok)
      {
        *strData = inputText.toStdString();
      }
    }
    break;
  }
}

//-----------------------------------------------------------------------------
void pqPythonShell::runScript()
{
  pqFileDialog dialog(
    nullptr, this, tr("Run Script"), QString(), QString(tr("Python Script (*.py);;All files (*)")));
  dialog.setObjectName("PythonShellRunScriptDialog");
  dialog.setFileMode(pqFileDialog::ExistingFile);
  if (dialog.exec() == QDialog::Accepted)
  {
    for (const QString& filename : dialog.getSelectedFiles())
    {
      QFile file(filename);
      if (file.open(QIODevice::ReadOnly))
      {
        QByteArray code;
        // First inject code to let the script know its own path
        code.append(QString("__file__ = r'%1'\n").arg(filename));
        // Then append the file content
        code.append(file.readAll());
        code.append("\n");
        code.append("del __file__\n");
        this->executeScript(code.data());
      }
      else
      {
        qCritical() << "Error opening '" << filename << "'";
      }
    }
  }
}
