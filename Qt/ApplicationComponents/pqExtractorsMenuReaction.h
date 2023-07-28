// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqExtractorsMenuReaction_h
#define pqExtractorsMenuReaction_h

#include "pqApplicationComponentsModule.h" // for exports
#include "pqTimer.h"                       // for pqTimer
#include <QObject>

class pqExtractor;
class pqProxyGroupMenuManager;

class PQAPPLICATIONCOMPONENTS_EXPORT pqExtractorsMenuReaction : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqExtractorsMenuReaction(pqProxyGroupMenuManager* menuManager, bool hideDisabledActions = false);
  ~pqExtractorsMenuReaction() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Updates the enabled state for actions in the menu assigned to the
   * menuManager.
   */
  void updateEnableState(bool updateOnlyToolbars = false);

  /**
   * Creates an extract generator.
   */
  pqExtractor* createExtractor(const QString& group, const QString& name) const;

private:
  Q_DISABLE_COPY(pqExtractorsMenuReaction);
  bool HideDisabledActions;
  pqTimer Timer;
};

#endif
