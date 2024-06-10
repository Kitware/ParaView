// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqPythonMacroSettings.h"

#include "pqApplicationCore.h"
#include "pqSettings.h"

namespace
{
namespace details
{
QString SETTINGS_ITEM_KEY()
{
  return "item";
}

QString SETTINGS_GROUP(const pqPythonMacroSettings::MacroItemCategories& type)
{
  switch (type)
  {
    case pqPythonMacroSettings::MacroItemCategories::ToolTip:
      return "MacroToolTips";
    case pqPythonMacroSettings::MacroItemCategories::Name:
      return "MacroNames";
    case pqPythonMacroSettings::MacroItemCategories::Unused:
    default:
      return "unused";
  }
}

QString SETTINGS_VALUE_KEY(const pqPythonMacroSettings::MacroItemCategories& type)
{
  switch (type)
  {
    case pqPythonMacroSettings::MacroItemCategories::ToolTip:
      return "tooltip";
    case pqPythonMacroSettings::MacroItemCategories::Name:
      return "name";
    case pqPythonMacroSettings::MacroItemCategories::Unused:
    default:
      return "unused";
  }
}
}
}

//----------------------------------------------------------------------------
pqPythonMacroSettings::pqPythonMacroSettings(
  pqPythonMacroSettings::MacroItemCategories currentCategory)
  : CurrentCategory(currentCategory)
{
}

//----------------------------------------------------------------------------
bool pqPythonMacroSettings::getItemIndexInSettings(const QString& macroPath, int& idx, int& size)
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->beginGroup(details::SETTINGS_GROUP(this->CurrentCategory));
  size = settings->beginReadArray("Macros");
  for (idx = 0; idx < size; ++idx)
  {
    settings->setArrayIndex(idx);
    auto macro = settings->value(details::SETTINGS_ITEM_KEY()).toString();
    if (macro == macroPath)
    {
      break;
    }
  }
  settings->endArray();
  settings->endGroup();

  return idx < size;
}

//----------------------------------------------------------------------------
void pqPythonMacroSettings::setItemInSettings(const QString& macroPath, const QString& value)
{
  int idx, currentSize;
  bool exists = this->getItemIndexInSettings(macroPath, idx, currentSize);
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->beginGroup(details::SETTINGS_GROUP(this->CurrentCategory));
  if (exists)
  {
    settings->beginWriteArray("Macros", /*size=*/currentSize);
  }
  else
  {
    settings->beginWriteArray("Macros");
  }
  settings->setArrayIndex(idx);
  settings->setValue(details::SETTINGS_ITEM_KEY(), macroPath);
  settings->setValue(details::SETTINGS_VALUE_KEY(this->CurrentCategory), value);
  settings->endArray();
  settings->endGroup();
}

//----------------------------------------------------------------------------
QString pqPythonMacroSettings::getItemFromSettings(const QString& macroPath)
{
  int idx, size;
  if (!this->getItemIndexInSettings(macroPath, idx, size))
  {
    return QString("");
  }

  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->beginGroup(details::SETTINGS_GROUP(this->CurrentCategory));
  settings->beginReadArray("Macros");
  settings->setArrayIndex(idx);
  auto value = settings->value(details::SETTINGS_VALUE_KEY(this->CurrentCategory)).toString();
  settings->endArray();
  settings->endGroup();
  return value;
}

//----------------------------------------------------------------------------
QMap<QString, QString> pqPythonMacroSettings::getItemSettings()
{
  QMap<QString, QString> valueMap;
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->beginGroup(details::SETTINGS_GROUP(this->CurrentCategory));
  int size = settings->beginReadArray("Macros");
  for (int idx = 0; idx < size; ++idx)
  {
    settings->setArrayIndex(idx);
    auto macroPath = settings->value(details::SETTINGS_ITEM_KEY()).toString();
    auto value = settings->value(details::SETTINGS_VALUE_KEY(this->CurrentCategory)).toString();
    valueMap[macroPath] = value;
  }

  settings->endArray();
  settings->endGroup();
  return valueMap;
}

//----------------------------------------------------------------------------
void pqPythonMacroSettings::writeItemSettings(const QMap<QString, QString>& valueMap)
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->beginGroup(details::SETTINGS_GROUP(this->CurrentCategory));
  settings->remove("Macros");
  settings->beginWriteArray("Macros");
  int idx = 0;
  for (auto macroPathToolTip = valueMap.begin(); macroPathToolTip != valueMap.end();
       macroPathToolTip++)
  {
    settings->setArrayIndex(idx);
    settings->setValue(details::SETTINGS_ITEM_KEY(), macroPathToolTip.key());
    settings->setValue(
      details::SETTINGS_VALUE_KEY(this->CurrentCategory), macroPathToolTip.value());
    idx++;
  }
  settings->endArray();
  settings->endGroup();
}

//----------------------------------------------------------------------------
void pqPythonMacroSettings::removeItemFromSettings(const QString& macroPath)
{
  auto valueMap = this->getItemSettings();
  if (valueMap.remove(macroPath) > 0)
  {
    this->writeItemSettings(valueMap);
  }
}
