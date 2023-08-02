// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
