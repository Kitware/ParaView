// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCatalystRemoveBreakpointReaction_h
#define pqCatalystRemoveBreakpointReaction_h

#include "pqReaction.h"
#include <QPointer>

class pqLiveInsituVisualizationManager;
class vtkSMLiveInsituLinkProxy;

/**
 * @class pqCatalystRemoveBreakpointReaction
 * @brief  Reaction for setting a breakpoint to Catalyst CoProcessing Engine for Live-Data
 * Visualization.
 *
 * @ingroup Reactions
 * @ingroup LiveInsitu
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCatalystRemoveBreakpointReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqCatalystRemoveBreakpointReaction(QAction* parent = nullptr);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void updateEnableState() override;

protected:
  void onTriggered() override;

private:
  Q_DISABLE_COPY(pqCatalystRemoveBreakpointReaction)
};

#endif
