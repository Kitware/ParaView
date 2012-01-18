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
#include "pqDisplayPolicy.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "vtkSmartPointer.h"
#include "vtkSMLiveInsituLinkProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "pqLiveInsituVisualizationManager.h"

#include <QDockWidget>

namespace
{
  class pqCatalystDisplayPolicy : public pqDisplayPolicy
  {
  typedef pqDisplayPolicy Superclass;
  QPointer<pqServer> MainSession;
  QPointer<pqServer> CatalystSession;
  vtkWeakPointer<vtkSMLiveInsituLinkProxy> InsituLinkProxy;

public:
  pqCatalystDisplayPolicy(
    pqServer* session,
    pqServer* catalystSession,
    vtkSMLiveInsituLinkProxy* linkProxy,
    QObject* parentObject)
    : Superclass(parentObject),
    MainSession(session),
    CatalystSession(catalystSession),
    InsituLinkProxy(linkProxy)
  {
  }
  virtual ~pqCatalystDisplayPolicy() { }
  
  virtual VisibilityState getVisibility(pqView* view, pqOutputPort* port) const
    {
    if (port && port->getServer() == this->CatalystSession)
      {
      if (this->InsituLinkProxy->HasExtract(
          port->getSource()->getSMGroup().toAscii().data(),
          port->getSource()->getSMName().toAscii().data(),
          port->getPortNumber()))
        {
        return Visible;
        }

      return Hidden;
      }
    else
      {
      return this->Superclass::getVisibility(view, port);
      }
    }

  virtual pqDataRepresentation* setRepresentationVisibility(
    pqOutputPort* port, pqView* view, bool visible) const
    {
    if (port && port->getServer() == this->CatalystSession)
      {
      if (visible && 
        !this->InsituLinkProxy->HasExtract(
          port->getSource()->getSMGroup().toAscii().data(),
          port->getSource()->getSMName().toAscii().data(),
          port->getPortNumber()))
        {
        this->addExtract(port);
        }
      return NULL;
      }
    else
      {
      return this->Superclass::setRepresentationVisibility(port, view, visible);
      }
    }
protected:
  void addExtract(pqOutputPort* port) const
    {
    vtkSMProxy* proxy = this->InsituLinkProxy->CreateExtract(
      port->getSource()->getSMGroup().toAscii().data(),
      port->getSource()->getSMName().toAscii().data(),
      port->getPortNumber());

    this->MainSession->proxyManager()->RegisterProxy(
      "sources", QString("%1 (%2)").arg(port->getSource()->getSMName()).arg(
        port->getPortNumber()).toAscii().data(),
      proxy);

    //pqPipelineSource* pqproxy =
    //  pqApplicationCore::instance()->getServerManagerModel()->findItem<pqPipelineSource*>(proxy);
    //Q_ASSERT(pqproxy);
    //pqActiveObjects::instance().setActiveServer(pqproxy->getServer());
    //this->setRepresentationVisibility(
    //  pqproxy->getOutputPort(0),
    //  pqActiveObjects::instance().activeView(), true);
    }
  };
}

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

  pqLiveInsituVisualizationManager* mgr =
    new pqLiveInsituVisualizationManager(22222, server);

  return (mgr != NULL);
}
