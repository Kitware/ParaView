// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqReaction.h"

#include "pqApplicationCore.h"
#include "pqServer.h"

#include <cassert>

//-----------------------------------------------------------------------------
pqReaction::pqReaction(QAction* parentObject, Qt::ConnectionType type)
  : Superclass(parentObject)
{
  assert(parentObject != nullptr);

  QObject::connect(parentObject, SIGNAL(triggered(bool)), this, SLOT(onTriggered()), type);

  // Deal with master/slave enable/disable
  QObject::connect(pqApplicationCore::instance(), SIGNAL(updateMasterEnableState(bool)), this,
    SLOT(updateMasterEnableState(bool)));

  this->IsMaster = true;
}

//-----------------------------------------------------------------------------
pqReaction::~pqReaction() = default;

//-----------------------------------------------------------------------------
void pqReaction::updateMasterEnableState(bool isMaster)
{
  this->IsMaster = isMaster;
  updateEnableState();
}
