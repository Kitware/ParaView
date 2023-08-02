// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqConsoleWidgetEventTranslator.h"

#include "pqConsoleWidget.h"
#include <QEvent>

//-----------------------------------------------------------------------------
pqConsoleWidgetEventTranslator::pqConsoleWidgetEventTranslator(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqConsoleWidgetEventTranslator::~pqConsoleWidgetEventTranslator() = default;

//-----------------------------------------------------------------------------
bool pqConsoleWidgetEventTranslator::translateEvent(
  QObject* target, QEvent* qtevent, bool& errorFlag)
{
  Q_UNUSED(errorFlag);
  // Capture inputs for pqConsoleWidget and all its children
  pqConsoleWidget* object = nullptr;
  for (QObject* current = target; current != nullptr; current = current->parent())
  {
    object = qobject_cast<pqConsoleWidget*>(current);
    if (object)
    {
      break;
    }
  }
  if (!object)
  {
    return false;
  }

  if (qtevent->type() == QEvent::FocusIn)
  {
    if (this->CurrentObject)
    {
      QObject::disconnect(this->CurrentObject, nullptr, this, nullptr);
    }
    this->CurrentObject = object;
    QObject::connect(this->CurrentObject, SIGNAL(executeCommand(const QString&)), this,
      SLOT(recordCommand(const QString&)));
  }

  return true;
}

//-----------------------------------------------------------------------------
void pqConsoleWidgetEventTranslator::recordCommand(const QString& text)
{
  Q_EMIT this->recordEvent(this->CurrentObject, "executeCommand", text);
}
