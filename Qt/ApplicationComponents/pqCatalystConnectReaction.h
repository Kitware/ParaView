// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCatalystConnectReaction_h
#define pqCatalystConnectReaction_h

#include "pqReaction.h"

#include "vtkSmartPointer.h"

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

  /**
   * Disconnect to Catalyst
   */
  bool disconnect();

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override;

  /**
   * reaction disabled when already in collaboration mode. Update the action named to
   * Connect/Disconnect.
   */
  void updateEnableState() override;

private:
  Q_DISABLE_COPY(pqCatalystConnectReaction)

  bool IsEstablished = false;
};

#endif
