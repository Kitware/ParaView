// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSetMainWindowTitleReaction_h
#define pqSetMainWindowTitleReaction_h

#include "pqReaction.h"

/**
 * @ingroup Reactions
 * pqSetMainWindowTitleReaction is the reaction that popups the Set Window Title
 * Dialog allowing the user edit the main window title.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqSetMainWindowTitleReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
   * Constructor. Parent cannot be nullptr.
   */
  pqSetMainWindowTitleReaction(QAction* parent);
  ~pqSetMainWindowTitleReaction() override;

  /**
   * Show the main window title editor dialog.
   */
  void showSetMainWindowTitleDialog();

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { pqSetMainWindowTitleReaction::showSetMainWindowTitleDialog(); }

private:
  Q_DISABLE_COPY(pqSetMainWindowTitleReaction)
};

#endif
