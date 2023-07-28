// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqVerifyRequiredPluginBehavior.h"

#include "pqApplicationCore.h"
#include "pqManagePluginsReaction.h"
#include "pqPluginManager.h"

//-----------------------------------------------------------------------------
pqVerifyRequiredPluginBehavior::pqVerifyRequiredPluginBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(pqApplicationCore::instance()->getPluginManager(),
    SIGNAL(requiredPluginsNotLoaded(pqServer*)), this, SLOT(requiredPluginsNotLoaded()));
}

//-----------------------------------------------------------------------------
void pqVerifyRequiredPluginBehavior::requiredPluginsNotLoaded()
{
  pqManagePluginsReaction::managePlugins();
}
