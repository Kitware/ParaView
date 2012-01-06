/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqCatalystConnectReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqObjectBuilder.h"
#include "pqServer.h"
#include "vtkSmartPointer.h"
#include "vtkSMLiveInsituLinkProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSession.h"

#include <QDockWidget>

//-----------------------------------------------------------------------------
pqCatalystConnectReaction::pqCatalystConnectReaction(QAction* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqCatalystConnectReaction::~pqCatalystConnectReaction()
{
}

//-----------------------------------------------------------------------------
bool pqCatalystConnectReaction::connect()
{
  pqServer* server = pqActiveObjects::instance().activeServer();

  vtkSMProxy* proxy =
    pqApplicationCore::instance()->getObjectBuilder()->createProxy(
      "coprocessing", "LiveInsituLink", server, "coprocessing");
  vtkSMLiveInsituLinkProxy* adaptor =
    vtkSMLiveInsituLinkProxy::SafeDownCast(proxy);
  if (!adaptor)
    {
    qCritical("Current VisualizationSession cannot create LiveInsituLink.");
    return false;
    }

  vtkSMPropertyHelper(adaptor, "InsituPort").Set(22222);
  vtkSMPropertyHelper(adaptor, "ProcessType").Set("Visualization");
  adaptor->UpdateVTKObjects();

  // create a new "server session" that acts as the dummy session representing
  // the insitu viz pipeline.
  pqServer* catalyst = pqApplicationCore::instance()->getObjectBuilder()->createServer(
    pqServerResource("builtin:"));
  catalyst->setResource(pqServerResource("catalyst:"));
  adaptor->SetInsituProxyManager(catalyst->proxyManager());
  catalyst->setMonitorServerNotifications(true);

  adaptor->InvokeCommand("Initialize");

  // FIXME: setup listeners so that when activeServer dies, the associated
  // live-insitu connection is also destroyed.
  return true;
}
