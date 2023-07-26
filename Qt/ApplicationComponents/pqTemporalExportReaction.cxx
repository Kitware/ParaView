// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#include "pqTemporalExportReaction.h"

#include <QtDebug>

//-----------------------------------------------------------------------------
pqTemporalExportReaction::pqTemporalExportReaction(QAction* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqTemporalExportReaction::~pqTemporalExportReaction() = default;

//-----------------------------------------------------------------------------
void pqTemporalExportReaction::onTriggered()
{
  qCritical("pqTemporalExportReaction is not functional for the time being");
}
