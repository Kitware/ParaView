// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqManageLinksReaction_h
#define pqManageLinksReaction_h

#include "pqMasterOnlyReaction.h"

/**
 * @ingroup Reactions
 * pqManageLinksReaction is the reaction to pop-up the links manager dialog.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqManageLinksReaction : public pqMasterOnlyReaction
{
  Q_OBJECT
  typedef pqMasterOnlyReaction Superclass;

public:
  pqManageLinksReaction(QAction* action)
    : Superclass(action)
  {
  }

  /**
   * Pops-up the pqLinksManager dialog.
   */
  static void manageLinks();

protected:
  void onTriggered() override { pqManageLinksReaction::manageLinks(); }

private:
  Q_DISABLE_COPY(pqManageLinksReaction)
};

#endif
