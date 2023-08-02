// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCatalystConnectReaction_h
#define pqCatalystConnectReaction_h

#include "pqReaction.h"
#include <QPointer>

class pqLiveInsituVisualizationManager;

/**
 * Reaction for connecting to Catalyst CoProcessing Engine for Live-Data
 * Visualization.
 * @ingroup Reactions
 * @ingroup LiveInsitu
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCatalystConnectReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqCatalystConnectReaction(QAction* parent = nullptr);
  ~pqCatalystConnectReaction() override;

  /**
   * Connect to Catalyst
   */
  bool connect();

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { this->connect(); }

  /**
   * reaction disabled when already connected to a catalyst server or in
   * collaboration mode.
   */
  void updateEnableState() override;

private:
  Q_DISABLE_COPY(pqCatalystConnectReaction)
};

#endif
