// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqIgnoreSourceTimeReaction_h
#define pqIgnoreSourceTimeReaction_h

#include "pqReaction.h"

class pqPipelineSource;

/**
 * @ingroup Reactions
 * Reaction for ignoring a source's time information for animations etc.
 * It affects all selected sources.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqIgnoreSourceTimeReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqIgnoreSourceTimeReaction(QAction* parent);

  /**
   * Ignore time from all selected sources.
   */
  static void ignoreSourceTime(bool ignore);

  /**
   * Ignore time for the given source.
   */
  static void ignoreSourceTime(pqPipelineSource*, bool ignore);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Updates the enabled state. Applications need not explicitly call this.
   */
  void updateEnableState() override;

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override
  {
    pqIgnoreSourceTimeReaction::ignoreSourceTime(this->parentAction()->isChecked());
  }

private:
  Q_DISABLE_COPY(pqIgnoreSourceTimeReaction)
};

#endif
