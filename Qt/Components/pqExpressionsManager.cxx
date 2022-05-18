/*=========================================================================

   Program: ParaView
   Module:    pqExpressionsManager.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

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
