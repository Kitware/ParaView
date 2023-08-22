// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqConfigureCategoriesReaction_h
#define pqConfigureCategoriesReaction_h

#include "pqMasterOnlyReaction.h"

class QAction;
class pqProxyGroupMenuManager;

/**
 * @ingroup Reactions
 * pqConfigureCategoriesReaction is the reaction to pop-up the categories manager dialog.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqConfigureCategoriesReaction : public pqMasterOnlyReaction
{
  Q_OBJECT
  typedef pqMasterOnlyReaction Superclass;

public:
  pqConfigureCategoriesReaction(QAction* action, pqProxyGroupMenuManager* manager)
    : Superclass(action)
    , Manager(manager)
  {
  }

  /**
   * Pops-up the pqConfigureCategoriesDialog dialog.
   */
  static void configureCategories(pqProxyGroupMenuManager* manager);

protected:
  void onTriggered() override { pqConfigureCategoriesReaction::configureCategories(this->Manager); }

private:
  Q_DISABLE_COPY(pqConfigureCategoriesReaction)

  pqProxyGroupMenuManager* Manager;
};

#endif
