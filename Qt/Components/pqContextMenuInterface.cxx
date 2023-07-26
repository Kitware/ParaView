// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#include "pqContextMenuInterface.h"

//-----------------------------------------------------------------------------
pqContextMenuInterface::pqContextMenuInterface() = default;

//-----------------------------------------------------------------------------
pqContextMenuInterface::~pqContextMenuInterface() = default;

//-----------------------------------------------------------------------------
bool pqContextMenuInterface::contextMenu(
  QMenu*, pqView*, const QPoint&, pqRepresentation*, const QList<unsigned int>&) const
{
  return false;
}

//-----------------------------------------------------------------------------
bool pqContextMenuInterface::contextMenu(
  QMenu*, pqView*, const QPoint&, pqRepresentation*, const QStringList&) const
{
  return false;
}
