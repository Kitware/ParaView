// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqPresetGroupsManager.h"

#include "vtkSMSettings.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonValueRef>
#include <QSet>
#include <QString>

//-----------------------------------------------------------------------------
pqPresetGroupsManager::pqPresetGroupsManager(QObject* p)
  : Superclass(p)
{
  this->connect(this, &pqPresetGroupsManager::groupsUpdated, this,
    &pqPresetGroupsManager::saveGroupsToSettings);
}

//-----------------------------------------------------------------------------
pqPresetGroupsManager::~pqPresetGroupsManager() = default;

//-----------------------------------------------------------------------------
int pqPresetGroupsManager::numberOfGroups()
{
  return this->GroupNames.size();
}

//-----------------------------------------------------------------------------
int pqPresetGroupsManager::numberOfPresetsInGroup(const QString& groupName)
{
  return this->Groups.value(groupName, QList<QString>()).size();
}

//-----------------------------------------------------------------------------
QList<QString> pqPresetGroupsManager::presetsInGroup(const QString& groupName)
{
  return this->Groups.value(groupName, QList<QString>());
}

//-----------------------------------------------------------------------------
int pqPresetGroupsManager::presetRankInGroup(const QString& presetName, const QString& groupName)
{
  const QList<QString>& group = this->Groups.value(groupName, QList<QString>());
  return group.indexOf(presetName);
}

//-----------------------------------------------------------------------------
QList<QString> pqPresetGroupsManager::groupNames()
{
  return this->GroupNames;
}

//-----------------------------------------------------------------------------
QString pqPresetGroupsManager::groupName(int i)
{
  return this->GroupNames[i];
}

//-----------------------------------------------------------------------------
void pqPresetGroupsManager::loadGroups(const QString& jsonString)
{
  QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonString.toUtf8());
  if (jsonDoc.isNull() || jsonDoc.isObject())
  {
    // TODO error messages
    return;
  }
  QJsonArray jArray = jsonDoc.array();
  for (int i = 0; i < jArray.size(); ++i)
  {
    QJsonValueRef ref = jArray[i];
    if (!ref.isObject())
    {
      continue;
    }
    QJsonObject jGroup = ref.toObject();
    if (!jGroup.contains("groupName") || !jGroup.contains("presets"))
    {
      continue;
    }
    QString name = jGroup["groupName"].toString();
    this->GroupNames.push_back(name);
    QList<QString> groupMembers = this->Groups.value(name, QList<QString>());
    QJsonArray jMembers = jGroup["presets"].toArray();
    for (int j = 0; j < jMembers.size(); ++j)
    {
      groupMembers.push_back(jMembers[j].toString());
    }
    this->Groups[name] = groupMembers;
  }
  // pqPresetDialog relies on "Default" being available.  Even an empty group should be fine.
  if (!this->GroupNames.contains("Default"))
  {
    this->GroupNames.append("Default");
    this->Groups["Default"] = QList<QString>();
  }
  Q_EMIT groupsUpdated();
}

//-----------------------------------------------------------------------------
void pqPresetGroupsManager::clearGroups()
{
  this->Groups.clear();
  this->GroupNames.clear();
}

//-----------------------------------------------------------------------------
void pqPresetGroupsManager::replaceGroups(const QString& json)
{
  this->clearGroups();
  this->loadGroups(json);
}

//-----------------------------------------------------------------------------
bool pqPresetGroupsManager::loadGroupsFromSettings()
{
  const QString settingPresetGroups = QString::fromStdString(
    vtkSMSettings::GetInstance()->GetSettingAsString("TransferFunctionPresets.Groups", ""));
  if (settingPresetGroups.isEmpty())
  {
    return false;
  }

  // combine what exists in the settings with the default groups
  const QString presetGroups = pqPresetGroupsManager::getPresetGroupsJson();
  const QJsonDocument presetGroupsJson = QJsonDocument::fromJson(presetGroups.toUtf8());
  const QJsonDocument settingPresetGroupsJson =
    QJsonDocument::fromJson(settingPresetGroups.toUtf8());

  // Find "User" groupName, and "Default" presets in settingPresetGroupsJson
  QJsonObject userGroup;
  QList<QString> settingsDefaultPresets;
  for (const QJsonValue value : settingPresetGroupsJson.array())
  {
    QJsonObject obj = value.toObject();
    if (obj["groupName"].toString() == "Default")
    {
      // Extract the preset names
      for (const QJsonValue preset : obj["presets"].toArray())
      {
        settingsDefaultPresets.push_back(preset.toString());
      }
    }
    else if (obj["groupName"].toString() == "User")
    {
      userGroup = obj;
    }
  }

  // Add "User" groupName to the end of presetGroupsJson
  QJsonArray presetGroupsArray = presetGroupsJson.array();
  if (!userGroup.isEmpty())
  {
    presetGroupsArray.append(userGroup);
  }

  // Add all the presets from the "Default" groupName of settingPresetGroupsJson
  // that do not exist in the "Default" groupName of presetGroupsJson
  for (int i = 0; i < presetGroupsArray.size(); ++i)
  {
    QJsonObject obj = presetGroupsArray[i].toObject();
    if (obj["groupName"].toString() == "Default")
    {
      QJsonArray defaultPresetsArray = obj["presets"].toArray();
      QSet<QString> defaultPresetsSet;
      for (const QJsonValue preset : defaultPresetsArray)
      {
        defaultPresetsSet.insert(preset.toString());
      }

      // Find the presets that are not part of the "Default" groupName of presetGroupsJson
      for (const QString& preset : settingsDefaultPresets)
      {
        if (!defaultPresetsSet.contains(preset))
        {
          defaultPresetsArray.append(preset);
        }
      }

      obj["presets"] = defaultPresetsArray;
      presetGroupsArray[i] = obj; // Update the array with the modified object
      break;
    }
  }

  // Get the string representation of the updated presetGroupsJson
  const QJsonDocument updatedPresetGroupsJson(presetGroupsArray);
  const QString updatedPresetGroups = updatedPresetGroupsJson.toJson(QJsonDocument::Compact);
  this->replaceGroups(updatedPresetGroups);

  return true;
  return true;
}

//-----------------------------------------------------------------------------
void pqPresetGroupsManager::saveGroupsToSettings()
{
  QJsonArray root;
  for (auto groupsIt = this->Groups.begin(); groupsIt != this->Groups.end(); ++groupsIt)
  {
    QJsonObject group;
    group.insert("groupName", groupsIt.key());
    group.insert("presets", QJsonArray::fromStringList(groupsIt.value()));
    root.append(group);
  }
  vtkSMSettings::GetInstance()->SetSetting("TransferFunctionPresets.Groups",
    QJsonDocument(root).toJson(QJsonDocument::Compact).toStdString());
}

//-----------------------------------------------------------------------------
void pqPresetGroupsManager::addToGroup(const QString& groupName, const QString& presetName)
{
  auto groupIterator = this->Groups.find(groupName);
  if (groupIterator == this->Groups.end())
  {
    groupIterator = this->Groups.insert(groupName, QList<QString>());
    this->GroupNames.push_back(groupName);
  }

  auto& presets = groupIterator.value();
  // preset not present in group
  if (presets.indexOf(presetName) == -1)
  {
    presets.push_back(presetName);
    Q_EMIT groupsUpdated();
  }
}

//-----------------------------------------------------------------------------
void pqPresetGroupsManager::removeFromGroup(const QString& groupName, const QString& presetName)
{
  auto groupIterator = this->Groups.find(groupName);
  if (groupIterator == this->Groups.end())
  {
    return;
  }

  auto& presets = groupIterator.value();
  auto presetRemoved = presets.removeOne(presetName);
  if (presetRemoved)
  {
    // if any group other than Default is empty, there is no point in keeping it
    if (presets.isEmpty() && groupName != "Default")
    {
      this->Groups.erase(groupIterator);
      this->GroupNames.removeOne(groupName);
    }
    Q_EMIT groupsUpdated();
  }
}

//-----------------------------------------------------------------------------
void pqPresetGroupsManager::removeFromAllGroups(const QString& presetName)
{
  // Make a copy because some groups might get deleted during removal of presets if they become
  // empty, which comme make the iterator invalid
  auto const groupNames = this->GroupNames;
  for (auto const& groupName : groupNames)
  {
    this->removeFromGroup(groupName, presetName);
  }
}

//-----------------------------------------------------------------------------
QString pqPresetGroupsManager::getPresetGroupsJson()
{
  QFile groupsFile(":pqWidgets/pqPresetGroups.json");
  if (!groupsFile.open(QIODevice::ReadOnly))
  {
    qWarning() << "Could not load preset group list.";
    return QString();
  }
  return groupsFile.readAll();
}
