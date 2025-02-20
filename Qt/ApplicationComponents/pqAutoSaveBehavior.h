// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqAutoSaveBehavior_h
#define pqAutoSaveBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>

#include <QDir>
#include <QString>

#include "pqUndoStack.h"
#include "vtkSMSessionProxyManager.h"

/**
 * @ingroup Behaviors
 * @brief: pqAutoSaveBehavior save states automatically in the background.
 *
 * On every significant change, as a pipeline update or a rendering,
 * it writes a new statefile and backup the previous one.
 * The target directory can be defined in the settings.
 * By default, the standard platform-dependent AppData directory is used.
 * See pqCoreUtilities::getParaViewApplicationDataDirectory().
 *
 * The file format can be chosen with the `AutoSaveStateFormat` setting.
 *
 * The features can be (de)activated through the `AutoSave` setting.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqAutoSaveBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqAutoSaveBehavior(QObject* parent = nullptr);
  ~pqAutoSaveBehavior() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Saves the state.
   * First make a copy of the previous state as a `.bak` file.
   *
   * Uses format and directory from settings.
   * Default to pvsm and pqCoreUtilities::getParaViewApplicationDataDirectory()
   */
  static void saveState();

  /**
   * Return the directory where to write the state.
   * Uses user-defined directory (from settings) or standard one if empty.
   * See pqCoreUtilities::getParaViewApplicationDataDirectory()
   */
  static QDir getStateDirectory();

  /**
   * Return true if the AutoSave settings is enabled.
   */
  static bool autoSaveSettingEnabled();

  /**
   * Set the AutoSave Setting to "enable".
   */
  static void setAutoSaveSetting(bool enable);

  /**
   * Return the file format for the statefile, as defined in the settings.
   * Default to StateFormat::PVSM.
   */
  static pqApplicationCore::StateFileFormat getStateFormat();

private Q_SLOTS:
  /**
   * Enable or disable the connections depending on the AutoSave setting.
   * When the settings is on, observe the UndoStack and the active view.
   */
  void updateConnections();

  void clearConnections();

private: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Return the path for the given state file.
   *
   * This path is under getStateDirectory and uses getStateExtension.
   * If `bak` is true, the `.bak` variant is returned.
   *
   * File may not exists.
   */
  static QString getStatePath(bool bak);

  /**
   * Returns the path for the last saved state.
   * File may not exists.
   */
  static QString getLastStatePath();

  /**
   * Returns the path for the `bak` state.
   * File may not exists.
   */
  static QString getBakStatePath();

  Q_DISABLE_COPY(pqAutoSaveBehavior)

  QPointer<pqUndoStack> ObservedStack;
  bool HasChanges = false;
};

#endif
