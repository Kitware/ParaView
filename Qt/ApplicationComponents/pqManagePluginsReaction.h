// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqManagePluginsReaction_h
#define pqManagePluginsReaction_h

#include "pqMasterOnlyReaction.h"

/**
 * @ingroup Reactions
 * pqManagePluginsReaction is the reaction to pop-up the plugins manager dialog.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqManagePluginsReaction : public pqMasterOnlyReaction
{
  Q_OBJECT
  typedef pqMasterOnlyReaction Superclass;

public:
  pqManagePluginsReaction(QAction* action)
    : Superclass(action)
  {
  }

  /**
   * Pops-up the pqPluginDialog dialog.
   */
  static void managePlugins();

protected:
  void onTriggered() override { pqManagePluginsReaction::managePlugins(); }

private:
  Q_DISABLE_COPY(pqManagePluginsReaction)
};

#endif
