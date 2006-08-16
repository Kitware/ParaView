/*=========================================================================

   Program: ParaView
   Module:    pqPythonShell.h

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

#ifndef _pqPythonShell_h
#define _pqPythonShell_h

#include "QtPythonExport.h"
#include <QWidget>

/**
  Qt widget that provides an interactive "shell" interface to an embedded Python interpreter.
  You can put an instance of pqPythonShell in a dialog or a window, and the user will be able
  to enter Python commands and see their output, while the UI is still responsive.
  
  \sa pqConsoleWidget, pqPythonDialog
*/  
  
class QTPYTHON_EXPORT pqPythonShell :
  public QWidget
{
  Q_OBJECT
  
public:
  pqPythonShell(QWidget* Parent);
  ~pqPythonShell();

  void InitializeInterpretor(int argc, char* argv[]);
public slots:
  void clear();
  void executeScript(const QString&);

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

