// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqTimerLogReaction_h
#define pqTimerLogReaction_h

#include "pqReaction.h"

/**
 * @ingroup Reactions
 * Reaction for showing the timer log dialog.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqTimerLogReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqTimerLogReaction(QAction* parentObject)
    : Superclass(parentObject)
  {
  }

  /**
   * Pops up (or raises) the timer log dialog.
   */
  static void showTimerLog();

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { pqTimerLogReaction::showTimerLog(); }

private:
  Q_DISABLE_COPY(pqTimerLogReaction)
};

#endif
