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
#include "pqCatalystSetBreakpointReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqLiveInsituManager.h"
#include "pqLiveInsituVisualizationManager.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSetBreakpointDialog.h"
#include "vtkProcessModule.h"
#include "vtkSMLiveInsituLinkProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSession.h"

#include <QInputDialog>
#include <QMessageBox>

class pqCatalystSetBreakpointReaction::sbrInternal
{
public:
  sbrInternal(QWidget* parent)
    : Dialog(parent)
  {
  }
  pqSetBreakpointDialog Dialog;
};

//-----------------------------------------------------------------------------
pqCatalystSetBreakpointReaction::pqCatalystSetBreakpointReaction(QAction* parentObject)
  : Superclass(parentObject)
  , Internal(new sbrInternal(pqCoreUtilities::mainWidget()))
{
  QObject::connect(parentObject->parent(), SIGNAL(aboutToShow()), this, SLOT(updateEnableState()));
}

//-----------------------------------------------------------------------------
void pqCatalystSetBreakpointReaction::onTriggered()
{
  // we set the breakpoint time inside the dialog
  this->Internal->Dialog.exec();
}

//-----------------------------------------------------------------------------
void pqCatalystSetBreakpointReaction::updateEnableState()
{
  this->parentAction()->setEnabled(pqLiveInsituManager::instance()->linkProxy() ? true : false);
}
