/*=========================================================================

   Program: ParaView
   Module:    pqPythonScriptEditor.h

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
#ifndef _pqPythonScriptEditor_h
#define _pqPythonScriptEditor_h

#include "pqPythonModule.h"

#include "pqCoreUtilities.h"
#include "pqPythonEditorActions.h"

#include <QMainWindow>

class QAction;
class QMenu;

class pqPythonManager;
class pqPythonTabWidget;

/**
 * @class pqPythonScriptEditor
 * @brief Paraview Python Editor widget
 * @details This widget can be used as a embedded Qt python editor
 * inside paraview. It provides functionality to read, write and edit
 * python script and paraview macros within paraview itself. The text editor
 * provides basic functionality such as undo/redo, line numbering and
 * syntax highlighting (through pygments).
 *
 * You can either use this widget as a sole editor, or get the paraview
 * static one from \ref GetUniqueInstance (which gives you the editor
 * used for the macro and the scripts).
 *
 * Note that \ref GetUniqueInstance is a lazy way of having a unique instance
 * of the editor ready to be used. A better approach would be to actually change
 * the code that uses this class to reflect that peculiar behavior (and not embed
 * it inside the class). Also note that you can freely instantiate as many editor
 * as you want as two instances of this class don't share any common data.
 *
 * \note This class handles the main window components. If you are interested only
 * in the text editor itself, please see \ref pqPythonTextArea.
 */
class PQPYTHON_EXPORT pqPythonScriptEditor : public QMainWindow
{
  Q_OBJECT

public:
  explicit pqPythonScriptEditor(QWidget* parent = nullptr);

  /**
   * @brief Sets the default save directory for the current buffer
   * @details Internally, it is only used for the QFileDialog
   * directory argument when saving or loading a file (ie the
   * folder from which the QFileDialog is launched).
   */
  void setSaveDialogDefaultDirectory(const QString& dir);

  /**
   * @brief Sets the \ref pqPythonManager used for the macros
   */
  void setPythonManager(pqPythonManager* manager);

  /**
   * @brief Scroll the editor to the bottom of the scroll area
   */
  void scrollToBottom();

  /**
   * @brief Open a file inside the editor
   */
  void open(const QString& filename);

  /**
   * @brief Updates the trace tab text
   * and creates a new one if it doesn't exists
   */
  void updateTrace(const QString& str);

  /**
   * @brief Wraps up the trace tab
   */
  void stopTrace(const QString& str);

  /**
   * @brief Utility function that provides a single instance
   * of the editor.
   */
  static pqPythonScriptEditor* getUniqueInstance()
  {
    static pqPythonScriptEditor* instance = new pqPythonScriptEditor(pqCoreUtilities::mainWidget());
    return instance;
  }

  /**
   * @brief Triggers an macro list update
   * if the PythonManager exists
   */
  static void updateMacroList();

protected:
  /**
   * @brief Override the QMainWindow closeEvent
   * @details We ask the user wants to save the current
   * file if it's not already saved
   */
  void closeEvent(QCloseEvent* event) override;

private:
  void createMenus();

  void createStatusBar();

  QMenu* fileMenu;
  QMenu* editMenu;
  QMenu* helpMenu;

  pqPythonTabWidget* TabWidget;

  pqPythonEditorActions Actions;

  /**
   * @brief The python manager only used for the
   * paraview macro system
   */
  pqPythonManager* pythonManager;
};

#endif
