// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqExpressionsManager.h"

#include "pqApplicationCore.h"
#include "pqSettings.h"

//----------------------------------------------------------------------------
void pqExpressionsManager::storeToSettings(const QList<pqExpression>& expressions)
{
  pqSettings* settings = pqApplicationCore::instance()->settings();

  settings->beginGroup(SETTINGS_GROUP());
  // cleanup previous content.
  settings->remove("");
  QString settingString;
  for (auto expr : expressions)
  {
    settingString += QString("%1;%2;%3;").arg(expr.Group).arg(expr.Name).arg(expr.Value);
  }
  settings->setValue(SETTINGS_KEY(), settingString);
  settings->endGroup();
}

//----------------------------------------------------------------------------
QList<pqExpressionsManager::pqExpression> pqExpressionsManager::getExpressionsFromSettings()
{
  QList<pqExpression> expressions;
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->beginGroup(SETTINGS_GROUP());

  if (settings->contains(SETTINGS_KEY()))
  {
    QString settingString = settings->value(SETTINGS_KEY()).toString();
    QStringList values = settingString.split(";");
    for (int i = 0; i < values.size() - 1; i += 3)
    {
      pqExpression newExpr(values[i], values[i + 1], values[i + 2]);
      if (!expressions.contains(newExpr))
      {
        expressions.push_back(newExpr);
      }
    }
  }
  settings->endGroup();

  return expressions;
}

//----------------------------------------------------------------------------
QList<pqExpressionsManager::pqExpression> pqExpressionsManager::getExpressionsFromSettings(
  const QString& group)
{
  QList<pqExpression> settings = pqExpressionsManager::getExpressionsFromSettings();
  QList<pqExpression> expressions;
  for (auto expr : settings)
  {
    if (expr.Group == group)
    {
      expressions << expr;
    }
  }
  return expressions;
}

//----------------------------------------------------------------------------
bool pqExpressionsManager::addExpressionToSettings(const QString& group, const QString& value)
{
  if (value.isEmpty() || group.isEmpty())
  {
    return false;
  }

  QList<pqExpression> expressions = pqExpressionsManager::getExpressionsFromSettings();
  pqExpression newExpr(group, value);
  if (!expressions.contains(newExpr))
  {
    expressions.push_back(newExpr);
    pqExpressionsManager::storeToSettings(expressions);
    return true;
  }

  return false;
}
