// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqLoadStateReaction_h
#define pqLoadStateReaction_h

#include "pqReaction.h"
#include "vtkType.h"

class pqServer;

/**
 * @ingroup Reactions
 * Reaction for load state action.
 */
class pqServer;
class PQAPPLICATIONCOMPONENTS_EXPORT pqLoadStateReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
   * Constructor. Parent cannot be nullptr.
   */
  pqLoadStateReaction(QAction* parent);

  /**
   * Loads the state file.
   * Note that this method is static. Applications can simply use this without
   * having to create a reaction instance.
   * If no server is specified, active server is used.
   */
  static void loadState(const QString& filename, bool dialogBlocked = false,
    pqServer* server = nullptr, vtkTypeUInt32 location = 0x10 /*vtkPVSession::CLIENT*/);
  static void loadState();

  /**
   * Set default active view after state is loaded. Make the first view active.
   */
  static void activateView();

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Updates the enabled state. Applications need not explicitly call this.
   */
  void updateEnableState() override;

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { pqLoadStateReaction::loadState(); }

private:
  Q_DISABLE_COPY(pqLoadStateReaction)
};

#endif
