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
#include "pqDataRepresentation.h"
#include "pqLiveInsituManager.h"
#include "pqPipelineFilter.h"
#include "pqPropertiesPanel.h"
#include "pqProxyModifiedStateUndoElement.h"
#include "pqRenderView.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "vtkDataObject.h"
#include "vtkNew.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGeneralSettings.h"
#include "vtkSMAnimationSceneProxy.h"
#include "vtkSMLiveInsituLinkProxy.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSession.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTrace.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkSMViewProxy.h"
#include "vtkWeakPointer.h"

#include <QList>
#include <QSet>
#include <QtDebug>

class pqApplyBehavior::pqInternals
{
public:
  typedef QPair<vtkWeakPointer<vtkSMRepresentationProxy>, vtkWeakPointer<vtkSMViewProxy> > PairType;
  QList<PairType> NewlyCreatedRepresentations;
};

//-----------------------------------------------------------------------------
pqApplyBehavior::pqApplyBehavior(QObject* parentObject)
  : Superclass(parentObject)
  , Internals(new pqApplyBehavior::pqInternals())
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
  pqPipelineSource* pqsource = qobject_cast<pqPipelineSource*>(pqproxy);
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
    pqProxyModifiedStateUndoElement* undoElement = pqProxyModifiedStateUndoElement::New();
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
  //---------------------------------------------------------------------------
  // Update animation timesteps.
  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
  vtkSMAnimationSceneProxy::UpdateAnimationUsingDataTimeSteps(
    controller->GetAnimationScene(pqActiveObjects::instance().activeServer()->session()));

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();

  //---------------------------------------------------------------------------
  // If there is a catalyst session, push its updates to the server
  foreach (pqServer* server, smmodel->findItems<pqServer*>())
  {
    if (pqLiveInsituManager::isInsituServer(server))
    {
      pqLiveInsituManager::linkProxy(server)->PushUpdatedStates();
    }
  }

  QList<pqView*> dirty_views;

  //---------------------------------------------------------------------------
  // find views that need updating and update them.
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
    SM_SCOPED_TRACE(CallMethod)
      .arg(view->getViewProxy())
      .arg("Update")
      .arg("comment", "update the view to ensure updated data information");
    view->getViewProxy()->Update();
  }

  vtkPVGeneralSettings* gsettings = vtkPVGeneralSettings::GetInstance();
  foreach (const pqInternals::PairType& pair, this->Internals->NewlyCreatedRepresentations)
  {
    vtkSMRepresentationProxy* reprProxy = pair.first;
    vtkSMViewProxy* viewProxy = pair.second;

    // If not scalar coloring, we make an attempt to color using
    // 'vtkBlockColors' array, if present.
    if (vtkSMPVRepresentationProxy::SafeDownCast(reprProxy) &&
      vtkSMPVRepresentationProxy::GetUsingScalarColoring(reprProxy) == false &&
      reprProxy->GetRepresentedDataInformation()->GetArrayInformation(
        "vtkBlockColors", vtkDataObject::FIELD) != NULL &&
      reprProxy->GetRepresentedDataInformation()->GetNumberOfBlockLeafs(false) > 1)
    {
      vtkSMPVRepresentationProxy::SetScalarColoring(
        reprProxy, "vtkBlockColors", vtkDataObject::FIELD);
      if (gsettings->GetScalarBarMode() ==
        vtkPVGeneralSettings::AUTOMATICALLY_SHOW_AND_HIDE_SCALAR_BARS)
      {
        vtkSMPVRepresentationProxy::SetScalarBarVisibility(reprProxy, viewProxy, true);
      }
    }
  }

  //---------------------------------------------------------------------------
  // If user chose it, update all transfer function data range.
  // FIXME: This should happen for all servers available.
  vtkNew<vtkSMTransferFunctionManager> tmgr;
  int mode = gsettings->GetTransferFunctionResetMode();
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
        /*extend*/ true);
      break;
  }

  //---------------------------------------------------------------------------
  // Perform the render on visible views.
  foreach (pqView* view, dirty_views)
  {
    if (view->widget()->isVisible())
    {
      view->forceRender();
    }
  }

  this->Internals->NewlyCreatedRepresentations.clear();
}

//-----------------------------------------------------------------------------
void pqApplyBehavior::showData(pqPipelineSource* source, pqView* view)
{
  // HACK: Skip catalyst proxies.
  if (source->getServer()->getResource().scheme() == "catalyst")
  {
    return;
  }

  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();

  vtkSMViewProxy* currentViewProxy = view ? view->getViewProxy() : NULL;

  QSet<vtkSMProxy*> updated_views;

  // create representations for all output ports.
  for (int outputPort = 0; outputPort < source->getNumberOfOutputPorts(); outputPort++)
  {
    vtkSMViewProxy* preferredView = controller->ShowInPreferredView(
      vtkSMSourceProxy::SafeDownCast(source->getProxy()), outputPort, currentViewProxy);
    if (!preferredView)
    {
      continue;
    }

    updated_views.insert(preferredView);

    // reset camera if this is the only visible dataset.
    pqView* pqPreferredView = smmodel->findItem<pqView*>(preferredView);
    Q_ASSERT(pqPreferredView);
    if (preferredView != currentViewProxy)
    {
      // implying a new view was created, always reset that.
      pqPreferredView->resetDisplay();
    }
    else if (view && view->getNumberOfVisibleDataRepresentations() == 1)
    {
      // old view is being used, reset only if this is the only representation.
      view->resetDisplay();
    }

    // reset interaction mode for render views. Not a huge fan, but we'll fix
    // this some other time.
    if (pqRenderView* rview = qobject_cast<pqRenderView*>(pqPreferredView))
    {
      if (rview->getNumberOfVisibleDataRepresentations() == 1)
      {
        rview->updateInteractionMode(source->getOutputPort(outputPort));
      }
    }

    if (preferredView == currentViewProxy)
    {
      // Hide input, since the data wasn't shown in a new view, but an existing
      // view.
      if (pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(source))
      {
        this->hideInputIfRequired(filter, view);
      }
    }

    vtkSMRepresentationProxy* reprProxy =
      preferredView->FindRepresentation(source->getSourceProxy(), outputPort);
    // show scalar bar, if applicable.
    vtkPVGeneralSettings* gsettings = vtkPVGeneralSettings::GetInstance();
    if (gsettings->GetScalarBarMode() ==
      vtkPVGeneralSettings::AUTOMATICALLY_SHOW_AND_HIDE_SCALAR_BARS)
    {
      if (vtkSMPVRepresentationProxy::GetUsingScalarColoring(reprProxy))
      {
        vtkSMPVRepresentationProxy::SetScalarBarVisibility(reprProxy, preferredView, true);
      }
    }

    // Save the newly created representation for further fine-tuning in
    // pqApplyBehavior::applied().
    this->Internals->NewlyCreatedRepresentations.push_back(
      pqInternals::PairType(reprProxy, preferredView));
  }
}

//-----------------------------------------------------------------------------
void pqApplyBehavior::hideInputIfRequired(pqPipelineFilter* filter, pqView* view)
{
  int replace_input = filter->replaceInput();
  if (replace_input > 0)
  {
    vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;

    // hide input source.
    QList<pqOutputPort*> inputs = filter->getAllInputs();
    foreach (pqOutputPort* input, inputs)
    {
      pqDataRepresentation* inputRepr = input->getRepresentation(view);
      if (inputRepr)
      {
        if (replace_input == 2)
        {
          // Conditionally turn off the input. The input should be turned
          // off if the representation is surface and the opacity is 1.
          QString reprType =
            vtkSMPropertyHelper(inputRepr->getProxy(), "Representation", /*quiet=*/true)
              .GetAsString();
          double opacity =
            vtkSMPropertyHelper(inputRepr->getProxy(), "Opacity", /*quiet=*/true).GetAsDouble();
          if ((reprType != "Surface" && reprType != "Surface With Edges") ||
            (opacity != 0.0 && opacity < 1.0))
          {
            continue;
          }
        }
        // we use the controller API so that the scalar bars are updated as
        // needed.
        controller->SetVisibility(
          input->getSourceProxy(), input->getPortNumber(), view->getViewProxy(), false);
      }
    }
  }
}
