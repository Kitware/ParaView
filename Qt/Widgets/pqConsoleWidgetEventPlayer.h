// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqConsoleWidgetEventPlayer_h
#define pqConsoleWidgetEventPlayer_h

#include "pqWidgetEventPlayer.h"
#include "pqWidgetsModule.h" // needed for EXPORT macro.

/**
 * pqConsoleWidgetEventPlayer is used to play back test commands recorded by
 * pqConsoleWidgetEventTranslator for pqConsoleWidget.
 */
class PQWIDGETS_EXPORT pqConsoleWidgetEventPlayer : public pqWidgetEventPlayer
{
  Q_OBJECT
  typedef pqWidgetEventPlayer Superclass;

public:
  pqConsoleWidgetEventPlayer(QObject* parent = nullptr);
  ~pqConsoleWidgetEventPlayer() override;

  /**
   * Callback to play a command.
   */
  using Superclass::playEvent;
  bool playEvent(
    QObject* target, const QString& cmd, const QString& args, bool& errorFlag) override;

private:
  Q_DISABLE_COPY(pqConsoleWidgetEventPlayer)
};

#endif
