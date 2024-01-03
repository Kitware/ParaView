// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqPythonEditorActions_h
#define pqPythonEditorActions_h

#include "pqPythonUtils.h"

#include <QAction>
#include <QMenu>
#include <QPointer>

#include <array>
#include <cstddef>
#include <vector>

class pqPythonManager;

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
 * available to the \ref pqPythonScriptEditor.
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
    DeleteAll,

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
    Run,

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
    Run,

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
   * @param python_mgr: instance of the python manager
   */
  void updateScriptsList(pqPythonManager* python_mgr);

  /**
   * @brief Fill the input menus with the current actions
   * listed by this object
   */
  void FillQMenu(EnumArray<ScriptAction::Type, QMenu*> menus);

  /**
   * @brief Connects the \ref pqPythonEditorActions
   * to the type T.
   */
  template <class T>
  static void connect(pqPythonEditorActions&, T*);

  /**
   * @brief Disconnects the \ref pqPythonEditorActions
   * to the type T.
   */
  template <class T>
  static void disconnect(pqPythonEditorActions&, T*);
};

#endif // pqPythonEditorActions_h
