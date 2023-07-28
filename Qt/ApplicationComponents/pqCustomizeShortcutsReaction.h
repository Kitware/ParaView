// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCustomizeShortcutsReaction_h
#define pqCustomizeShortcutsReaction_h

#include "pqReaction.h"

class PQAPPLICATIONCOMPONENTS_EXPORT pqCustomizeShortcutsReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqCustomizeShortcutsReaction(QAction* parent);

  /**
   * Adds camera link with the active view.
   */
  static void showCustomizeShortcutsDialog();

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { pqCustomizeShortcutsReaction::showCustomizeShortcutsDialog(); }
};

#endif
