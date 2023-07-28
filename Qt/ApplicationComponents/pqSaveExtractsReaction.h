// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSaveExtractsReaction_h
#define pqSaveExtractsReaction_h

#include "pqApplicationComponentsModule.h"
#include "pqReaction.h"

/**
 *
 * @class pqSaveExtractsReaction
 * @brief reaction to save extracts.
 * @ingroup Reactions
 *
 * pqSaveExtractsReaction is a reaction that can be connection to a QAction
 * which when triggered should save extracts. This uses "SaveAnimationExtracts"
 * proxy internally to generate extracts using the animation defined on the
 * active session.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqSaveExtractsReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqSaveExtractsReaction(QAction* parent);
  ~pqSaveExtractsReaction() override;

  /**
   * Generates exports.
   */
  static bool generateExtracts();

protected Q_SLOTS:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { pqSaveExtractsReaction::generateExtracts(); }

private:
  Q_DISABLE_COPY(pqSaveExtractsReaction)
};

#endif
