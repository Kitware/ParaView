/*=========================================================================

   Program: ParaView
   Module:    pqCollaborationBehavior.cxx

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
#include "pqCollaborationBehavior.h"

#include "pqApplicationCore.h"
#include "pqActiveObjects.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqCollaborationManager.h"

#include "vtkSMSession.h"

//-----------------------------------------------------------------------------
pqCollaborationBehavior::pqCollaborationBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  this->CollaborationManager = NULL;
  QObject::connect( pqApplicationCore::instance()->getServerManagerModel(),
                    SIGNAL(preServerAdded(pqServer*)),
                    this, SLOT(onServerAdded(pqServer*)));
  QObject::connect( pqApplicationCore::instance()->getServerManagerModel(),
                    SIGNAL(aboutToRemoveServer(pqServer*)),
                    this, SLOT(onServerRemoved(pqServer*)));
}

//-----------------------------------------------------------------------------
void pqCollaborationBehavior::onServerAdded(pqServer* server)
{
  // Clean-up previous instance if needed
  this->onServerRemoved(NULL);

  if(server->session()->IsMultiClients())
    {
    this->CollaborationManager = new pqCollaborationManager(this);
    this->CollaborationManager->setServer(server);
    QObject::connect(this->CollaborationManager,
                     SIGNAL(triggeredMasterChanged(bool)),
                     pqApplicationCore::instance(),
                     SIGNAL(updateMasterEnableState(bool)));

    QObject::connect( pqApplicationCore::instance()->getServerManagerModel(),
                      SIGNAL(serverAdded(pqServer*)),
                      this->CollaborationManager, SLOT(updateEnabledState()),
                      Qt::QueuedConnection);

    // We attach mouse listener on active change time otherwise when the view
    // is just added, the widget is not yet correctly initialized...
    QObject::connect( &pqActiveObjects::instance(),
                      SIGNAL(viewChanged(pqView*)),
                      this->CollaborationManager, SLOT(attachMouseListenerTo3DViews()),
                      Qt::UniqueConnection);

    QObject::connect( pqApplicationCore::instance()->getServerManagerModel(),
                      SIGNAL(viewAdded(pqView*)),
                      this->CollaborationManager, SLOT(attachChartViewBoundsListener(pqView*)),
                      Qt::UniqueConnection);

    }
}

//-----------------------------------------------------------------------------
void pqCollaborationBehavior::onServerRemoved(pqServer* vtkNotUsed(server))
{
  if(this->CollaborationManager)
    {
    QObject::disconnect( this->CollaborationManager,
                         SIGNAL(triggeredMasterChanged(bool)),
                         pqApplicationCore::instance(),
                         SIGNAL(updateMasterEnableState(bool)));
    QObject::disconnect( pqApplicationCore::instance()->getServerManagerModel(),
                         SIGNAL(serverAdded(pqServer*)),
                         this->CollaborationManager, SLOT(updateEnabledState()));
    QObject::disconnect( &pqActiveObjects::instance(),
                         SIGNAL(viewChanged(pqView*)),
                         this->CollaborationManager, SLOT(attachMouseListenerTo3DViews()));
    QObject::disconnect( pqApplicationCore::instance()->getServerManagerModel(),
                         SIGNAL(viewAdded(pqView*)),
                         this->CollaborationManager, SLOT(attachChartViewBoundsListener(pqView*)));

    this->CollaborationManager->deleteLater();
    this->CollaborationManager = NULL;
    }
}
