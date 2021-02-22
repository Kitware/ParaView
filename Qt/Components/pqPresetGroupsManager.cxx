/*=========================================================================

   Program: ParaView
   Module:  pqPresetGroupsManager.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/

#include "pqPresetGroupsManager.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

pqPresetGroupsManager::pqPresetGroupsManager(QObject* p)
  : Superclass(p)
{
}

pqPresetGroupsManager::~pqPresetGroupsManager() = default;

int pqPresetGroupsManager::numberOfGroups()
{
  return this->GroupNames.size();
}

int pqPresetGroupsManager::numberOfPresetsInGroup(const QString& groupName)
{
  return this->Groups.value(groupName, QList<QString>()).size();
}

int pqPresetGroupsManager::presetRankInGroup(const QString& presetName, const QString& groupName)
{
  const QList<QString>& group = this->Groups.value(groupName, QList<QString>());
  return group.indexOf(presetName);
}

QList<QString> pqPresetGroupsManager::groupNames()
{
  return this->GroupNames;
}

QString pqPresetGroupsManager::groupName(int i)
{
  return this->GroupNames[i];
}

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

void pqPresetGroupsManager::clearGroups()
{
  this->Groups.clear();
  this->GroupNames.clear();
}

void pqPresetGroupsManager::replaceGroups(const QString& json)
{
  this->clearGroups();
  this->loadGroups(json);
}
