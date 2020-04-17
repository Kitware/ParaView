// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqBlotShell.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include <vtkPython.h> // python first

#include "pqBlotShell.h"

#include "pqConsoleWidget.h"
#include "pqServer.h"

#include <QCoreApplication>
#include <QDir>
#include <QVBoxLayout>

#include "vtkEventQtSlotConnect.h"
#include "vtkPVOptions.h"
#include "vtkPVPythonInterpretor.h"
#include "vtkProcessModule.h"

//=============================================================================
pqBlotShell::pqBlotShell(QWidget* p)
  : QWidget(p)
{
  QVBoxLayout* boxLayout = new QVBoxLayout(this);
  boxLayout->setMargin(0);

  this->Console = new pqConsoleWidget(this);
  boxLayout->addWidget(this->Console);

  this->setObjectName("pvblotShell");

  QObject::connect(this->Console, SIGNAL(executeCommand(const QString&)), this,
    SLOT(executeBlotCommand(const QString&)));

  this->ActiveServer = NULL;
  this->VTKConnect = vtkEventQtSlotConnect::New();
  this->Interpretor = NULL;
}

pqBlotShell::~pqBlotShell()
{
  this->VTKConnect->Disconnect();
  this->VTKConnect->Delete();
  this->destroyInterpretor();
}

//-----------------------------------------------------------------------------
void pqBlotShell::initialize(const QString& filename)
{
  this->FileName = QDir::fromNativeSeparators(filename);
  this->initialize();
}

void pqBlotShell::initialize()
{
  if (this->FileName.isEmpty())
  {
    qWarning("Need to initialize PV Blot with a filename.");
    return;
  }

  this->destroyInterpretor();

  this->Interpretor = vtkPVPythonInterpretor::New();
  this->Interpretor->SetCaptureStreams(true);
  this->VTKConnect->Connect(this->Interpretor, vtkCommand::ErrorEvent, this,
    SLOT(printStderr(vtkObject*, unsigned long, void*, void*)));
  this->VTKConnect->Connect(this->Interpretor, vtkCommand::WarningEvent, this,
    SLOT(printStdout(vtkObject*, unsigned long, void*, void*)));

  char* argv0 = const_cast<char*>(vtkProcessModule::GetProcessModule()->GetOptions()->GetArgv0());
  this->Interpretor->InitializeSubInterpretor(1, &argv0);

  this->executePythonCommand("import paraview\n");
  this->executePythonCommand("paraview.compatibility.major = 3\n");
  this->executePythonCommand("paraview.compatibility.minor = 5\n");
  this->executePythonCommand("from paraview import servermanager\n");
  this->executePythonCommand(
    QString("servermanager.ActiveConnection = servermanager.Connection(%1)\n")
      .arg(this->ActiveServer->GetConnectionID()));
  this->executePythonCommand(QString("servermanager.ActiveConnection.SetHost(\"%1\",0)\n")
                               .arg(this->ActiveServer->getResource().toURI()));
  this->executePythonCommand("servermanager.ToggleProgressPrinting()\n");
  this->executePythonCommand("servermanager.fromGUI = True\n");
  this->executePythonCommand("import paraview.simple\n");
  this->executePythonCommand(
    "paraview.simple.active_objects.view = servermanager.GetRenderView()\n");
  this->executePythonCommand("import pvblot\n");

  QString initcommand = "pvblot.initialize('" + this->FileName + "')\n";
  this->executePythonCommand(initcommand);

  this->promptForInput();
}

//-----------------------------------------------------------------------------
void pqBlotShell::destroyInterpretor()
{
  if (!this->Interpretor)
    return;

  this->executePythonCommand("pvblot.finalize()\n");

  QTextCharFormat format = this->Console->getFormat();
  format.setForeground(QColor(255, 0, 0));
  this->Console->setFormat(format);
  this->Console->printString("\n... restarting ...\n");
  format.setForeground(QColor(0, 0, 0));
  this->Console->setFormat(format);

  this->Interpretor->Delete();
  this->Interpretor = NULL;
}

//-----------------------------------------------------------------------------
void pqBlotShell::executePythonCommand(const QString& command)
{
  Q_EMIT this->executing(true);
  // this->printMessage(command);
  this->Interpretor->RunSimpleString(command.toLocal8Bit().data());
  Q_EMIT this->executing(false);
}

//-----------------------------------------------------------------------------
void pqBlotShell::executeBlotCommand(const QString& command)
{
  QString blotCommand = command;
  blotCommand.replace("'", "\\'");
  QString pythonCommand = QString("pvblot.execute('%1')\n").arg(blotCommand);
  this->executePythonCommand(pythonCommand);

  this->promptForInput();
}

//-----------------------------------------------------------------------------
void pqBlotShell::echoExecuteBlotCommand(const QString& command)
{
  QTextCharFormat format = this->Console->getFormat();
  format.setForeground(QColor(0, 0, 0));
  this->Console->setFormat(format);
  this->Console->printString(command);

  this->executeBlotCommand(command);
}

//-----------------------------------------------------------------------------
void pqBlotShell::executeBlotScript(const QString& filename)
{
  QString pythonCommand = QString("pvblot.execute_file('%1')\n").arg(filename);
  this->executePythonCommand(pythonCommand);

  this->promptForInput();
}

//-----------------------------------------------------------------------------
void pqBlotShell::printStderr(vtkObject*, unsigned long, void*, void* calldata)
{
  const char* text = reinterpret_cast<const char*>(calldata);
  this->printStderr(text);
  this->Interpretor->ClearMessages();
}

void pqBlotShell::printStdout(vtkObject*, unsigned long, void*, void* calldata)
{
  const char* text = reinterpret_cast<const char*>(calldata);
  this->printStdout(text);
  this->Interpretor->ClearMessages();
}

//-----------------------------------------------------------------------------
void pqBlotShell::printStderr(const QString& text)
{
  QTextCharFormat format = this->Console->getFormat();
  format.setForeground(QColor(255, 0, 0));
  this->Console->setFormat(format);
  this->Console->printString(text);

  QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void pqBlotShell::printStdout(const QString& text)
{
  QTextCharFormat format = this->Console->getFormat();
  format.setForeground(QColor(0, 150, 0));
  this->Console->setFormat(format);
  this->Console->printString(text);

  QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void pqBlotShell::printMessage(const QString& text)
{
  QTextCharFormat format = this->Console->getFormat();
  format.setForeground(QColor(0, 0, 150));
  this->Console->setFormat(format);
  this->Console->printString(text);

  QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

//-----------------------------------------------------------------------------
void pqBlotShell::promptForInput()
{
  QTextCharFormat format = this->Console->getFormat();
  format.setForeground(QColor(0, 0, 0));
  this->Console->setFormat(format);

  // Is there an easier way to get the value of pvblot.interpreter.prompt?
  this->Interpretor->MakeCurrent();
  PyObject* modules = PySys_GetObject(const_cast<char*>("modules"));
  PyObject* pvblotmodule = PyDict_GetItemString(modules, "pvblot");
  QString newPrompt = ">>> ";
  if (pvblotmodule)
  {
    PyObject* pvblotdict = PyModule_GetDict(pvblotmodule);
    if (pvblotdict)
    {
      PyObject* pvblotinterp = PyDict_GetItemString(pvblotdict, "interpreter");
      if (pvblotinterp)
      {
        PyObject* promptObj = PyObject_GetAttrString(pvblotinterp, const_cast<char*>("prompt"));
        newPrompt = PyString_AsString(PyObject_Str(promptObj));
      }
    }
  }

  this->Console->prompt(newPrompt);
  this->Interpretor->ReleaseControl();
}
