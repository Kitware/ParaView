/*=========================================================================

   Program: ParaView
   Module:    pqPythonShell.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#include "QtPythonConfig.h"

#include "pqConsoleWidget.h"
#include "pqPythonShell.h"
#include "pqPythonStream.h"

#include <pqPythonInterpreter.h>

#undef slots // Workaround for a conflict between Qt slots and the Python headers
#include <vtkPython.h>

#include <QDir>
#include <QResizeEvent>
#include <QTextCharFormat>
#include <QVBoxLayout>

/////////////////////////////////////////////////////////////////////////
// pqPythonShell::pqImplementation

struct pqPythonShell::pqImplementation
{
  pqImplementation(QWidget* Parent) :
    Console(Parent)
  {
    this->Interpreter.MakeCurrent();
    
    // Redirect Python's stdout and stderr
    PySys_SetObject(const_cast<char*>("stdout"), reinterpret_cast<PyObject*>(pqWrap(this->pythonStdout)));
    PySys_SetObject(const_cast<char*>("stderr"), reinterpret_cast<PyObject*>(pqWrap(this->pythonStderr)));

    /** \todo Add the path to the *installed* paraview module to Python's path, or do some sort of dynamic lookup */

    // For the convenience of developers, add the path to the server manager wrappers to Python's path
    if(PyObject* path = PySys_GetObject(const_cast<char*>("path")))
      {
      PyObject* const module_dir = PyString_FromString(QDir::convertSeparators(PV_PYTHON_MODULE_DIR).toAscii().data());
      PyList_Insert(path, 0, module_dir);
      Py_XDECREF(module_dir);
      }

    // For the convenience of developers, add the path to the paraview libraries to Python's path
    if(PyObject* path = PySys_GetObject(const_cast<char*>("path")))
      {
      PyObject* const module_dir = PyString_FromString(QDir::convertSeparators(PV_PYTHON_LIBRARY_DIR).toAscii().data());
      PyList_Insert(path, 1, module_dir);
      Py_XDECREF(module_dir);
      }
      
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
  }
  
  ~pqImplementation()
  {
    this->Interpreter.MakeCurrent();
    
    // Restore Python's original stdout and stderr
    PySys_SetObject(const_cast<char*>("stdout"), PySys_GetObject(const_cast<char*>("__stdout__")));
    PySys_SetObject(const_cast<char*>("stderr"), PySys_GetObject(const_cast<char*>("__stderr__")));
  }
  
  void executeCommand(const QString& Command)
  {
    this->Interpreter.MakeCurrent();
    
    PyRun_SimpleString(Command.toAscii().data());
  }

  void promptForInput()
  {
    this->Interpreter.MakeCurrent();
    
    QTextCharFormat format = this->Console.getFormat();
    format.setForeground(QColor(0, 0, 0));
    this->Console.setFormat(format);

    if(this->MultilineStatement.isEmpty())
      this->Console.printString(PyString_AsString(PySys_GetObject(const_cast<char*>("ps1"))));
    else
      this->Console.printString(PyString_AsString(PySys_GetObject(const_cast<char*>("ps2"))));
  }

  /// Provides a console for gathering user input and displaying Python output
  pqConsoleWidget Console;
  /// Iff the user is in the process of entering a multi-line statement, this will contain everything entered so-far
  QString MultilineStatement;
  /// Redirects Python's stdout stream
  pqPythonStream pythonStdout;
  /// Redirects Python's stderr stream
  pqPythonStream pythonStderr;
  /// Separate Python interpreter that will be used for this shell
  pqPythonInterpreter Interpreter;
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
  
  QObject::connect(&this->Implementation->pythonStdout, SIGNAL(streamWrite(const QString&)), this, SLOT(printStdout(const QString&)));
  QObject::connect(&this->Implementation->pythonStderr, SIGNAL(streamWrite(const QString&)), this, SLOT(printStderr(const QString&)));
  QObject::connect(&this->Implementation->Console, SIGNAL(executeCommand(const QString&)), this, SLOT(onExecuteCommand(const QString&)));

  this->Implementation->Console.printString(QString("Python %1 on %2\n").arg(Py_GetVersion()).arg(Py_GetPlatform()));
  this->promptForInput();
}

pqPythonShell::~pqPythonShell()
{
  delete this->Implementation;
}

void pqPythonShell::clear()
{
  this->Implementation->Console.clear();
  this->Implementation->promptForInput();
}

void pqPythonShell::printStdout(const QString& String)
{
  QTextCharFormat format = this->Implementation->Console.getFormat();
  format.setForeground(QColor(0, 0, 0));
  this->Implementation->Console.setFormat(format);
  
  this->Implementation->Console.printString(String);
}

void pqPythonShell::printStderr(const QString& String)
{
  QTextCharFormat format = this->Implementation->Console.getFormat();
  format.setForeground(QColor(255, 0, 0));
  this->Implementation->Console.setFormat(format);
  
  this->Implementation->Console.printString(String);
}

void pqPythonShell::onExecuteCommand(const QString& Command)
{
  QString command = Command;
  command.replace(QRegExp("\\s*$"), "");
  
  if(this->Implementation->MultilineStatement.isEmpty())
    {
    if(command.endsWith(":"))
      {
      this->Implementation->MultilineStatement.append(command);
      this->Implementation->MultilineStatement.append("\n");
      }
    else
      {
      this->Implementation->executeCommand(command);
      }
    }
  else
    {
    if(command.isEmpty())
      {
      this->Implementation->MultilineStatement.append("\n");
      this->Implementation->executeCommand(this->Implementation->MultilineStatement);
      this->Implementation->MultilineStatement.clear();
      }
    else
      {
      this->Implementation->MultilineStatement.append(command);
      this->Implementation->MultilineStatement.append("\n");
      }
    }
  
  this->promptForInput();
}

void pqPythonShell::promptForInput()
{
  this->Implementation->promptForInput();
}

