// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqLockPanelsReaction_h
#define pqLockPanelsReaction_h

#include "pqReaction.h"

class pqLockPanelsBehavior;
class QMainWindow;

/**
 * @ingroup Reactions
 * Reaction to toggle locking of dockable panels.
 * Note: For this reaction to have any effect on the dockable panels,
 * a pqLockPanelsBehavior must be instantiated.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqLockPanelsReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqLockPanelsReaction(QAction* action);
  ~pqLockPanelsReaction() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void actionTriggered();

private:
  Q_DISABLE_COPY(pqLockPanelsReaction)
};

#endif // pqLockPanelsReaction_h
