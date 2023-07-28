// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqChangeFileNameReaction_h
#define pqChangeFileNameReaction_h

#include "pqReaction.h"

/**
 * @ingroup Reactions
 * Reaction for change file of current active reader.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqChangeFileNameReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqChangeFileNameReaction(QAction* parent = nullptr);
  ~pqChangeFileNameReaction() override = default;

  /**
   * Changes the input for the active source.
   */
  static void changeFileName();

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
  void onTriggered() override { pqChangeFileNameReaction::changeFileName(); }

private:
  Q_DISABLE_COPY(pqChangeFileNameReaction)
};

#endif
