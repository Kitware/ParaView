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
pqRepresentationBehaviorStarter::pqRepresentationBehaviorStarter(QObject* p /*=0*/)
  : QObject(p)
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
