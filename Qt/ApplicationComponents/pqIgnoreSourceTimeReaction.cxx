/*=========================================================================

   Program: ParaView
   Module:    pqIgnoreSourceTimeReaction.cxx

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
#include "pqIgnoreSourceTimeReaction.h"

#include "pqActiveObjects.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqTimeKeeper.h"
#include "pqUndoStack.h"
#include "vtkSMTimeKeeperProxy.h"

//-----------------------------------------------------------------------------
pqIgnoreSourceTimeReaction::pqIgnoreSourceTimeReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  parentObject->setCheckable(true);

  QObject::connect(&pqActiveObjects::instance(), SIGNAL(sourceChanged(pqPipelineSource*)), this,
    SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqIgnoreSourceTimeReaction::updateEnableState()
{
  if (!pqActiveObjects::instance().activeSource())
  {
    this->parentAction()->setEnabled(false);
    return;
  }

  // Decide enable state as well as check state for the action.
  QAction* action = this->parentAction();
  bool prev = action->blockSignals(true);
  bool enabled = true;
  bool checked = false;

  // Now determine the check state for the action.
  pqPipelineSource* source = pqActiveObjects::instance().activeSource();
  if (!source)
  {
    enabled = false;
  }
  else
  {
    pqTimeKeeper* timeKeeper = source->getServer()->getTimeKeeper();
    checked = checked ||
      // "checked" when the source proxy is not being tracked.
      !vtkSMTimeKeeperProxy::IsTimeSourceTracked(timeKeeper->getProxy(), source->getProxy());
  }
  action->setChecked(checked);
  action->blockSignals(prev);
  action->setEnabled(enabled);
}

//-----------------------------------------------------------------------------
void pqIgnoreSourceTimeReaction::ignoreSourceTime(bool ignore)
{
  BEGIN_UNDO_SET("Toggle Ignore Time");
  pqPipelineSource* source = pqActiveObjects::instance().activeSource();
  if (source)
  {
    pqIgnoreSourceTimeReaction::ignoreSourceTime(source, ignore);
  }
  END_UNDO_SET();
}

//-----------------------------------------------------------------------------
void pqIgnoreSourceTimeReaction::ignoreSourceTime(pqPipelineSource* source, bool ignore)
{
  if (!source)
  {
    return;
  }

  pqTimeKeeper* timeKeeper = source->getServer()->getTimeKeeper();
  vtkSMTimeKeeperProxy::SetSuppressTimeSource(timeKeeper->getProxy(), source->getProxy(), ignore);
}
