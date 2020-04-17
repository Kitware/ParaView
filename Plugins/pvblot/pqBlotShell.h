// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqBlotShell.h

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

#ifndef pqBlotWidget_h
#define pqBlotWidget_h

#include <QWidget>

class pqConsoleWidget;
class pqServer;

class vtkObject;
class vtkEventQtSlotConnect;
class vtkPythonInteractiveInterpreter;

/**
   Qt widget that provides an interactive "shell" interface to a PV Blot
   interpreter.
*/

class pqBlotShell : public QWidget
{
  Q_OBJECT;

public:
  pqBlotShell(QWidget* p);
  virtual ~pqBlotShell();

  // Description:
  // Get/set the server to direct commands.  This only has an effect on the
  // next time initialize is called.
  virtual pqServer* activeServer() const { return this->ActiveServer; }
  virtual void setActiveServer(pqServer* server) { this->ActiveServer = server; }

Q_SIGNALS:
  /// Emitted whenever this widget starts or stops executing something.  The
  /// single argument is true when execution starts, false when it stops.
  void executing(bool);

public Q_SLOTS:
  virtual void initialize();
  virtual void initialize(const QString& filename);
  virtual void executePythonCommand(const QString& command);
  virtual void executeBlotCommand(const QString& command);
  virtual void echoExecuteBlotCommand(const QString& command);

  // Description:
  // Takes a filename of a blot script and executes it.
  virtual void executeBlotScript(const QString& filename);

  virtual void printStderr(const QString& text);
  virtual void printStdout(const QString& text);
  virtual void printMessage(const QString& text);

protected:
  pqConsoleWidget* Console;

  QString FileName;
  pqServer* ActiveServer;

  vtkEventQtSlotConnect* VTKConnect;
  // Interpreter is spelled wrong.  Maybe someone should fix that.
  vtkPythonInteractiveInterpreter* Interpretor;

  virtual void destroyInterpretor();

  virtual void promptForInput();

protected Q_SLOTS:
  virtual void printStderr(vtkObject*, unsigned long, void*, void*);
  virtual void printStdout(vtkObject*, unsigned long, void*, void*);

private:
  Q_DISABLE_COPY(pqBlotShell)
};

#endif // pqBlotWidget_h
