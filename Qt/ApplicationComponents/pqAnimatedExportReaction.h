// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqAnimatedExportReaction_h
#define pqAnimatedExportReaction_h

#include "pqReaction.h"

/**
 * @ingroup Reactions
 * Reaction to export an animated scene in a web format.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqAnimatedExportReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
   * Constructor. Parent cannot be nullptr.
   */
  pqAnimatedExportReaction(QAction* parent);

  /**
   * Export the animated scene from the active render view.
   * Note that this method is static. Applications can simply use this without
   * having to create a reaction instance.
   */
  static void Export();
  static void Export(const QString& filename);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Updates the enabled state. Applications need not explicitly call
   * this.
   */
  void updateEnableState() override;

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { pqAnimatedExportReaction::Export(); }

private:
  Q_DISABLE_COPY(pqAnimatedExportReaction)
};

#endif
