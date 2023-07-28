// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCatalystSetBreakpointReaction_h
#define pqCatalystSetBreakpointReaction_h

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

class PQAPPLICATIONCOMPONENTS_EXPORT pqCatalystSetBreakpointReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqCatalystSetBreakpointReaction(QAction* parent = nullptr);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void updateEnableState() override;

protected:
  void onTriggered() override;

private:
  Q_DISABLE_COPY(pqCatalystSetBreakpointReaction)
  class sbrInternal;
  sbrInternal* Internal;
};

#endif
