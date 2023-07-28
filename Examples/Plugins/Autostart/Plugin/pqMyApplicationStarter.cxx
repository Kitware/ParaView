// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqMyApplicationStarter.h"

// Server Manager Includes.

// Qt Includes.
#include <QtDebug>
// ParaView Includes.

//-----------------------------------------------------------------------------
pqMyApplicationStarter::pqMyApplicationStarter(QObject* p /*=0*/)
  : QObject(p)
{
}

//-----------------------------------------------------------------------------
pqMyApplicationStarter::~pqMyApplicationStarter() = default;

//-----------------------------------------------------------------------------
void pqMyApplicationStarter::onStartup()
{
  qWarning() << "Message from pqMyApplicationStarter: Application Started";
}

//-----------------------------------------------------------------------------
void pqMyApplicationStarter::onShutdown()
{
  qWarning() << "Message from pqMyApplicationStarter: Application Shutting down";
}
