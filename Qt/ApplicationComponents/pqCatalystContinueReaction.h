// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCatalystContinueReaction_h
#define pqCatalystContinueReaction_h

#include "pqCatalystPauseSimulationReaction.h"
#include <QPointer>

class pqLiveInsituVisualizationManager;

/**
 * Reaction for setting a breakpoint to Catalyst CoProcessing Engine
 * for Live-Data Visualization.
 * @ingroup Reactions
 * @ingroup LiveInsitu
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCatalystContinueReaction
  : public pqCatalystPauseSimulationReaction
{
  Q_OBJECT
  typedef pqCatalystPauseSimulationReaction Superclass;

public:
  pqCatalystContinueReaction(QAction* parentTemp = nullptr)
    : Superclass(parentTemp)
  {
  }

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void updateEnableState() override
  {
    pqCatalystPauseSimulationReaction::updateEnableState(CONTINUE);
  }

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { this->setPauseSimulation(false); }

private:
  Q_DISABLE_COPY(pqCatalystContinueReaction)
};

#endif
