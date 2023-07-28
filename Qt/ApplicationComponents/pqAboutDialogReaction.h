// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqAboutDialogReaction_h
#define pqAboutDialogReaction_h

#include "pqReaction.h"

/**
 * @ingroup Reactions
 * pqAboutDialogReaction used to show the standard about dialog for the
 * application.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqAboutDialogReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqAboutDialogReaction(QAction* parent);

  /**
   * Shows the about dialog for the application.
   */
  static void showAboutDialog();

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { pqAboutDialogReaction::showAboutDialog(); }

private:
  Q_DISABLE_COPY(pqAboutDialogReaction)
};

#endif
