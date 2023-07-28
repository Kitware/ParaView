// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqMasterOnlyReaction.h"

//-----------------------------------------------------------------------------
pqMasterOnlyReaction::pqMasterOnlyReaction(QAction* parentObject)
  : Superclass(parentObject)
{
}
//-----------------------------------------------------------------------------
pqMasterOnlyReaction::pqMasterOnlyReaction(QAction* parentObject, Qt::ConnectionType type)
  : Superclass(parentObject, type)
{
}

//-----------------------------------------------------------------------------
void pqMasterOnlyReaction::updateEnableState()
{
  if (this->parentAction())
  {
    this->parentAction()->setEnabled(this->IsMaster);
  }
}
