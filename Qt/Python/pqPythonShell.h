/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqPythonShell_h
#define _pqPythonShell_h

#include "QtPythonExport.h"
#include <QWidget>

/// Provides an interactive "shell" interface to an embedded Python interpreter
class QTPYTHON_EXPORT pqPythonShell :
  public QWidget
{
  Q_OBJECT
  
public:
  pqPythonShell(QWidget* Parent);
  ~pqPythonShell();

private slots:
  void printStdout(const QString&);
  void printStderr(const QString&);
  void onExecuteCommand(const QString&);

private:
  pqPythonShell(const pqPythonShell&);
  pqPythonShell& operator=(const pqPythonShell&);

  void promptForInput();

  struct pqImplementation;
  pqImplementation* const Implementation;
};

#endif // !_pqPythonShell_h

