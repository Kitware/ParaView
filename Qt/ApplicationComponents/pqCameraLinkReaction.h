// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCameraLinkReaction_h
#define pqCameraLinkReaction_h

#include "pqReaction.h"

class PQAPPLICATIONCOMPONENTS_EXPORT pqCameraLinkReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqCameraLinkReaction(QAction* parent);

  /**
   * Adds camera link with the active view.
   */
  static void addCameraLink();

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Updates the enabled state. Applications need not explicitly call this.
   */
  void updateEnableState() override;

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { pqCameraLinkReaction::addCameraLink(); }

private:
  Q_DISABLE_COPY(pqCameraLinkReaction)
};

#endif
