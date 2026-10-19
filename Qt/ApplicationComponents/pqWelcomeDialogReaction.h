// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqWelcomeDialogReaction_h
#define pqWelcomeDialogReaction_h

#include "pqReaction.h"

/**
 * @ingroup Reactions
 * pqWelcomeDialogReaction used to show the welcome dialog for the application.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqWelcomeDialogReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqWelcomeDialogReaction(QAction* parent);

  /**
   * Shows the welcome dialog for the application.
   */
  static void showWelcomeDialog();

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { pqWelcomeDialogReaction::showWelcomeDialog(); }

private:
  Q_DISABLE_COPY(pqWelcomeDialogReaction)
};

#endif
