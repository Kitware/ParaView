// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqMacroReaction_h
#define pqMacroReaction_h

#include "pqMasterOnlyReaction.h"

/**
 * @ingroup Reactions
 * Reaction for creating or deleting a python macro
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqMacroReaction : public pqMasterOnlyReaction
{
  Q_OBJECT
  typedef pqMasterOnlyReaction Superclass;

public:
  pqMacroReaction(QAction* parent);

  /**
   * define a python file as a macro and save it.
   */
  static void createMacro();

protected Q_SLOTS:
  void enable(bool);

protected: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { pqMacroReaction::createMacro(); }

private:
  Q_DISABLE_COPY(pqMacroReaction)
};

#endif
