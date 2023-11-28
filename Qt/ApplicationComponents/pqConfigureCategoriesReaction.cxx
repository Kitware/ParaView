// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqConfigureCategoriesReaction.h"

#include "pqConfigureCategoriesDialog.h"
#include "pqCoreUtilities.h"

//-----------------------------------------------------------------------------
void pqConfigureCategoriesReaction::configureCategories(pqProxyGroupMenuManager* manager)
{
  pqConfigureCategoriesDialog dialog(manager, pqCoreUtilities::mainWidget());
  dialog.setObjectName("ConfigureCategoriesDialog");
  dialog.exec();
}
