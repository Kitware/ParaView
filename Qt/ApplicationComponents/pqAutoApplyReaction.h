// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqAutoApplyReaction_h
#define pqAutoApplyReaction_h

#include "pqReaction.h"

/**
 * @ingroup Reactions
 * Reaction for enabling/disabling auto-apply.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqAutoApplyReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqAutoApplyReaction(QAction* parent = nullptr);

  /**
   * Set the status of auto-apply.
   */
  static void setAutoApply(bool);

  /**
   * Get the status of auto-apply.
   */
  static bool autoApply();

protected Q_SLOTS:
  void updateState();
  void checkStateChanged(bool);

private:
  Q_DISABLE_COPY(pqAutoApplyReaction)
};

#endif
