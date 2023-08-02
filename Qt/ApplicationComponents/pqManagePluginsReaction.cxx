// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqManagePluginsReaction.h"

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqPluginDialog.h"

//-----------------------------------------------------------------------------
void pqManagePluginsReaction::managePlugins()
{
  pqPluginDialog dialog(pqActiveObjects::instance().activeServer(), pqCoreUtilities::mainWidget());
  dialog.setObjectName("PluginManagerDialog");
  dialog.exec();
}
