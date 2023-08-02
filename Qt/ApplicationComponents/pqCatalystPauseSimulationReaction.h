// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCatalystPauseSimulationReaction_h
#define pqCatalystPauseSimulationReaction_h

#include "pqReaction.h"
#include <QPointer>

class pqLiveInsituVisualizationManager;
class vtkSMLiveInsituLinkProxy;

/**
 * Reaction for setting a breakpoint to Catalyst CoProcessing Engine
 * for Live-Data Visualization.
 * @ingroup Reactions
 * @ingroup LiveInsitu
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCatalystPauseSimulationReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqCatalystPauseSimulationReaction(QAction* parent = nullptr);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void updateEnableState() override { updateEnableState(PAUSE); }

protected:
  enum Type
  {
    CONTINUE,
    PAUSE
  };

  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { this->setPauseSimulation(true); }

  void setPauseSimulation(bool pause);
  void updateEnableState(Type type);

private:
  Q_DISABLE_COPY(pqCatalystPauseSimulationReaction)
};

#endif
