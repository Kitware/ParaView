/*=========================================================================

   Program: ParaView
   Module:    pqPythonDialog.h

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

#ifndef _pqPythonDialog_h
#define _pqPythonDialog_h

#include "pqPythonModule.h"
#include <QDialog>

/**
  Qt dialog that embeds an instance of pqPythonShell, providing the user
  with an interactive Python console where they can enter Python commands
  manually and see the corresponding output.

  \sa pqPythonShell, pqConsoleWidget
*/
class pqPythonShell;
class QCloseEvent;
class QSplitter;
class PQPYTHON_EXPORT pqPythonDialog : public QDialog
{
  Q_OBJECT

public:
  typedef QDialog Superclass;
  pqPythonDialog(QWidget* Parent = 0);
  ~pqPythonDialog();

public slots:
  /**
  * Execute a commond in the python shell.
  */
  void runString(const QString& script);

  /**
  * Simply prints some text onto the shell. Note that this does not treat it
  * as a python script and hence doesn't execute it.
  */
  void print(const QString& msg);

  /**
  * Treats each string in the given stringlist as a filename and tries to
  * execute the file as a python script.
  */
  void runScript(const QStringList&);

  /**
  * Return a pointer to the pqPythonShell widget used by this dialog.
  */
  pqPythonShell* shell();

protected:
  /**
  * Overloaded to save window geometry on close events.
  */
  void closeEvent(QCloseEvent* event);

private slots:
  void runScript();
  void clearConsole();

private:
  pqPythonDialog(const pqPythonDialog&);
  pqPythonDialog& operator=(const pqPythonDialog&);

  struct pqImplementation;
  pqImplementation* const Implementation;
};

#endif // !_pqPythonDialog_h
