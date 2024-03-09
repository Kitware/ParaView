// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqIconSettings_h
#define pqIconSettings_h

#include "pqCoreModule.h"

#include <QMap>
#include <QString>

/**
 * pqIconSettings is a class to handle icons settings for a specific category of
 * graphical item.
 *
 * It can be used for instance to store icon associated to macros.
 */
class PQCORE_EXPORT pqIconSettings
{
public:
  pqIconSettings(const QString& iconCategory);

  /**
   * Fill settings with item key and icon paths.
   */
  void setItemIconInSettings(const QString& itemKey, const QString& iconPath);

  /**
   * Return icon path associated to given key in the settings.
   */
  QString getIconFromSettings(const QString& itemKey);

  /**
   * Get a map of {itemkey: iconPath} from the current settings
   * for the current category.
   */
  QMap<QString, QString> getSettings();

  /**
   * Write the given map to the settings.
   * Map is expected to be at format {itemkey: iconPath}
   * Previous settings are erased for current category.
   */
  void writeSettings(const QMap<QString, QString>& iconMap);

  /**
   * Remove the given item and its associated icon from the settings.
   */
  void removeItemFromSettings(const QString& itemKey);

private:
  /**
   * Get the settings index of the given item.
   * Return true if index exists.
   */
  bool getItemIndexInSettings(const QString& itemKey, int& idx, int& size);

  QString IconCategory;
};

#endif // pqIconSettings_h
