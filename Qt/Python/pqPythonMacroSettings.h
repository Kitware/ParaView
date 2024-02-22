// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqPythonMacroSettings_h
#define pqPythonMacroSettings_h

#include "pqPythonModule.h" // for exports

#include <QMap>
#include <QString>

/**
 * pqPythonMacroSettings is a class to handle macro settings like names and tooltips.
 *
 */
class PQPYTHON_EXPORT pqPythonMacroSettings
{
public:
  enum class MacroItemCategories
  {
    ToolTip,
    Name,
    Unused
  };

  explicit pqPythonMacroSettings(MacroItemCategories currentCategory);

  /**
   * Fill setting for macroPath with value under the current category.
   */
  void setItemInSettings(const QString& macroPath, const QString& value);

  /**
   * Return value for macroPath from the settings under the current category.
   */
  QString getItemFromSettings(const QString& macroPath);

  /**
   * Get a map of {macroPath: value} from the given settings
   * under the current category.
   */
  QMap<QString, QString> getItemSettings();

  /**
   * Write the given map to the settings under the current category.
   * Map is expected to be at format {macroPath: value}
   * Previous settings are erased under the current category.
   */
  void writeItemSettings(const QMap<QString, QString>& settingsMap);

  /**
   * Remove the given macroPath and its associated value from the settings under the current
   * category.
   */
  void removeItemFromSettings(const QString& macroPath);

private:
  /**
   * Get the index of the given macroPath from settings under the current category.
   * Return true if index exists.
   */
  bool getItemIndexInSettings(const QString& macroPath, int& idx, int& size);

  MacroItemCategories CurrentCategory = MacroItemCategories::Unused;
};

#endif // pqPythonMacroSettings_h
