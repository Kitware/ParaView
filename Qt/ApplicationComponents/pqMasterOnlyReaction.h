// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqMasterOnlyReaction_h
#define pqMasterOnlyReaction_h

#include "pqApplicationComponentsModule.h"
#include "pqReaction.h"
#include <QAction>

/**
 * @defgroup Reactions ParaView Reactions
 * ParaView client relies of a collection of reactions that autonomous entities
 * that use pqApplicationCore and other core components to get their work done which
 * keeping track for their own enabled state without any external input. To
 * use, simple create this reaction and assign it to a menu
 * and add it to menus/toolbars whatever.
 */

/**
 * @ingroup Reactions
 * This is a superclass just to make it easier to collect all such reactions.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqMasterOnlyReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqMasterOnlyReaction(QAction* parentObject);
  pqMasterOnlyReaction(QAction* parentObject, Qt::ConnectionType type);

protected Q_SLOTS:
  void updateEnableState() override;

private:
  Q_DISABLE_COPY(pqMasterOnlyReaction)
};

#endif
