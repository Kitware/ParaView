// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSaveAnimationReaction_h
#define pqSaveAnimationReaction_h

#include "pqReaction.h"

class pqServer;

/**
 * @ingroup Reactions
 * Reaction to save animation.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqSaveAnimationReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
   * Constructor. Parent cannot be nullptr.
   */
  pqSaveAnimationReaction(QAction* parent);

  /**
   * Saves the animation from the active scene.
   * Note that this method is static. Applications can simply use this without
   * having to create a reaction instance.
   */
  static void saveAnimation();

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Updates the enabled state. Applications need not explicitly call this.
   */
  void updateEnableState() override;

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { pqSaveAnimationReaction::saveAnimation(); }

private:
  Q_DISABLE_COPY(pqSaveAnimationReaction)
};

#endif
