// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSearchItemReaction_h
#define pqSearchItemReaction_h

#include "pqReaction.h"

#include <QPointer>

class pqProxy;
class QWidget;

/**
 * @ingroup Reactions
 * Reaction to filter item widgets.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqSearchItemReaction : pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqSearchItemReaction(QAction* filterAction);

protected Q_SLOTS:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override;
  void updateEnableState() override;
};

#endif
