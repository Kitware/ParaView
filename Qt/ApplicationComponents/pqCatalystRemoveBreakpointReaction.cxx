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
#include "pqCatalystRemoveBreakpointReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqLiveInsituManager.h"
#include "pqLiveInsituVisualizationManager.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "vtkProcessModule.h"
#include "vtkSMLiveInsituLinkProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSession.h"

#include <QInputDialog>
#include <QMessageBox>

//-----------------------------------------------------------------------------
pqCatalystRemoveBreakpointReaction::pqCatalystRemoveBreakpointReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(parentObject->parent(), SIGNAL(aboutToShow()), this, SLOT(updateEnableState()));
}

//-----------------------------------------------------------------------------
void pqCatalystRemoveBreakpointReaction::onTriggered()
{
  pqLiveInsituManager::instance()->removeBreakpoint();
}

//-----------------------------------------------------------------------------
void pqCatalystRemoveBreakpointReaction::updateEnableState()
{
  pqLiveInsituManager* server = pqLiveInsituManager::instance();
  this->parentAction()->setEnabled(
    (server->linkProxy() && server->breakpointTime() != pqLiveInsituManager::INVALID_TIME) ? true
                                                                                           : false);
}
