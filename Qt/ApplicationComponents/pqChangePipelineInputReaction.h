// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqChangePipelineInputReaction_h
#define pqChangePipelineInputReaction_h

#include "pqReaction.h"

/**
 * @ingroup Reactions
 * Reaction for change pipeline input for the currently selected element.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqChangePipelineInputReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqChangePipelineInputReaction(QAction* parent = nullptr);

  /**
   * Changes the input for the active source.
   */
  static void changeInput();

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
  void onTriggered() override { pqChangePipelineInputReaction::changeInput(); }

private:
  Q_DISABLE_COPY(pqChangePipelineInputReaction)
};

#endif
