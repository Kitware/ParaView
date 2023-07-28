// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqLinkSelectionReaction_h
#define pqLinkSelectionReaction_h

#include "pqReaction.h"

/**
 * @ingroup Reactions
 * Reaction for change pipeline input for the currently selected element.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqLinkSelectionReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqLinkSelectionReaction(QAction* parent = nullptr);

  /**
   * Link selection of the current active source with
   * the current selected source
   */
  static void linkSelection();

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Updates the enabled state. Applications need not explicitly call this.
   */
  void updateEnableState() override;

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { pqLinkSelectionReaction::linkSelection(); }

private:
  Q_DISABLE_COPY(pqLinkSelectionReaction)
};
#endif
