// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqMainWindowEventManager.h"

#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QShowEvent>

//-----------------------------------------------------------------------------
pqMainWindowEventManager::pqMainWindowEventManager(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqMainWindowEventManager::~pqMainWindowEventManager() = default;

//-----------------------------------------------------------------------------
void pqMainWindowEventManager::closeEvent(QCloseEvent* event)
{
  this->Closing = true;
  Q_EMIT this->close(event);
}

//-----------------------------------------------------------------------------
void pqMainWindowEventManager::showEvent(QShowEvent* event)
{
  Q_EMIT this->show(event);
}

//-----------------------------------------------------------------------------
void pqMainWindowEventManager::dragEnterEvent(QDragEnterEvent* event)
{
  Q_EMIT this->dragEnter(event);
}

//-----------------------------------------------------------------------------
void pqMainWindowEventManager::dropEvent(QDropEvent* event)
{
  Q_EMIT this->drop(event);
}
