// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqIconSettings.h"

#include "pqApplicationCore.h"
#include "pqSettings.h"

namespace
{
namespace details
{
QString SETTINGS_ICONS_GROUP()
{
  return "Icons";
}

QString SETTINGS_ICON_KEY()
{
  return "icon";
}

QString SETTINGS_ITEM_KEY()
{
  return "item";
}
}
}

//----------------------------------------------------------------------------
pqIconSettings::pqIconSettings(const QString& key)
  : IconCategory(key)
{
}

//----------------------------------------------------------------------------
bool pqIconSettings::getItemIndexInSettings(const QString& macroPath, int& idx, int& size)
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->beginGroup(details::SETTINGS_ICONS_GROUP());
  size = settings->beginReadArray(this->IconCategory);
  idx = 0;
  for (; idx < size; ++idx)
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
void pqIconSettings::setItemIconInSettings(const QString& macroPath, const QString& iconPath)
{
  int idx, currentSize;
  bool exists = this->getItemIndexInSettings(macroPath, idx, currentSize);
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->beginGroup(details::SETTINGS_ICONS_GROUP());
  if (exists)
  {
    settings->beginWriteArray(this->IconCategory, /*size=*/currentSize);
  }
  else
  {
    settings->beginWriteArray(this->IconCategory);
  }
  settings->setArrayIndex(idx);
  settings->setValue(details::SETTINGS_ITEM_KEY(), macroPath);
  settings->setValue(details::SETTINGS_ICON_KEY(), iconPath);
  settings->endArray();
  settings->endGroup();
}

//----------------------------------------------------------------------------
QString pqIconSettings::getIconFromSettings(const QString& macroPath)
{
  int idx, size;
  if (!this->getItemIndexInSettings(macroPath, idx, size))
  {
    return QString("");
  }

  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->beginGroup(details::SETTINGS_ICONS_GROUP());
  settings->beginReadArray(this->IconCategory);
  settings->setArrayIndex(idx);
  auto iconPath = settings->value(details::SETTINGS_ICON_KEY()).toString();
  settings->endArray();
  settings->endGroup();
  return iconPath;
}

//----------------------------------------------------------------------------
QMap<QString, QString> pqIconSettings::getSettings()
{
  QMap<QString, QString> iconMap;
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->beginGroup(details::SETTINGS_ICONS_GROUP());
  int size = settings->beginReadArray(this->IconCategory);
  for (int idx = 0; idx < size; ++idx)
  {
    settings->setArrayIndex(idx);
    auto item = settings->value(details::SETTINGS_ITEM_KEY()).toString();
    auto icon = settings->value(details::SETTINGS_ICON_KEY()).toString();
    iconMap[item] = icon;
  }

  settings->endArray();
  settings->endGroup();
  return iconMap;
}

//----------------------------------------------------------------------------
void pqIconSettings::writeSettings(const QMap<QString, QString>& iconMap)
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->beginGroup(details::SETTINGS_ICONS_GROUP());
  settings->remove(this->IconCategory);
  settings->beginWriteArray(this->IconCategory);
  int idx = 0;
  for (auto itemIcon = iconMap.begin(); itemIcon != iconMap.end(); itemIcon++)
  {
    settings->setArrayIndex(idx);
    settings->setValue(details::SETTINGS_ITEM_KEY(), itemIcon.key());
    settings->setValue(details::SETTINGS_ICON_KEY(), itemIcon.value());
    idx++;
  }
  settings->endArray();
  settings->endGroup();
}

//----------------------------------------------------------------------------
void pqIconSettings::removeItemFromSettings(const QString& itemKey)
{
  auto iconMap = this->getSettings();
  if (iconMap.remove(itemKey) > 0)
  {
    this->writeSettings(iconMap);
  }
}
