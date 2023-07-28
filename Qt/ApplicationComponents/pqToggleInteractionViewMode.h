// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqToggleInteractionViewMode_h
#define pqToggleInteractionViewMode_h

#include "pqReaction.h"
#include <QPointer>

class pqView;

/**
 * @ingroup Reactions
 * pqToggleInteractionViewMode is a reaction that toggle 2D/3D interaction mode
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqToggleInteractionViewMode : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqToggleInteractionViewMode(QAction* parent, pqView* view = nullptr);

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override;

public Q_SLOTS:
  void updateInteractionLabel(int interactionMode);

private:
  Q_DISABLE_COPY(pqToggleInteractionViewMode)
  QPointer<pqView> View;
};

#endif
