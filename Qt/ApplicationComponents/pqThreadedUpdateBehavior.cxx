// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqThreadedUpdateBehavior.h"

#include "pqApplicationCore.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "vtkPVSessionCore.h"
#include "vtkSMSession.h"

#include <QEventLoop>

//-----------------------------------------------------------------------------
pqThreadedUpdateBehavior::pqThreadedUpdateBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(serverAdded(pqServer*)), this, SLOT(onServerCreation(pqServer*)));
}

//-----------------------------------------------------------------------------
void pqThreadedUpdateBehavior::onServerCreation(pqServer* server)
{
  this->VTKConnector->Connect(server->session()->GetSessionCore(),
    vtkPVSessionBase::ThreadedStreamStartEvent, this, SLOT(loop()));
  this->VTKConnector->Connect(server->session()->GetSessionCore(),
    vtkPVSessionBase::ThreadedStreamEndEvent, this, SIGNAL(endLoop()));
}

//-----------------------------------------------------------------------------
void pqThreadedUpdateBehavior::loop()
{
  QEventLoop qloop;
  QObject::connect(
    this, &pqThreadedUpdateBehavior::endLoop, &qloop, &QEventLoop::quit, Qt::QueuedConnection);
  qloop.exec();
}
