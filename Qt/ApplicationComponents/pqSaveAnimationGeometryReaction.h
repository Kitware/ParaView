// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSaveAnimationGeometryReaction_h
#define pqSaveAnimationGeometryReaction_h

#include "pqReaction.h"

/**
 * @ingroup Reactions
 * Reaction to save animation geometry.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqSaveAnimationGeometryReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
   * Constructor. Parent cannot be nullptr.
   */
  pqSaveAnimationGeometryReaction(QAction* parent);

  /**
   * Saves the animation from the active scene.
   * Note that this method is static. Applications can simply use this without
   * having to create a reaction instance.
   */
  static void saveAnimationGeometry();
  static void saveAnimationGeometry(const QString& filename);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Updates the enabled state. Applications need not explicitly call this.
   */
  void updateEnableState() override;

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { pqSaveAnimationGeometryReaction::saveAnimationGeometry(); }

private:
  Q_DISABLE_COPY(pqSaveAnimationGeometryReaction)
};

#endif
