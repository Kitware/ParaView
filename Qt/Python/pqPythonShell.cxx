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

#include <vtkPython.h> // python first

#include "QtPythonConfig.h"

#include "pqConsoleWidget.h"
#include "pqPythonShell.h"
#include "vtkCommand.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkPVPythonInteractiveInterpretor.h"
#include "vtkStdString.h"

#include <QCoreApplication>
#include <QResizeEvent>
#include <QScrollBar>
#include <QStringListModel>
#include <QTextCharFormat>
#include <QVBoxLayout>


class pqPythonShellCompleter : public pqConsoleWidgetCompleter
{
public:
  pqPythonShellCompleter(pqPythonShell& p) : Parent(p)
    {
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
    for (int i = completion.length()-1; i >= 0; --i)
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
      compareText = compareText.mid(dot+1);
      }

    // Lookup python names
    QStringList attrs;
    if (!lookup.isEmpty() || !compareText.isEmpty())
      {
      attrs = Parent.getPythonAttributes(lookup);
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
  pqPythonShell& Parent;
};


/////////////////////////////////////////////////////////////////////////
// pqPythonShell::pqImplementation

struct pqPythonShell::pqImplementation
{
  pqImplementation(QWidget* Parent) 
    : Console(Parent), Interpreter(0)
  {
    this->VTKConnect = vtkEventQtSlotConnect::New();
  }

  void initialize(int argc, char* argv[])
  {
    this->destroyInterpretor();
    this->Interpreter = vtkPVPythonInteractiveInterpretor::New();
    this->Interpreter->SetCaptureStreams(true);
    this->Interpreter->InitializeSubInterpretor(argc, argv);
    this->Interpreter->MakeCurrent();
    
    // Setup Python's interactive prompts
    PyObject* ps1 = PySys_GetObject(const_cast<char*>("ps1"));
    if(!ps1)
      {
      PySys_SetObject(const_cast<char*>("ps1"), ps1 = PyString_FromString(">>> "));
      Py_XDECREF(ps1);
      }

    PyObject* ps2 = PySys_GetObject(const_cast<char*>("ps2"));
    if(!ps2)
      {
      PySys_SetObject(const_cast<char*>("ps2"), ps2 = PyString_FromString("... "));
      Py_XDECREF(ps2);
      }
    this->Interpreter->ReleaseControl();
    this->MultilineStatement = false;
  }

  ~pqImplementation()
  {
    this->VTKConnect->Disconnect();
    this->VTKConnect->Delete();
    this->destroyInterpretor();
  }

  void destroyInterpretor()
    {
    if (this->Interpreter)
      {
      QTextCharFormat format = this->Console.getFormat();
      format.setForeground(QColor(255, 0, 0));
      this->Console.setFormat(format);
      this->Console.printString("\n... restarting ...\n");
      format.setForeground(QColor(0, 0, 0));
      this->Console.setFormat(format);

      this->Interpreter->MakeCurrent();

      // Restore Python's original stdout and stderr
      PySys_SetObject(const_cast<char*>("stdout"), PySys_GetObject(const_cast<char*>("__stdout__")));
      PySys_SetObject(const_cast<char*>("stderr"), PySys_GetObject(const_cast<char*>("__stderr__")));
      this->Interpreter->ReleaseControl();
      this->Interpreter->Delete();
      }
    this->Interpreter = 0;
    }

  void executeCommand(const QString& Command)
  {
    this->MultilineStatement = 
      this->Interpreter->Push(Command.toAscii().data());
  }

  void promptForInput(const QString& indent=QString())
  {
    QTextCharFormat format = this->Console.getFormat();
    format.setForeground(QColor(0, 0, 0));
    this->Console.setFormat(format);

    this->Interpreter->MakeCurrent();
    if(!this->MultilineStatement)
      {
      this->Console.prompt(
        PyString_AsString(PySys_GetObject(const_cast<char*>("ps1"))));
      }
    else
      {
      this->Console.prompt(
        PyString_AsString(PySys_GetObject(const_cast<char*>("ps2"))));
      }
    this->Console.printCommand(indent);
    this->Interpreter->ReleaseControl();
  }

  /// Provides a console for gathering user input and displaying 
  /// Python output
  pqConsoleWidget Console;

  /// Indicates if the last statement processes was incomplete.
  bool MultilineStatement;

  /// Separate Python interpreter that will be used for this shell
  vtkPVPythonInteractiveInterpretor* Interpreter;

  vtkEventQtSlotConnect *VTKConnect;
};

/////////////////////////////////////////////////////////////////////////
// pqPythonShell

pqPythonShell::pqPythonShell(QWidget* Parent) :
  QWidget(Parent),
  Implementation(new pqImplementation(this))
{
  QVBoxLayout* const boxLayout = new QVBoxLayout(this);
  boxLayout->setMargin(0);
  boxLayout->addWidget(&this->Implementation->Console);

  this->setObjectName("pythonShell");

  pqPythonShellCompleter* completer = new pqPythonShellCompleter(*this);
  this->Implementation->Console.setCompleter(completer);
  
  QObject::connect(
    &this->Implementation->Console, SIGNAL(executeCommand(const QString&)), 
    this, SLOT(onExecuteCommand(const QString&)));
}

pqPythonShell::~pqPythonShell()
{
  delete this->Implementation;
}

void pqPythonShell::initializeInterpretor()
{
  const char* argv0 = vtkProcessModule::GetProcessModule()->
    GetOptions()->GetArgv0();
  this->initializeInterpretor(1, (char**)&argv0);
}

void pqPythonShell::initializeInterpretor(int argc, char* argv[])
{
  this->Implementation->VTKConnect->Disconnect();

  this->Implementation->initialize(argc, argv);
  QTextCharFormat format = this->Implementation->Console.getFormat();
  format.setForeground(QColor(0, 0, 255));
  this->Implementation->Console.setFormat(format);
  this->Implementation->Console.printString(
    QString("Python %1 on %2\n").arg(Py_GetVersion()).arg(Py_GetPlatform()));
  this->promptForInput();

  this->Implementation->VTKConnect->Connect(
    this->Implementation->Interpreter, vtkCommand::ErrorEvent, 
    this, SLOT(printStderr(vtkObject*, unsigned long, void*, void*))); 
  this->Implementation->VTKConnect->Connect(
    this->Implementation->Interpreter, vtkCommand::WarningEvent, 
    this, SLOT(printStdout(vtkObject*, unsigned long, void*, void*))); 

  this->Implementation->VTKConnect->Connect(
    this->Implementation->Interpreter, vtkCommand::UpdateEvent, 
    this, SLOT(readInputLine(vtkObject*, unsigned long, void*, void*))); 
}

void pqPythonShell::clear()
{
  this->Implementation->Console.clear();
  this->Implementation->promptForInput();
}

void pqPythonShell::makeCurrent()
{
  this->Implementation->Interpreter->MakeCurrent();
}
void pqPythonShell::releaseControl()
{
  this->Implementation->Interpreter->ReleaseControl();
}

void pqPythonShell::executeInitFromGUI()
{
  this->Implementation->Interpreter->ExecuteInitFromGUI();
}

void pqPythonShell::executeScript(const QString& script)
{
  this->printStdout("\n");
  emit this->executing(true);  
  this->Implementation->Interpreter->RunSimpleString(
    script.toAscii().data());
  emit this->executing(false);
  this->Implementation->promptForInput();
}

QStringList pqPythonShell::getPythonAttributes(const QString& objectName)
{
  this->makeCurrent();

  PyObject* dict = PyImport_GetModuleDict();
  PyObject* object = PyDict_GetItemString(dict, "__main__");
  Py_INCREF(object);


  if (!objectName.isEmpty())
    {
    QStringList tmpNames = objectName.split('.');
    for (int i = 0; i < tmpNames.size() && object; ++i)
      {
      QByteArray tmpName = tmpNames.at(i).toLatin1();
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
    PyObject* keys = PyObject_Dir(object);
    if (keys)
      {
      PyObject* key;
      PyObject* value;
      QString keystr;
      int nKeys = PyList_Size(keys);
      for (int i = 0; i < nKeys; ++i)
        {
        key = PyList_GetItem(keys, i);
        value = PyObject_GetAttr(object, key);
        if (!value)
          {
          continue;
          }

        results << PyString_AsString(key);
        Py_DECREF(value);
        }
      Py_DECREF(keys);
      }
    Py_DECREF(object);
    }

  this->releaseControl();
  return results;
}

void pqPythonShell::printStdout(vtkObject*, unsigned long, void*, void* calldata)
{
  const char* text = reinterpret_cast<const char*>(calldata);
  this->printStdout(text);
  this->Implementation->Interpreter->ClearMessages();
}

#include <QDialog>
#include <QLineEdit>
#include <QVBoxLayout>
class LineInput : public QDialog {
public:
  LineInput(QWidget* parent=NULL) : QDialog(parent, Qt::FramelessWindowHint)
  {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setMargin(0);
    this->Edit = new QLineEdit;
    layout->addWidget(this->Edit);
    this->connect(this->Edit, SIGNAL(returnPressed()), SLOT(accept()));
  }
  QLineEdit* Edit;
};

void pqPythonShell::readInputLine(vtkObject*, unsigned long, void*, void* calldata)
{
  vtkStdString* ret = reinterpret_cast<vtkStdString*>(calldata);
  QPoint pos = this->Implementation->Console.getCursorPosition();
  LineInput input(this);
  input.move(this->mapToGlobal(pos));
  input.exec();
  *ret = input.Edit->text().toAscii().data();
}

void pqPythonShell::printStderr(vtkObject*, unsigned long, void*, void* calldata)
{
  const char* text = reinterpret_cast<const char*>(calldata);
  this->printStderr(text);
  this->Implementation->Interpreter->ClearMessages();
}

void pqPythonShell::printStdout(const QString& text)
{
  QTextCharFormat format = this->Implementation->Console.getFormat();
  format.setForeground(QColor(0, 150, 0));
  this->Implementation->Console.setFormat(format);
  
  this->Implementation->Console.printString(text);
  
  QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void pqPythonShell::printMessage(const QString& text)
{
  QTextCharFormat format = this->Implementation->Console.getFormat();
  format.setForeground(QColor(0, 0, 150));
  this->Implementation->Console.setFormat(format);
  
  this->Implementation->Console.printString(text);
}

void pqPythonShell::printStderr(const QString& text)
{
  QTextCharFormat format = this->Implementation->Console.getFormat();
  format.setForeground(QColor(255, 0, 0));
  this->Implementation->Console.setFormat(format);
  
  this->Implementation->Console.printString(text);
  
  QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void pqPythonShell::onExecuteCommand(const QString& Command)
{
  QString command = Command;
  command.replace(QRegExp("\\s*$"), "");
  this->internalExecuteCommand(command);

  // Find the indent for the command.
  QRegExp regExp("^(\\s+)");
  QString indent;
  if (regExp.indexIn(command) != -1)
    {
    indent = regExp.cap(1);
    }
  this->Implementation->promptForInput(indent);
}

void pqPythonShell::promptForInput()
{
  this->Implementation->promptForInput();
}

void pqPythonShell::internalExecuteCommand(const QString& command)
{
  emit this->executing(true);  
  this->Implementation->executeCommand(command);
  emit this->executing(false);
}
