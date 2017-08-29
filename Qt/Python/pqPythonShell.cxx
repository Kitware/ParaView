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
#include "pqUndoStack.h"
#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVOptions.h"
#include "vtkProcessModule.h"
#include "vtkPythonCompatibility.h"
#include "vtkPythonInteractiveInterpreter.h"
#include "vtkPythonInterpreter.h"
#include "vtkStdString.h"
#include "vtkStringOutputWindow.h"
#include "vtkWeakPointer.h"

#include <QApplication>
#include <QCursor>
#include <QFile>
#include <QInputDialog>
#include <QPointer>
#include <QStringListModel>
#include <QTextCharFormat>
#include <QVBoxLayout>
#include <QtDebug>

QStringList pqPythonShell::Preamble;

class pqPythonShellCompleter : public pqConsoleWidgetCompleter
{
  vtkWeakPointer<vtkPythonInteractiveInterpreter> Interpreter;

public:
  pqPythonShellCompleter(pqPythonShell& p, vtkPythonInteractiveInterpreter* interp)
  {
    this->Interpreter = interp;
    this->setParent(&p);
  }

  virtual void updateCompletionModel(const QString& completion)
  {
    // Start by clearing the model
    this->setModel(0);

    // Don't try to complete the empty string
    if (completion.isEmpty())
    {
      return;
    }

    // Search backward through the string for usable characters
    QString textToComplete;
    for (int i = completion.length() - 1; i >= 0; --i)
    {
      QChar c = completion.at(i);
      if (c.isLetterOrNumber() || c == '.' || c == '_')
      {
        textToComplete.prepend(c);
      }
      else
      {
        break;
      }
    }

    // Split the string at the last dot, if one exists
    QString lookup;
    QString compareText = textToComplete;
    int dot = compareText.lastIndexOf('.');
    if (dot != -1)
    {
      lookup = compareText.mid(0, dot);
      compareText = compareText.mid(dot + 1);
    }

    // Lookup python names
    QStringList attrs;
    if (!lookup.isEmpty() || !compareText.isEmpty())
    {
      attrs = this->getPythonAttributes(lookup);
    }

    // Initialize the completion model
    if (!attrs.isEmpty())
    {
      this->setCompletionMode(QCompleter::PopupCompletion);
      this->setModel(new QStringListModel(attrs, this));
      this->setCaseSensitivity(Qt::CaseInsensitive);
      this->setCompletionPrefix(compareText.toLower());
      this->popup()->setCurrentIndex(this->completionModel()->index(0, 0));
    }
  }

  /// Given a python variable name, lookup its attributes and return them in a
  /// string list.
  QStringList getPythonAttributes(const QString& pythonObjectName)
  {
    vtkPythonScopeGilEnsurer gilEnsurer;
    if (this->Interpreter == NULL ||
      this->Interpreter->GetInteractiveConsoleLocalsPyObject() == NULL)
    {
      return QStringList();
    }

    PyObject* object =
      reinterpret_cast<PyObject*>(this->Interpreter->GetInteractiveConsoleLocalsPyObject());
    Py_INCREF(object);

    if (!pythonObjectName.isEmpty())
    {
      QStringList tmpNames = pythonObjectName.split('.');
      for (int i = 0; i < tmpNames.size() && object; ++i)
      {
        QByteArray tmpName = tmpNames.at(i).toLocal8Bit();
        PyObject* prevObj = object;
        if (PyDict_Check(object))
        {
          object = PyDict_GetItemString(object, tmpName.data());
          Py_XINCREF(object);
        }
        else
        {
          object = PyObject_GetAttrString(object, tmpName.data());
        }
        Py_DECREF(prevObj);
      }
      PyErr_Clear();
    }

    QStringList results;
    if (object)
    {
      PyObject* keys = NULL;
      bool is_dict = PyDict_Check(object);
      if (is_dict)
      {
        keys = PyDict_Keys(object); // returns *new* reference.
      }
      else
      {
        keys = PyObject_Dir(object); // returns *new* reference.
      }
      if (keys)
      {
        PyObject* key;
        PyObject* value;
        QString keystr;
        int nKeys = PyList_Size(keys);
        for (int i = 0; i < nKeys; ++i)
        {
          key = PyList_GetItem(keys, i);
          if (is_dict)
          {
            value = PyDict_GetItem(object, key); // Return value: Borrowed reference.
            Py_XINCREF(value);                   // so we can use Py_DECREF later.
          }
          else
          {
            value = PyObject_GetAttr(object, key); // Return value: New refernce.
          }
          if (!value)
          {
            continue;
          }
          results << PyString_AsString(key);
          Py_DECREF(value);

          // Clear out any errors that may have occurred.
          PyErr_Clear();
        }
        Py_DECREF(keys);
      }
      Py_DECREF(object);
    }
    return results;
  }
};

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
  pqPythonShellOutputWindow(const pqPythonShellOutputWindow&) VTK_DELETE_FUNCTION;
  void operator=(const pqPythonShellOutputWindow&) VTK_DELETE_FUNCTION;
};

vtkStandardNewMacro(pqPythonShellOutputWindow);

//-----------------------------------------------------------------------------
class pqPythonShell::pqInternals
{
  QPointer<pqPythonShell> Parent;
  vtkNew<pqPythonShellOutputWindow> MessageCapture;
  vtkSmartPointer<vtkOutputWindow> OldInstance;
  bool OldCapture;
  int ExecutionCounter;

public:
  Ui::PythonShell Ui;

  pqInternals(pqPythonShell* self)
    : Parent(self)
    , OldCapture(false)
    , ExecutionCounter(0)
  {
    this->MessageCapture->SetShell(self);
    this->Ui.setupUi(self);
    self->connect(this->Ui.clearButton, SIGNAL(clicked()), SLOT(clear()));
    self->connect(this->Ui.resetButton, SIGNAL(clicked()), SLOT(reset()));
    self->connect(this->Ui.runScriptButton, SIGNAL(clicked()), SLOT(runScript()));
  }

  void begin()
  {
    Q_ASSERT(this->ExecutionCounter >= 0);
    if (this->ExecutionCounter == 0)
    {
      Q_ASSERT(this->OldInstance == nullptr);
      emit this->Parent->executing(true);

      this->OldInstance = vtkOutputWindow::GetInstance();
      vtkOutputWindow::SetInstance(this->MessageCapture);
      this->OldCapture = vtkPythonInterpreter::GetCaptureStdin();
      vtkPythonInterpreter::SetCaptureStdin(true);
    }
    this->ExecutionCounter++;
  }

  void end()
  {
    this->ExecutionCounter--;
    Q_ASSERT(this->ExecutionCounter >= 0);
    if (this->ExecutionCounter == 0)
    {
      vtkPythonInterpreter::SetCaptureStdin(this->OldCapture);
      this->OldCapture = false;
      vtkOutputWindow::SetInstance(this->OldInstance);
      this->OldInstance = nullptr;
      emit this->Parent->executing(false);
    }
  }

  bool isExecuting() const { return this->ExecutionCounter > 0; }
};

//-----------------------------------------------------------------------------
pqPythonShell::pqPythonShell(QWidget* parentObject, Qt::WindowFlags _flags)
  : Superclass(parentObject, _flags)
  , ConsoleWidget(nullptr)
  , Interpreter(vtkPythonInteractiveInterpreter::New())
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
  ui.defaultWidget->installEventFilter(this);

  // Setup completer for the console widget.
  pqPythonShellCompleter* completer = new pqPythonShellCompleter(*this, this->Interpreter);
  ui.consoleWidget->setCompleter(completer);

  // Accept user input from the console and push it into the Python interpreter.
  this->connect(
    ui.consoleWidget, SIGNAL(executeCommand(const QString&)), SLOT(pushScript(const QString&)));

  this->Interpreter->AddObserver(
    vtkCommand::AnyEvent, this, &pqPythonShell::HandleInterpreterEvents);
}

//-----------------------------------------------------------------------------
pqPythonShell::~pqPythonShell()
{
  this->Interpreter->RemoveObservers(vtkCommand::AnyEvent);
  this->Interpreter->Delete();
  this->Interpreter = NULL;
}

//-----------------------------------------------------------------------------
bool pqPythonShell::isExecuting() const
{
  return this->Internals->isExecuting();
}

//-----------------------------------------------------------------------------
bool pqPythonShell::eventFilter(QObject* obj, QEvent* evt)
{
  if (evt->type() == QEvent::MouseButtonDblClick)
  {
    this->setupInterpreter();
    const Ui::PythonShell& ui = this->Internals->Ui;
    ui.consoleWidget->takeFocus();
    return true;
  }
  return this->Superclass::eventFilter(obj, evt);
}

//-----------------------------------------------------------------------------
void pqPythonShell::initialize()
{
  const Ui::PythonShell& ui = this->Internals->Ui;
  if (ui.stackedWidget->currentWidget() != ui.consoleWidget)
  {
    this->setupInterpreter();
  }
}

//-----------------------------------------------------------------------------
void pqPythonShell::setupInterpreter()
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  vtkPythonInterpreter::Initialize();
  Q_ASSERT(vtkPythonInterpreter::IsInitialized());

  this->Internals->begin();

  // Print the default Python interpreter greeting.
  this->printString(
    QString("Python %1 on %2\n").arg(Py_GetVersion()).arg(Py_GetPlatform()), OUTPUT);

  // Note that we assume each line of the preamble is a complete statement
  // (i.e., no multi-line statements):
  foreach (QString line, this->Preamble)
  {
    this->prompt();
    this->printString(line + "\n");
    this->pushScript(line);
  }
  this->prompt();

  const Ui::PythonShell& ui = this->Internals->Ui;
  ui.stackedWidget->setCurrentWidget(ui.consoleWidget);
  ui.clearButton->setEnabled(true);
  ui.resetButton->setEnabled(true);
  QApplication::restoreOverrideCursor();

  this->Internals->end();
}

//-----------------------------------------------------------------------------
void pqPythonShell::reset()
{
  this->Internals->begin();
  const Ui::PythonShell& ui = this->Internals->Ui;
  if (ui.stackedWidget->currentWidget() == ui.consoleWidget)
  {
    this->Interpreter->Reset();
    this->printString("\n...resetting...\n", ERROR);
    this->setupInterpreter();
  }
  this->Internals->end();
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
        format.setForeground(QColor(0, 0, 150));
    }
    consoleWidget->setFormat(format);
    consoleWidget->printString(string);
    format.setForeground(QColor(0, 0, 0));
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
bool pqPythonShell::prompt(const QString& indent)
{
  if (!this->Prompted)
  {
    this->Prompted = true;

    Ui::PythonShell& ui = this->Internals->Ui;
    QTextCharFormat format = ui.consoleWidget->getFormat();
    format.setForeground(QColor(0, 0, 0));
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
  this->Internals->begin();

  QString command = script;
  command.replace("\r\n", "\n");
  command.replace("\r", "\n");
  this->Interpreter->RunStringWithConsoleLocals(command.toLocal8Bit().data());

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
    bool isMultilineStatement = this->Interpreter->Push(line.toLocal8Bit().data());
    this->Prompt = isMultilineStatement ? pqPythonShell::PS2() : pqPythonShell::PS1();
  }
  this->Internals->end();
  this->prompt();
  CLEAR_UNDO_STACK();
}

//-----------------------------------------------------------------------------
void* pqPythonShell::consoleLocals()
{
  return this->Interpreter->GetInteractiveConsoleLocalsPyObject();
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
      vtkStdString* strData = reinterpret_cast<vtkStdString*>(calldata);
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
        Ui::PythonShell& ui = this->Internals->Ui;
        if (ui.stackedWidget->currentWidget() != ui.consoleWidget)
        {
          // if we haven't set up the interpret, let's do it first.
          this->setupInterpreter();
        }
        QByteArray code = file.readAll();
        this->executeScript(code.data());
      }
      else
      {
        qCritical() << "Error opening '" << filename << "'";
      }
    }
  }
}
