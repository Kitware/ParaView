// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqRepresentationBehaviorStarter.h"

// Server Manager Includes.

// Qt Includes.
#include <QtDebug>
// ParaView Includes.
#include "pqSurfaceRepresentationBehavior.h"

//-----------------------------------------------------------------------------
pqRepresentationBehaviorStarter::pqRepresentationBehaviorStarter(QObject* parent)
  : QObject(parent)
{
}

//-----------------------------------------------------------------------------
pqRepresentationBehaviorStarter::~pqRepresentationBehaviorStarter() = default;

//-----------------------------------------------------------------------------
void pqRepresentationBehaviorStarter::onStartup()
{
  // Create the RepresentationBehavior
  new pqSurfaceRepresentationBehavior(this);
}

//-----------------------------------------------------------------------------
void pqRepresentationBehaviorStarter::onShutdown() {}
