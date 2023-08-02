// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqTemporalExportReaction_h
#define pqTemporalExportReaction_h

#include "pqApplicationComponentsModule.h"
#include "pqReaction.h"

/**
 * @ingroup Reactions
 * Reaction to export a script that will produce configured temporal data
 * products simultaneously. Each group of nodes in the MPI job will process a
 * different subset of the temporal domain. Within a group the nodes split
 * the data spatially as usual.
 */

class PQAPPLICATIONCOMPONENTS_EXPORT pqTemporalExportReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqTemporalExportReaction(QAction* parent);
  ~pqTemporalExportReaction() override;

protected Q_SLOTS:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override;

private:
  Q_DISABLE_COPY(pqTemporalExportReaction)
};

#endif
