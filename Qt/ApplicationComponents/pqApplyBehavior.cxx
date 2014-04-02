/*=========================================================================

   Program: ParaView
   Module:  pqApplyBehavior.cxx

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
#include "pqApplyBehavior.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqDisplayPolicy.h"
#include "pqPipelineFilter.h"
#include "pqPropertiesPanel.h"
#include "pqProxyModifiedStateUndoElement.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "vtkNew.h"
#include "vtkPVGeneralSettings.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkSMViewProxy.h"

#include <QtDebug>

//-----------------------------------------------------------------------------
pqApplyBehavior::pqApplyBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqApplyBehavior::~pqApplyBehavior()
{
}

//-----------------------------------------------------------------------------
void pqApplyBehavior::registerPanel(pqPropertiesPanel* panel)
{
  Q_ASSERT(panel);

  this->connect(panel, SIGNAL(applied(pqProxy*)), SLOT(onApplied(pqProxy*)));
  this->connect(panel, SIGNAL(applied()), SLOT(onApplied()));
}

//-----------------------------------------------------------------------------
void pqApplyBehavior::unregisterPanel(pqPropertiesPanel* panel)
{
  Q_ASSERT(panel);
  this->disconnect(panel);
}

//-----------------------------------------------------------------------------
void pqApplyBehavior::onApplied(pqProxy* proxy)
{
  pqPropertiesPanel* panel = qobject_cast<pqPropertiesPanel*>(this->sender());
  if (panel)
    {
    this->applied(panel, proxy);
    }
}

//-----------------------------------------------------------------------------
void pqApplyBehavior::onApplied()
{
  pqPropertiesPanel* panel = qobject_cast<pqPropertiesPanel*>(this->sender());
  if (panel)
    {
    this->applied(panel);
    }
}

//-----------------------------------------------------------------------------
void pqApplyBehavior::applied(pqPropertiesPanel*, pqProxy* pqproxy)
{
  pqPipelineSource *pqsource = qobject_cast<pqPipelineSource*>(pqproxy);
  if (pqsource == NULL)
    {
    return;
    }

  Q_ASSERT(pqsource);

  if (pqsource->modifiedState() == pqProxy::UNINITIALIZED)
    {
    // if this is first apply after creation, show the data in the view.
    this->showData(pqsource, pqActiveObjects::instance().activeView());

    // add undo-element to ensure this state change happens when
    // undoing/redoing.
    pqProxyModifiedStateUndoElement* undoElement =
      pqProxyModifiedStateUndoElement::New();
    undoElement->SetSession(pqsource->getServer()->session());
    undoElement->MadeUnmodified(pqsource);
    ADD_UNDO_ELEM(undoElement);
    undoElement->Delete();
    }
  pqsource->setModifiedState(pqProxy::UNMODIFIED);
}

//-----------------------------------------------------------------------------
void pqApplyBehavior::applied(pqPropertiesPanel*)
{
  QList<pqView*> dirty_views;

  //---------------------------------------------------------------------------
  // find views that need updating and update them.
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  foreach (pqView* view, smmodel->findItems<pqView*>())
    {
    if (view && view->getViewProxy()->GetNeedsUpdate())
      {
      dirty_views.push_back(view);
      }
    }

  //---------------------------------------------------------------------------
  // Update all the views separately. This ensures that all pipelines are
  // up-to-date before we render as we may need to change some rendering
  // properties like color transfer functions before the actual render.
  foreach (pqView* view, dirty_views)
    {
    view->getViewProxy()->Update();
    }

  //---------------------------------------------------------------------------
  // If user chose it, update all transfer function data range.
  // FIXME: This should happen for all servers available.
  vtkNew<vtkSMTransferFunctionManager> tmgr;
  int mode = vtkPVGeneralSettings::GetInstance()->GetTransferFunctionResetMode();
  switch (mode)
    {
  case vtkPVGeneralSettings::RESET_ON_APPLY:
  case vtkPVGeneralSettings::RESET_ON_APPLY_AND_TIMESTEP:
    tmgr->ResetAllTransferFunctionRangesUsingCurrentData(
      pqActiveObjects::instance().activeServer()->proxyManager(), false);
    break;

  case vtkPVGeneralSettings::GROW_ON_APPLY:
  case vtkPVGeneralSettings::GROW_ON_APPLY_AND_TIMESTEP:
  default:
    tmgr->ResetAllTransferFunctionRangesUsingCurrentData(
      pqActiveObjects::instance().activeServer()->proxyManager(),
      /*extend*/true);
    break;
    }

  //---------------------------------------------------------------------------
  // Perform the render on visible views.
  foreach (pqView* view, dirty_views)
    {
    if (view->getWidget()->isVisible())
      {
      view->getViewProxy()->StillRender();
      }
    }
}

//-----------------------------------------------------------------------------
void pqApplyBehavior::showData(pqPipelineSource* source, pqView* view)
{
  pqDisplayPolicy *displayPolicy = pqApplicationCore::instance()->getDisplayPolicy();
  if (!displayPolicy)
    {
    qCritical() << "No display policy defined. Cannot create pending displays.";
    return;
    }

  // create representations for all output ports.
  for (int i = 0; i < source->getNumberOfOutputPorts(); i++)
    {
    pqDataRepresentation *repr = displayPolicy->createPreferredRepresentation(
      source->getOutputPort(i), view, false);
    if (!repr || !repr->getView())
      {
      continue;
      }

    pqView *reprView = repr->getView();
    pqPipelineFilter *filter = qobject_cast<pqPipelineFilter *>(source);
    if (filter)
      {
      filter->hideInputIfRequired(reprView);
      }
    }

  vtkNew<vtkSMTransferFunctionManager> tmgr;
  switch (vtkPVGeneralSettings::GetInstance()->GetScalarBarMode())
    {
  case vtkPVGeneralSettings::AUTOMATICALLY_SHOW_AND_HIDE_SCALAR_BARS:
    tmgr->UpdateScalarBars(view->getProxy(),
      (vtkSMTransferFunctionManager::HIDE_UNUSED_SCALAR_BARS |
       vtkSMTransferFunctionManager::SHOW_USED_SCALAR_BARS));
    break;

  case vtkPVGeneralSettings::AUTOMATICALLY_HIDE_SCALAR_BARS:
    tmgr->UpdateScalarBars(view->getProxy(),
      vtkSMTransferFunctionManager::HIDE_UNUSED_SCALAR_BARS);
    break;
    }
}
