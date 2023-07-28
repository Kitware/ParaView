// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqColorDialogEventPlayer.h"

#include <QColorDialog>
#include <QRegularExpression>

//----------------------------------------------------------------------------
pqColorDialogEventPlayer::pqColorDialogEventPlayer(QObject* parentObject)
  : Superclass(parentObject)
{
}

//----------------------------------------------------------------------------
pqColorDialogEventPlayer::~pqColorDialogEventPlayer() = default;

//-----------------------------------------------------------------------------
bool pqColorDialogEventPlayer::playEvent(
  QObject* object, const QString& command, const QString& arguments, bool& /*error*/)
{
  QColorDialog* dialog = qobject_cast<QColorDialog*>(object);
  if (!dialog)
  {
    return false;
  }

  QRegularExpression regExp("^(\\d+),(\\d+),(\\d+)$");
  QRegularExpressionMatch match = regExp.match(arguments);
  if ((command == pqColorDialogEventPlayer::EVENT_NAME()) && match.hasMatch())
  {
    QColor rgb(match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt());
    dialog->setCurrentColor(rgb);
    return true;
  }
  else if (command == "done")
  {
    static_cast<QDialog*>(dialog)->done(arguments.toInt());
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
const QString& pqColorDialogEventPlayer::EVENT_NAME()
{
  static const QString eventName("setChosenColor");
  return eventName;
}
