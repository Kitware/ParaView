// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
