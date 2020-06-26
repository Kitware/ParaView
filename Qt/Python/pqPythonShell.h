/*=========================================================================

   Program: ParaView
   Module:    pqPythonShell.h

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
#ifndef _pqPythonShell_h
#define _pqPythonShell_h

#include "pqPythonModule.h" //  needed for PQPYTHON_EXPORT.
#include <QScopedPointer>   // needed for QScopedPointer.
#include <QWidget>

class vtkObject;
class pqConsoleWidget;

/**
* @class pqPythonShell
* @brief Widget for a Python shell.
*
* pqPythonShell is a QWidget subclass that provides an interactive Python
* shell. It uses vtkPythonInteractiveInterpreter to provide an interactive Python
* interpreter. Note that this is still executing Python code using the
* application wide global Python interpreter, it just keeps the context separate
* using an instance of `core.InteractiveConsole` internally.
*
* @section PythonInit Python initialization
*
* pqPythonShell does not initialize Python on creation. By default, it waits
* till the pqConsoleWidget gets focus or `pqPythonShell::executeScript` is
* called. One can also call `pqPythonShell::initialize` explicitly to
* initialize the interpreter.
*
* @sa pqConsoleWidget.
*/
class PQPYTHON_EXPORT pqPythonShell : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqPythonShell(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags{});
  ~pqPythonShell() override;

  /**
  * Returns the interactive console context (the locals() dict).
  * You can use static_cast<PythonObject*>() to convert the void pointer.
  * See vtkPythonInteractiveInterpreter::GetInteractiveConsoleLocalsPyObject().
  */
  void* consoleLocals();

  //@{
  /**
   * Set/get the font size in points for the Python shell text.
   */
  void setFontSize(int fontSize);
  //@}

  enum PrintMode
  {
    STATUS,
    OUTPUT,
    ERROR
  };

public Q_SLOTS:
  /**
  * Prints some text on the shell.
  */
  void printMessage(const QString&);

  /**
  * Clears the terminal. This does not change the state of the Python
  * interpreter, just clears the text shown in the Widget.
  */
  void clear();

  /**
  * Execute an arbitrary python script/string. This simply execute the Python
  * script in the global Python interpreter.
  */
  void executeScript(const QString&);

  /**
  * Resets the python interactive interpreter. This does not affect the global
  * Python interpreter. If the interactive interpreter hasn't been initialized
  * this has no effect.
  */
  void reset();

  /**
  * Returns true is the shell is currently executing a script/command.
  */
  bool isExecuting() const;

  /**
  * Use this method instead of calling pqConsoleWidget::printString()
  * directly. That helps us keep track of whether we need to show the prompt
  * or not.
  */
  void printString(const QString&, PrintMode mode = STATUS);

  /**
  * Set a list of statements to be run each time the interpreter is reset.
  *
  * By default, this imports the paraview.simple module.
  * If you call this method, be aware that the preamble is
  * assumed not to have any multi-line statements.
  */
  static void setPreamble(const QStringList& statements);
  static const QStringList& preamble();

  /**
   * Initialize the Python interpreter in the shell, if not already.
   * Has no effect if the interpreter has already been initialized.
   */
  void initialize();

Q_SIGNALS:
  /**
  * signal fired whenever the shell starts (starting=true) and finishes
  * (starting=false) executing a Python command/script. This can be used by
  * the UI to block user input while the script is executing.
  */
  void executing(bool starting);

protected Q_SLOTS:
  void pushScript(const QString&);
  void runScript();

protected:
  pqConsoleWidget* ConsoleWidget;
  const char* Prompt;
  static QStringList Preamble;

  static const char* PS1() { return ">>> "; }
  static const char* PS2() { return "... "; }

  /**
  * Show the user-input prompt, if needed. Returns true if the prompt was
  * re-rendered, otherwise false.
  */
  bool prompt(const QString& indent = QString());

  void HandleInterpreterEvents(vtkObject* caller, unsigned long eventid, void* calldata);

private:
  Q_DISABLE_COPY(pqPythonShell)
  bool Prompted;

  class pqInternals;
  friend class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif // !_pqPythonShell_h
