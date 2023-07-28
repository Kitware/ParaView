// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqFiltersMenuReaction_h
#define pqFiltersMenuReaction_h

#include <QObject>

#include "pqApplicationComponentsModule.h"
#include "pqTimer.h"

class pqPipelineSource;
class pqProxyGroupMenuManager;

/**
 * @ingroup Reactions
 * Reaction to handle creation of filters from the filters menu.
 * pqFiltersMenuReaction knows when to enable/disable actions in the menu as
 * well as what to do when an action is triggered.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqFiltersMenuReaction : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqFiltersMenuReaction(pqProxyGroupMenuManager* menuManager, bool hideDisabledActions = false);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Updates the enabled state.  The actions in toolbars will
   * be updated automatically.  The containing widget of other actions
   * should connect its aboutToShow signal to this slot.
   */
  virtual void updateEnableState(bool updateOnlyToolbars = false);

  /**
   * Creates a filter of the given type.
   */
  static pqPipelineSource* createFilter(const QString& group, const QString& name);

protected Q_SLOTS:
  /**
   * Called when the action is triggered.
   */
  virtual void onTriggered(const QString& group, const QString& name)
  {
    pqFiltersMenuReaction::createFilter(group, name);
  }
  virtual void setEnableStateDirty();

private:
  Q_DISABLE_COPY(pqFiltersMenuReaction)

  pqTimer Timer;
  bool IsDirty;
  bool HideDisabledActions;
};

#endif
