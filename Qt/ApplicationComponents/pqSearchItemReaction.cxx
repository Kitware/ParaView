// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSearchItemReaction.h"

#include "pqActiveObjects.h"
#include "pqPVApplicationCore.h"

#include <QAbstractItemView>
#include <QApplication>

//-----------------------------------------------------------------------------
pqSearchItemReaction::pqSearchItemReaction(QAction* renameAction)
  : Superclass(renameAction)
{
  this->connect(qApp, SIGNAL(focusChanged(QWidget*, QWidget*)), SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqSearchItemReaction::updateEnableState()
{
  this->Superclass::updateEnableState();
  this->parentAction()->setEnabled(
    qobject_cast<QAbstractItemView*>(QApplication::focusWidget()) != nullptr);
}

//-----------------------------------------------------------------------------
void pqSearchItemReaction::onTriggered()
{
  auto app = pqPVApplicationCore::instance();
  if (app)
  {
    app->startSearch();
  }
}
