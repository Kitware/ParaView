// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqTranslationsTesting.h"
#include "pqCoreUtilities.h"
#include "pqObjectNaming.h"

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QGuiApplication>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QRegularExpression>
#include <QStack>
#include <QTextEdit>
#include <QWindow>

//-----------------------------------------------------------------------------
pqTranslationsTesting::pqTranslationsTesting(QObject* parent)
  : QObject(parent)
{
}

//-----------------------------------------------------------------------------
pqTranslationsTesting::~pqTranslationsTesting() = default;

//-----------------------------------------------------------------------------
bool pqTranslationsTesting::isIgnored(QObject* object, const char* property) const
{
  for (QPair<QString, QString> pair : TRANSLATION_IGNORE_STRINGS)
  {
    if (pqObjectNaming::GetName(*object) == pair.first &&
      (QString(property) == pair.second || QString(pair.second).isEmpty()))
    {
      return true;
    }
  }
  for (QPair<QRegularExpression, QString> pair : TRANSLATION_IGNORE_REGEXES)
  {
    if (pqObjectNaming::GetName(*object).contains(pair.first) &&
      (QString(property) == pair.second || QString(pair.second).isEmpty()))
    {
      return true;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
void pqTranslationsTesting::printWarningIfUntranslated(QObject* object, const char* property) const
{
  QString str = object->property(property).toString();
  bool isNumber;
  // Replace commas by point so `toDouble` can detect numbers
  str.replace(QRegularExpression(","), QString("."));
  str.toDouble(&isNumber);
  if (isNumber)
  {
    return;
  }

  // Remove HTML tags
  str.remove(QRegularExpression("<[\\/]?[aZ-zA-Z]+ ?[^>]*[\\/]?>"));
  // Remove & before _TranslationTestings
  str.replace(QRegularExpression("&(_TranslationTesting)"), QString("\\1"));
  // Remove <>, [] and () around expressions
  str.replace(QRegularExpression("[\\[{<\\(](.*)[\\]}>\\)]"), QString("\\1"));
  // isIgnored can be slow because of regex usage, so we do it at last
  if (!str.contains(QRegularExpression("^(_TranslationTesting.*)?$")) &&
    !this->isIgnored(object, property))
  {

    qCritical() << QString("Error: %2 of %1 not translated : '%3'")
                     .arg(pqObjectNaming::GetName(*object), property, str);
    return;
  }
}

//-----------------------------------------------------------------------------
bool pqTranslationsTesting::shouldBeIgnored(QWidget* widget) const
{
  return dynamic_cast<QLineEdit*>(widget) || dynamic_cast<QTextEdit*>(widget);
}

//-----------------------------------------------------------------------------
void pqTranslationsTesting::recursiveFindUntranslatedStrings(QWidget* widget) const
{
  if (!this->shouldBeIgnored(widget))
  {
    this->printWarningIfUntranslated(widget, "text");
    this->printWarningIfUntranslated(widget, "toolTip");
    this->printWarningIfUntranslated(widget, "windowTitle");
    this->printWarningIfUntranslated(widget, "placeholderText");
    this->printWarningIfUntranslated(widget, "label");
  }

  for (QAction* action : widget->actions())
  {
    this->printWarningIfUntranslated(action, "text");
    this->printWarningIfUntranslated(action, "toolTip");
    this->printWarningIfUntranslated(action, "windowTitle");
    this->printWarningIfUntranslated(action, "placeholderText");
    this->printWarningIfUntranslated(action, "label");
  }
  for (QWidget* child :
    widget->findChildren<QWidget*>(QRegularExpression("^.*$"), Qt::FindDirectChildrenOnly))
  {
    this->recursiveFindUntranslatedStrings(child);
  }
}

//-----------------------------------------------------------------------------
void pqTranslationsTesting::onStartup()
{
  QMainWindow* win = qobject_cast<QMainWindow*>(pqCoreUtilities::mainWidget());
  this->recursiveFindUntranslatedStrings(win);
}
