// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqColorButtonEventTranslator.h"

#include "pqColorButtonEventPlayer.h"
#include "pqColorChooserButton.h"
#include "pqTestUtility.h"

#include <QEvent>
#include <QMenu>
#include <QtDebug>

//-----------------------------------------------------------------------------
pqColorButtonEventTranslator::pqColorButtonEventTranslator(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqColorButtonEventTranslator::~pqColorButtonEventTranslator() = default;

//-----------------------------------------------------------------------------
bool pqColorButtonEventTranslator::translateEvent(
  QObject* object, QEvent* tr_event, bool& /*error*/)
{
  // Capture events from pqColorChooserButton and all its children.
  if (qobject_cast<QMenu*>(object))
  {
    // we don't want to capture events from the menu on the color chooser button.
    return false;
  }

  pqColorChooserButton* color_button = nullptr;
  while (object && !color_button)
  {
    color_button = qobject_cast<pqColorChooserButton*>(object);
    object = object->parent();
  }

  if (!color_button)
  {
    return false;
  }

  if (tr_event->type() == QEvent::FocusIn)
  {
    QObject::disconnect(color_button, nullptr, this, nullptr);
    QObject::connect(color_button, SIGNAL(validColorChosen(const QColor&)), this,
      SLOT(onColorChosen(const QColor&)));
  }

  return true;
}

//-----------------------------------------------------------------------------
void pqColorButtonEventTranslator::onColorChosen(const QColor& color)
{
  pqColorChooserButton* color_button = qobject_cast<pqColorChooserButton*>(this->sender());

  QString colorvalue = QString("%1,%2,%3").arg(color.red()).arg(color.green()).arg(color.blue());

  Q_EMIT this->recordEvent(color_button, pqColorButtonEventPlayer::EVENT_NAME(), colorvalue);
}
