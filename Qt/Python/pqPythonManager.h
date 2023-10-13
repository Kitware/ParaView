// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPythonManager_h
#define pqPythonManager_h

#include "pqPythonModule.h" // for exports
#include "vtkType.h"        // for vtkTypeUInt32

#include <QObject>
#include <QVector>

class QWidget;
class QToolBar;
class pqPythonMacroSupervisor;
class pqServer;

/**
 * pqPythonManager is a class to facilitate the use of a python interpreter
 * by various paraview GUI components.
 *
 * @section Roadmap Roadmap
 *
 * pqPythonManager is slated for deprecation. It's unclear there's a need for
 * such a manager anymore since Python interpreter is globally accessible via
 * vtkPythonInterpreter.
 */
class PQPYTHON_EXPORT pqPythonManager : public QObject
{
  Q_OBJECT

public:
  pqPythonManager(QObject* parent = nullptr);
  ~pqPythonManager() override;

  /**
   * Provides access to the macro supervisor.
   */
  pqPythonMacroSupervisor* macroSupervisor() const;

  /**
   * Convienience method to call `vtkPythonInterpreter::Initialize()`.
   */
  bool initializeInterpreter();

  /**
   * Returns true if the interpreter has been initialized.
   * Same as calling `vtkPythonInterpreter::IsInitialized()`.
   */
  bool interpreterIsInitialized();

  ///@{
  /**
   * Add a widget to be given macro actions.  QActions representing script macros
   * will be added to the widget.  This could be a QToolBar, QMenu, or other type
   * of widget.
   */
  void addWidgetForRunMacros(QWidget* widget);
  void addWidgetForEditMacros(QWidget* widget);
  void addWidgetForDeleteMacros(QWidget* widget);
  ///@}

  /**
   * Save the macro in ParaView configuration and update widget automatically
   */
  void addMacro(const QString& fileName, vtkTypeUInt32 location = 0x10 /*vtkPVSession::CLIENT*/);

  /**
   * Invalidate the macro list, so the menu/toolbars are updated according to
   * the content of the Macros directories...
   */
  void updateMacroList();

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)

  /**
   * @brief Executes the given code.  If the python interpreter hasn't been initialized
   * yet it will be initialized.
   * @param code: lines of code to execute
   * @param pre_push: instructions to execute before the code execution
   * @param post_push: instructions to execute after the code execution
   */
  void executeCode(const QByteArray& code, const QVector<QByteArray>& pre_push = {},
    const QVector<QByteArray>& post_push = {});

  /**
   * Executes the given script.  If the python interpreter hasn't been initialized
   * yet it will be initialized.
   */
  void executeScript(
    const QString& filename, vtkTypeUInt32 location = 0x10 /*vtkPVSession::CLIENT*/);

  /**
   * Same as `executeScript()` except that is also triggers a render on all
   * views in the application after the script has been processed. This is used
   * when playing back macros, for example.
   */
  void executeScriptAndRender(
    const QString& filename, vtkTypeUInt32 location = 0x10 /*vtkPVSession::CLIENT*/);

  /**
   * Launch python editor to edit the macro
   */
  void editMacro(const QString& fileName);

private:
  struct pqInternal;
  pqInternal* Internal;
};
#endif
