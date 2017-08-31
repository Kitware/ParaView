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
#include "vtkSetGet.h" // for VTK_LEGACY
#include <QDialog>

/**
  Qt dialog that embeds an instance of pqPythonShell, providing the user
  with an interactive Python console where they can enter Python commands
  manually and see the corresponding output.

  @deprecated ParaView 5.5. Please use pqPythonShell directly and house it in
  a QDialog if needed.
*/
#if !defined(VTK_LEGACY_REMOVE)
class pqPythonShell;
class QCloseEvent;
class QSplitter;
class PQPYTHON_EXPORT pqPythonDialog : public QDialog
{
  Q_OBJECT

public:
  typedef QDialog Superclass;
  VTK_LEGACY(pqPythonDialog(QWidget* Parent = 0));
  VTK_LEGACY(~pqPythonDialog() override);

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
  void closeEvent(QCloseEvent* event) override;

private slots:
  void runScript();
  void clearConsole();

private:
  pqPythonDialog(const pqPythonDialog&);
  pqPythonDialog& operator=(const pqPythonDialog&);

  struct pqImplementation;
  pqImplementation* const Implementation;
};

#endif // !defined(VTK_LEGACY_REMOVE)
#endif // !_pqPythonDialog_h
