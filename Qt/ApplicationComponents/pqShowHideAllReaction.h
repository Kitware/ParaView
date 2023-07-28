// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqShowHideAllReaction_h
#define pqShowHideAllReaction_h

#include "pqReaction.h"

/**
 * @ingroup Reactions
 * Reaction to show or hide all sources output ports.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqShowHideAllReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  enum class ActionType
  {
    Show,
    Hide
  };

  pqShowHideAllReaction(QAction* parent, ActionType action);

  static void act(ActionType action);

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { pqShowHideAllReaction::act(this->Action); }

private:
  Q_DISABLE_COPY(pqShowHideAllReaction)

  /// Show all representations if true, hide if false.
  ActionType Action;
};
#endif
