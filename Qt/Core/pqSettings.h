// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSettings_h
#define pqSettings_h

#include "pqCoreModule.h"
#include <QSettings>

class QDialog;
class QMainWindow;
class QDockWidget;
class vtkSMProperty;

/**
 * pqSettings extends QSettings to add support for following:
 * \li saving/restoring window/dialog geometry.
 *
 * Note that pqApplicationCore::settings() configure differents things about
 * the settings files in used (name, site-settings path).
 * This should be prefered over creating a new instance of pqSettings manually.
 */
class PQCORE_EXPORT pqSettings : public QSettings
{
  Q_OBJECT
  typedef QSettings Superclass;

public:
  ///@{
  /**
   * Create a new instance of pqSettings.
   * Note that the differents arguments impact the actual file path/name used:
   * be sure to be consistent or you will end up with different files.
   *
   * If you want to access the standard ParaView application settings,
   * please use pqApplicationCore::settings() that already ensure the configuration.
   */
  pqSettings(
    const QString& organization, const QString& application = QString(), QObject* parent = nullptr);
  pqSettings(Scope scope, const QString& organization, const QString& application = QString(),
    QObject* parent = nullptr);
  pqSettings(Format format, Scope scope, const QString& organization,
    const QString& application = QString(), QObject* parent = nullptr);
  pqSettings(const QString& fileName, Format format, QObject* parent = nullptr);
  pqSettings(QObject* parent = nullptr);
  ~pqSettings() override;
  ///@}

  void saveState(const QMainWindow& window, const QString& key);
  void saveState(const QDialog& dialog, const QString& key);

  void restoreState(const QString& key, QMainWindow& window);
  void restoreState(const QString& key, QDialog& dialog);

  /**
   * Calling this method will cause the modified signal to be emitted.
   */
  void alertSettingsModified();

  /**
   * Save a property value to a given setting name
   */
  void saveInQSettings(const char* key, vtkSMProperty* smproperty);

  /**
   * Creates a new backup file for the current settings.
   * If `filename` is empty, then a backup file name will automatically be
   * picked. On success returns the backup file name, on failure an empty string
   * is returned.
   */
  QString backup(const QString& filename = QString());

private:
  /**
   * ensure that when window state is being loaded, if dock windows are
   * beyond the viewport, we correct them.
   */
  void sanityCheckDock(QDockWidget* dock_widget);
Q_SIGNALS:
  void modified();
};

#endif
