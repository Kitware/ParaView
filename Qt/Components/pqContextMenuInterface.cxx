// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
