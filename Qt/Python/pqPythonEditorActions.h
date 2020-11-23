/*=========================================================================

   Program: ParaView
   Module:    pqPythonEditorActions.h

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

#ifndef pqPythonActionsConnector_h
#define pqPythonActionsConnector_h

#include "pqPythonUtils.h"

#include <QAction>

#include <array>
#include <cstddef>

/**
 * @struct PythonEditorActions
 * @brief Structs that contains the list of actions
 * available to the \ref PythonScriptEditor.
 *
 * @details Any actions related to the editor itself
 * (ie the text area) should be added here. The main
 * widget class pqPythonScriptEditor is responsible
 * for reconnecting the actions when a tab change occurs.
 */
struct pqPythonEditorActions
{
  pqPythonEditorActions();

  enum class Action : std::uint8_t
  {
    // File IO Actions
    NewFile,
    OpenFile,
    SaveFile,
    SaveFileAs,
    SaveFileAsMacro,

    // Undo/Redo Actions
    Undo,
    Redo,

    // Text Actions
    Copy,
    Cut,
    Paste,

    // Editor Actions
    CloseCurrentTab,
    Exit,

    END
  };

  EnumArray<Action, QAction> Actions;

  const QAction& operator[](const Action action) const { return Actions[action]; }

  QAction& operator[](const Action action) { return Actions[action]; }

  /**
   * @brief Connects the \ref PythonEditorActions
   * to the type T.
   */
  template <class T>
  static void connect(pqPythonEditorActions&, T*);

  /**
   * @brief Passthrough function
   */
  template <typename T>
  static void connect(pqPythonEditorActions& pea, QtStackPointer<T>& t)
  {
    pqPythonEditorActions::connect(pea, t.operator T*());
  }

  /**
   * @brief Disconnects the \ref PythonEditorActions
   * to the type T.
   */
  template <class T>
  static void disconnect(pqPythonEditorActions&, T*);

  /**
   * @brief Passthrough function
   */
  template <typename T>
  static void disconnect(pqPythonEditorActions& pea, QtStackPointer<T>& t)
  {
    pqPythonEditorActions::disconnect(pea, t.operator T*());
  }
};

#endif // pqPythonActionsConnector_h
