// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqColorDialogEventTranslator.h"

#include "pqColorDialogEventPlayer.h"

#include <QColorDialog>
#include <QEvent>
#include <QtDebug>

//-----------------------------------------------------------------------------
pqColorDialogEventTranslator::pqColorDialogEventTranslator(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqColorDialogEventTranslator::~pqColorDialogEventTranslator() = default;

//-----------------------------------------------------------------------------
bool pqColorDialogEventTranslator::translateEvent(
  QObject* object, QEvent* tr_event, bool& /*error*/)
{
  // Capture events from QColorDialog and all its children.

  QColorDialog* color_dialog = nullptr;
  while (object && !color_dialog)
  {
    color_dialog = qobject_cast<QColorDialog*>(object);
    object = object->parent();
  }

  if (!color_dialog)
  {
    return false;
  }

  if (tr_event->type() == QEvent::FocusIn)
  {
    QObject::connect(color_dialog, SIGNAL(currentColorChanged(const QColor&)), this,
      SLOT(onColorChosen(const QColor&)), Qt::UniqueConnection);
    QObject::connect(
      color_dialog, SIGNAL(finished(int)), this, SLOT(onFinished(int)), Qt::UniqueConnection);
  }

  return true;
}

//-----------------------------------------------------------------------------
void pqColorDialogEventTranslator::onColorChosen(const QColor& color)
{
  QColorDialog* color_dialog = qobject_cast<QColorDialog*>(this->sender());

  QString colorvalue = QString("%1,%2,%3").arg(color.red()).arg(color.green()).arg(color.blue());

  Q_EMIT this->recordEvent(color_dialog, pqColorDialogEventPlayer::EVENT_NAME(), colorvalue);
}

//-----------------------------------------------------------------------------
void pqColorDialogEventTranslator::onFinished(int result)
{
  QColorDialog* color_dialog = qobject_cast<QColorDialog*>(this->sender());
  Q_EMIT recordEvent(color_dialog, "done", QString::number(result));
}
