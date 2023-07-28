// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqColorButtonEventPlayer.h"

#include "pqColorChooserButton.h"
#include "pqTestUtility.h"

#include <QColor>
#include <QRegularExpression>

//----------------------------------------------------------------------------
pqColorButtonEventPlayer::pqColorButtonEventPlayer(QObject* parentObject)
  : Superclass(parentObject)
{
}

//----------------------------------------------------------------------------
pqColorButtonEventPlayer::~pqColorButtonEventPlayer() = default;

//-----------------------------------------------------------------------------
bool pqColorButtonEventPlayer::playEvent(
  QObject* object, const QString& command, const QString& arguments, bool& /*error*/)
{
  pqColorChooserButton* button = qobject_cast<pqColorChooserButton*>(object);
  if (!button)
  {
    return false;
  }

  QRegularExpression regExp("^(\\d+),(\\d+),(\\d+)$");
  QRegularExpressionMatch match = regExp.match(arguments);
  if ((command == pqColorButtonEventPlayer::EVENT_NAME()) && match.hasMatch())
  {
    QColor rgb(match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt());
    button->setChosenColor(rgb);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
const QString& pqColorButtonEventPlayer::EVENT_NAME()
{
  static const QString eventName("setChosenColor");
  return eventName;
}
