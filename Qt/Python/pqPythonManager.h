/*=========================================================================

   Program: ParaView
   Module:    pqPythonManager.h

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

========================================================================*/
#ifndef pqPythonManager_h
#define pqPythonManager_h

#include "pqPythonModule.h"
#include <QObject>

class QWidget;
class QToolBar;
class pqServer;
class pqPythonDialog;

/**
* pqPythonManager is a class to facilitate the use of a python interpreter
* by various paraview GUI components.  The manager has a single instance
* of the pqPythonDialog.  Currently the pqPythonDialog "owns" the
* python interpreter.  Anyone who wants to execute python code should call
* pythonShellDialog() to get a pointer to the pqPythonDialog instance.  This
* manager class provides global access to the python dialog and methods to
* ensure the python dialog's interpreter stays in sync with the current active
* server.
*
* Note: because the interpreter is initialized lazily, a number of the member
* functions on this class have the side effect of initializing the python
* interpreter first.
*/
class PQPYTHON_EXPORT pqPythonManager : public QObject
{
  Q_OBJECT

public:
  pqPythonManager(QObject* parent = NULL);
  virtual ~pqPythonManager();

  /**
   * Returns true if the interpreter has been initialized.
   */
  bool interpreterIsInitialized();

  /**
   * Return the python shell dialog.  This will cause the interpreter to be initialized
   * if it has not been already.
   */
  pqPythonDialog* pythonShellDialog();

  //@{
  /**
   * Add a widget to be given macro actions.  QActions representing script macros
   * will be added to the widget.  This could be a QToolBar, QMenu, or other type
   * of widget.
   */
  void addWidgetForRunMacros(QWidget* widget);
  void addWidgetForEditMacros(QWidget* widget);
  void addWidgetForDeleteMacros(QWidget* widget);
  //@}

  /**
   * Show the python editor with the trace in it.
   * If txt is empty, the editor will obtain the state from active vtkSMTrace
   * instance, if any.
   */
  void editTrace(const QString& txt = QString(), bool update = false);

  /**
   * Save the macro in ParaView configuration and update widget automatically
   */
  void addMacro(const QString& fileName);

  /**
   * Invalidate the macro list, so the menu/toolbars are updated according to
   * the content of the Macros directories...
   */
  void updateMacroList();

signals:
  void paraviewPythonModulesImported();

public slots:
  /**
   * Executes the given script.  If the python interpreter hasn't been initialized
   * yet it will be initialized.
   */
  void executeScript(const QString& filename);

  /**
   * Same as `executeScript()` except that is also triggers a render on all
   * views in the application after the script has been processed. This is used
   * when playing back macros, for example.
   */
  void executeScriptAndRender(const QString& filename);

  /**
   * Launch python editor to edit the macro
   */
  void editMacro(const QString& fileName);

  /**
   * Print on the status bar "Python Trace is currently ON" if currently tracing...
   */
  void updateStatusMessage();

protected slots:
  /**
   * Whenever we are about to disconnect from a server, we "reset" the Python
   * shell, if created. This will ensure all Python objects created by the shell
   * are released.
   */
  void onRemovingServer(pqServer* server);

protected:
  QString getTraceString();

private:
  class pqInternal;
  pqInternal* Internal;
};
#endif
