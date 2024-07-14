// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

// Hide PARAVIEW_DEPRECATED_IN_5_13_0() warnings for this class.
#define PARAVIEW_DEPRECATION_LEVEL 0

#include "pqManageFavoritesReaction.h"

#include "pqCoreUtilities.h"
#include "pqFavoritesDialog.h"
#include "pqProxyGroupMenuManager.h"

#include <QAction>
#include <QVariant>

//-----------------------------------------------------------------------------
void pqManageFavoritesReaction::manageFavorites(pqProxyGroupMenuManager* manager)
{

  QVariantList data;
  for (QAction* action : manager->actions())
  {
    QStringList proxyStrings = action->data().toStringList();
    proxyStrings[0] = action->text();
    // data contains 'label , proxyName'
    data << proxyStrings;
  }

  pqFavoritesDialog dialog(data, pqCoreUtilities::mainWidget());
  dialog.setObjectName("FavoritesManagerDialog");
  dialog.exec();
}
