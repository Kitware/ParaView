// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#include "pqConsoleWidgetEventPlayer.h"

#include "pqConsoleWidget.h"
#include <QtDebug>

//-----------------------------------------------------------------------------
pqConsoleWidgetEventPlayer::pqConsoleWidgetEventPlayer(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqConsoleWidgetEventPlayer::~pqConsoleWidgetEventPlayer() = default;

//-----------------------------------------------------------------------------
bool pqConsoleWidgetEventPlayer::playEvent(
  QObject* target, const QString& cmd, const QString& args, bool& errorFlag)
{
  pqConsoleWidget* widget = qobject_cast<pqConsoleWidget*>(target);
  if (!widget)
  {
    return false;
  }

  if (cmd == "executeCommand")
  {
    widget->printAndExecuteCommand(args);
    return true;
  }

  qCritical() << "Unknown command for pqConsoleWidget : " << target << " " << cmd << " " << args;
  errorFlag = true;
  return true;
}
