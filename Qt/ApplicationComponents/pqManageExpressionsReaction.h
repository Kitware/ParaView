// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqManageExpressionsReaction_h
#define pqManageExpressionsReaction_h

#include "pqMasterOnlyReaction.h"

class QAction;

/**
 * @ingroup Reactions
 * pqManageExpressionsReaction is the reaction to pop-up the expressions manager dialog.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqManageExpressionsReaction : public pqMasterOnlyReaction
{
  Q_OBJECT
  typedef pqMasterOnlyReaction Superclass;

public:
  pqManageExpressionsReaction(QAction* action);

  /**
   * Pops-up the pqExpressionsManager dialog.
   */
  static void manageExpressions();

protected:
  void onTriggered() override { pqManageExpressionsReaction::manageExpressions(); }

private:
  Q_DISABLE_COPY(pqManageExpressionsReaction)
};

#endif
