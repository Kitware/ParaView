// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPythonMacroSupervisor_h
#define pqPythonMacroSupervisor_h

#include "pqPythonModule.h"

#include <QList>
#include <QMap>
#include <QObject>
#include <QPointer>
#include <QString>

class QAction;

class PQPYTHON_EXPORT pqPythonMacroSupervisor : public QObject
{
  Q_OBJECT
public:
  pqPythonMacroSupervisor(QObject* p = nullptr);
  ~pqPythonMacroSupervisor() override;

  /**
   * Add a widget to be given macro actions.  QActions representing script macros
     will be added to the widget.  This could be a QToolBar, QMenu, or other type
     of widget.
  */
  void addWidgetForRunMacros(QWidget* widget);

  /**
   * Add a widget to be given macro actions.  QActions representing script macros
   * will be added to the widget.  This could be a QToolBar, QMenu, or other type
   * of widget.
   */
  void addWidgetForEditMacros(QWidget* widget);

  /**
   * Add a widget to be given macro actions.  QActions representing script macros
   * will be added to the widget.  This could be a QToolBar, QMenu, or other type
   * of widget.
   */
  void addWidgetForDeleteMacros(QWidget* widget);

  /**
   * Lookup and return a macro action by fileName (absolute path of macro file).
   * If it does not exist, return null.
   */
  QAction* getMacro(const QString& fileName);

  /**
   * Get macros from known macro directories (see getMacrosFilePaths)
   * In the returned map, the keys are fileNames and values are macro names.
   */
  static QMap<QString, QString> getStoredMacros();

  /**
   * Hide file by prepending a `.` to its name.
   * Hidden file in macro directory are not loaded.
   */
  static void hideFile(const QString& fileName);

  /**
   * Get a macro name from the fileName (absolute path of macro file).
   */
  static QString macroNameFromFileName(const QString& fileName);

  static void setNameForMacro(const QString& macroPath, const QString& name);

  /**
   * Get a macro tooltip from the fileName (absolute path of macro file).
   */
  static QString macroToolTipFromFileName(const QString& fileName);

  static void setTooltipForMacro(const QString& macroPath, const QString& name);

  /**
   * Get an icon path from the fileName (absolute path of macro file).
   * If no corresponding icon, return an empty string.
   */
  static QString iconPathFromFileName(const QString& fileName);

  static void setIconForMacro(const QString& macroPath, const QString& iconPath);

  /**
   * Get a list a "*.py" files from macro directories.
   */
  static QStringList getMacrosFilePaths();

Q_SIGNALS:

  /**
   * Emitted when a macro has been triggered.
   */
  void executeScriptRequested(const QString& fileName);

  /**
   * Emitted when a macro has been added.
   */
  void onAddedMacro();

  /**
   * Emitted when a macro has to be edited
   */
  void onEditMacro(const QString& fileName);

public Q_SLOTS:

  /**
   * Add an action with the given name and fileName.  If there is already
   * a macro with the given fileName it's macroname will be updated to the
   * one given. Macro names do not have to be unique.
   */
  void addMacro(const QString& macroName, const QString& tip, const QString& fileName);
  void addMacro(const QString& macroName, const QString& fileName);
  void addMacro(const QString& fileName);

  /**
   * Remove an action from the UI, with the given fileName (absolute path of
   * macro file). Note, this does not remove the macro from a future load,
   * you must call hideFile yourself (or manually remove the file from the
   * settings dir).
   */
  void removeMacro(const QString& fileName);

  /**
   * Update Macro list widgets and actions...
   */
  void updateMacroList();

protected Q_SLOTS:

  /**
   * If the sender is a QAction managed by this class, the fileName will be
   * looked up and the signal requestExecuteScript will be emitted.
   */
  void onMacroTriggered();

  /**
   * If the sender is a QAction managed by this class, the fileName will be
   * moved (deleted), and the macro will be removed
   */
  void onDeleteMacroTriggered();

  /**
   * If the sender is a QAction managed by this class, the macro file will be
   * open in a python edit
   */
  void onEditMacroTriggered();

protected: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Add a widget to be given macro actions.  QActions representing script macros
   * will be added to the widget.  This could be a QToolBar, QMenu, or other type
   * of widget.
   */
  void addWidgetForMacros(QWidget* widget, int actionType); // 0:run, 1:edit, 2:delete

  /**
   * Removes all actions and re-adds actions for each macro stored.
   */
  void resetActions();

private:
  class pqInternal;
  pqInternal* Internal;
};

#endif // ifndef pqPythonMacroSupervisor_h
