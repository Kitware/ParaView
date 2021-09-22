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
#include <QMenu>
#include <QPointer>

#include <array>
#include <cstddef>
#include <vector>

/**
 * @struct EditorActionGroup
 * @brief A template group of action
 * @details The template parameter
 * should be an enum ending with END
 * that contains at least one entry.
 */
template <typename E>
struct EditorActionGroup
{
  static_assert(static_cast<int>(E::END), "The input enum must end with END");
  static_assert(static_cast<int>(E::END) != 0, "At least one action is required");

  using Type = E;

  /**
   * @brief Default constructor allocates
   * all the actions listed in the enum
   */
  EditorActionGroup()
  {
    for (int i = 0; i < static_cast<int>(Type::END); ++i)
    {
      this->Actions[static_cast<Type>(i)] = new QAction();
    }
  }

  /**
   * @brief const accessor to an action
   */
  const QAction& operator[](const Type action) const { return *this->Actions[action]; }

  /**
   * @brief accessor to an action
   */
  QAction& operator[](const Type action) { return *this->Actions[action]; }

  /**
   * @brief The array of actions
   */
  EnumArray<Type, QPointer<QAction>> Actions;
};

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
  /**
   * @brief Default constructor initialize
   * the tooltip and text of the various
   * actions.
   */
  pqPythonEditorActions();

  /**
   * @brief General editor actions
   */
  enum class GeneralActionType : std::uint8_t
  {
    // File IO Actions
    NewFile,
    OpenFile,
    SaveFile,
    SaveFileAs,
    SaveFileAsMacro,
    SaveFileAsScript,

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

  /**
   * @brief Specialized actions
   * for the scripting part of the editor
   */
  enum class ScriptActionType : std::uint8_t
  {
    Open,
    Load,
    Delete,

    END
  };

  using TGeneralAction = EditorActionGroup<GeneralActionType>;

  /**
   * @brief The list of general actions
   */
  TGeneralAction GeneralActions;

  using ScriptAction = EditorActionGroup<ScriptActionType>;

  /**
   * @brief The variable sized array of script actions
   */
  std::vector<ScriptAction> ScriptActions;

  /**
   * @brief const accessor to an action
   */
  const QAction& operator[](const GeneralActionType action) const
  {
    return this->GeneralActions[action];
  }

  /**
   * @brief accessor to an action
   */
  QAction& operator[](const GeneralActionType action) { return this->GeneralActions[action]; }

  /**
   * @brief Updates the list of actions by listing
   * the files contained into the default Script dir
   */
  void updateScriptsList();

  /**
   * @brief Fill the input menus with the current actions
   * listed by this object
   */
  void FillQMenu(EnumArray<ScriptAction::Type, QMenu*> menus);

  /**
   * @brief Connects the \ref PythonEditorActions
   * to the type T.
   */
  template <class T>
  static void connect(pqPythonEditorActions&, T*);

  /**
   * @brief Disconnects the \ref PythonEditorActions
   * to the type T.
   */
  template <class T>
  static void disconnect(pqPythonEditorActions&, T*);
};

#endif // pqPythonActionsConnector_h
