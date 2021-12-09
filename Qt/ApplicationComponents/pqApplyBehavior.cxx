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
#include "pqObjectBuilder.h"
#include "pqPVApplicationCore.h"
#include "pqPipelineFilter.h"
#include "pqPropertiesPanel.h"
#include "pqProxyModifiedStateUndoElement.h"
#include "pqRenderView.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "vtkDataObject.h"
#include "vtkNew.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVCatalystChannelInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGeneralSettings.h"
#include "vtkSMAnimationSceneProxy.h"
#include "vtkSMLiveInsituLinkProxy.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTrace.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkSMViewProxy.h"
#include "vtkWeakPointer.h"

#include <QList>
#include <QSet>
#include <QtDebug>

#include <cassert>

class pqApplyBehavior::pqInternals
{
public:
  typedef QPair<vtkWeakPointer<vtkSMRepresentationProxy>, vtkWeakPointer<vtkSMViewProxy>> PairType;
  QList<PairType> NewlyCreatedRepresentations;
  pqTimer AutoApplyTimer;
  int PanelCount = 0;
  QMetaObject::Connection ShortcutApplyConnection;
};

//-----------------------------------------------------------------------------
pqApplyBehavior::pqApplyBehavior(QObject* parentObject)
  : Superclass(parentObject)
  , Internals(new pqApplyBehavior::pqInternals())
{
  // Connect auto apply timer to triggerApply signal
  this->Internals->AutoApplyTimer.setSingleShot(true);
  QObject::connect(
    &this->Internals->AutoApplyTimer, &pqTimer::timeout, this, &pqApplyBehavior::triggerApply);

  // Connect pqPVApplicationCore::triggerApply to triggerApply signal
  QObject::connect(pqPVApplicationCore::instance(), &pqPVApplicationCore::triggerApply, this,
    &pqApplyBehavior::triggerApply);

  // Connect the shortcut triggerApply signal to onApplied, used only if no panel are registered
  // later.
  this->Internals->ShortcutApplyConnection = QObject::connect(
    this, &pqApplyBehavior::triggerApply, this, QOverload<>::of(&pqApplyBehavior::onApplied));

  // Connect builder creation signals to onModified
  // to allow using auto apply without registered properties panels
  const auto* builder = pqApplicationCore::instance()->getObjectBuilder();
  QObject::connect(builder, &pqObjectBuilder::sourceCreated, this, &pqApplyBehavior::onModified);
  QObject::connect(builder, &pqObjectBuilder::filterCreated, this, &pqApplyBehavior::onModified);
}

//-----------------------------------------------------------------------------
pqApplyBehavior::~pqApplyBehavior() = default;

//-----------------------------------------------------------------------------
void pqApplyBehavior::registerPanel(pqPropertiesPanel* panel)
{
  assert(panel);

  // Disconnect to rely on panel behavior
  QObject::disconnect(this->Internals->ShortcutApplyConnection);

  // Connect both overload on applied to onApplied
  QObject::connect(panel, QOverload<pqProxy*>::of(&pqPropertiesPanel::applied), this,
    QOverload<pqProxy*>::of(&pqApplyBehavior::onApplied));
  QObject::connect(panel, QOverload<>::of(&pqPropertiesPanel::applied), this,
    QOverload<>::of(&pqApplyBehavior::onApplied));

  // Connect modified and resetDone
  QObject::connect(panel, &pqPropertiesPanel::modified, this, &pqApplyBehavior::onModified);
  QObject::connect(panel, &pqPropertiesPanel::resetDone, this, &pqApplyBehavior::onResetDone);

  // Connect triggerApply to the panel apply
  QObject::connect(this, &pqApplyBehavior::triggerApply, panel, &pqPropertiesPanel::apply);

  this->Internals->PanelCount++;
}

//-----------------------------------------------------------------------------
void pqApplyBehavior::unregisterPanel(pqPropertiesPanel* panel)
{
  assert(panel);
  this->Internals->PanelCount--;
  this->disconnect(panel);
  if (this->Internals->PanelCount == 0)
  {
    // If there is no panel, restore the behavior without panel
    this->Internals->ShortcutApplyConnection = QObject::connect(
      this, &pqApplyBehavior::triggerApply, this, QOverload<>::of(&pqApplyBehavior::onApplied));
  }
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
  this->applied();
}

//-----------------------------------------------------------------------------
void pqApplyBehavior::onModified()
{
  vtkPVGeneralSettings* generalSettings = vtkPVGeneralSettings::GetInstance();
  if (generalSettings->GetAutoApply())
  {
    this->Internals->AutoApplyTimer.start(generalSettings->GetAutoApplyDelay());
  }
}

//-----------------------------------------------------------------------------
void pqApplyBehavior::onResetDone()
{
  this->Internals->AutoApplyTimer.stop();
}

//-----------------------------------------------------------------------------
void pqApplyBehavior::applied(pqPropertiesPanel*, pqProxy* pqproxy)
{
  this->Internals->AutoApplyTimer.stop();
  if (pqproxy->modifiedState() == pqProxy::UNINITIALIZED)
  {
    if (auto pqsource = qobject_cast<pqPipelineSource*>(pqproxy))
    {
      // if this is first apply after creation, show the data in the view.
      this->showData(pqsource, pqActiveObjects::instance().activeView());
    }

    // add undo-element to ensure this state change happens when
    // undoing/redoing.
    pqProxyModifiedStateUndoElement* undoElement = pqProxyModifiedStateUndoElement::New();
    undoElement->SetSession(pqproxy->getServer()->session());
    undoElement->MadeUnmodified(pqproxy);
    ADD_UNDO_ELEM(undoElement);
    undoElement->Delete();
  }
  pqproxy->setModifiedState(pqProxy::UNMODIFIED);

  // Make sure filters menu enable state is updated
  Q_EMIT pqApplicationCore::instance()->forceFilterMenuRefresh();

  auto pqfilter = qobject_cast<pqPipelineFilter*>(pqproxy);
  auto pqsource = qobject_cast<pqPipelineSource*>(pqproxy);
  if (pqsource != nullptr && pqfilter == nullptr)
  {
    // if we have a dataset from a source that has a Catalyst channel name we now rename
    // the proxy to be the channel name if the user didn't modify the name already
    if (pqproxy->userModifiedSMName() == false)
    {
      vtkSMSourceProxy* proxy = pqsource->getSourceProxy();

      vtkNew<vtkPVCatalystChannelInformation> information;
      information->Initialize();

      // Ask the server to fill out the rest of the information:
      proxy->GatherInformation(information);

      std::string name = information->GetChannelName();
      if (!name.empty())
      {
        pqproxy->rename(name.c_str());
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqApplyBehavior::applied(pqPropertiesPanel* panel)
{
  this->Internals->AutoApplyTimer.stop();

  // Update all proxies if needed
  if (this->Internals->PanelCount == 0)
  {
    pqServerManagerModel* model = pqApplicationCore::instance()->getServerManagerModel();
    QList<pqPipelineSource*> sources =
      model->findItems<pqPipelineSource*>(pqActiveObjects::instance().activeServer());
    for (pqPipelineSource* source : sources)
    {
      this->applied(panel, source);
    }
  }

  //---------------------------------------------------------------------------
  // Update animation timesteps.
  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
  vtkSMAnimationSceneProxy::UpdateAnimationUsingDataTimeSteps(
    controller->GetAnimationScene(pqActiveObjects::instance().activeServer()->session()));

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();

  //---------------------------------------------------------------------------
  // If there is a catalyst session, push its updates to the server
  Q_FOREACH (pqServer* server, smmodel->findItems<pqServer*>())
  {
    if (pqLiveInsituManager::isInsituServer(server))
    {
      pqLiveInsituManager::linkProxy(server)->PushUpdatedStates();
    }
  }

  QList<pqView*> dirty_views;

  //---------------------------------------------------------------------------
  // find views that need updating and update them.
  Q_FOREACH (pqView* view, smmodel->findItems<pqView*>())
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
  Q_FOREACH (pqView* view, dirty_views)
  {
    SM_SCOPED_TRACE(CallMethod)
      .arg(view->getViewProxy())
      .arg("Update")
      .arg("comment", "update the view to ensure updated data information");
    view->getViewProxy()->Update();
  }

  vtkPVGeneralSettings* gsettings = vtkPVGeneralSettings::GetInstance();
  if (gsettings->GetColorByBlockColorsOnApply())
  {
    Q_FOREACH (const pqInternals::PairType& pair, this->Internals->NewlyCreatedRepresentations)
    {
      vtkSMRepresentationProxy* reprProxy = pair.first;
      vtkSMViewProxy* viewProxy = pair.second;

      // If not scalar coloring, we make an attempt to color using
      // 'vtkBlockColors' array, if present.
      if (vtkSMPVRepresentationProxy::SafeDownCast(reprProxy) &&
        vtkSMPVRepresentationProxy::GetUsingScalarColoring(reprProxy) == false)
      {
        auto dataInfo = reprProxy->GetRepresentedDataInformation();
        auto arrayInfo = dataInfo->GetArrayInformation("vtkBlockColors", vtkDataObject::FIELD);
        if (dataInfo->IsCompositeDataSet() && arrayInfo != nullptr &&
          arrayInfo->GetComponentRange(0)[1] > 0)
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
    }
  }

  //---------------------------------------------------------------------------
  // If user chose it, update all transfer function data range.
  // FIXME: This should happen for all servers available.
  vtkNew<vtkSMTransferFunctionManager> tmgr;
  tmgr->ResetAllTransferFunctionRangesUsingCurrentData(
    pqActiveObjects::instance().activeServer()->proxyManager(), false /*animating*/);

  //---------------------------------------------------------------------------
  // Perform the render on visible views.
  Q_FOREACH (pqView* view, dirty_views)
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

  vtkSMViewProxy* currentViewProxy = view ? view->getViewProxy() : nullptr;

  const auto& activeObjects = pqActiveObjects::instance();
  auto activeLayout = activeObjects.activeLayout();
  const auto location = activeObjects.activeLayoutLocation();

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
    // if new view was created, let's make sure it is assigned to a layout.
    controller->AssignViewToLayout(preferredView, activeLayout, location);
    updated_views.insert(preferredView);

    // if active layout changed, let's use that from this point on.
    activeLayout = activeObjects.activeLayout();

    // reset camera if this is the only visible dataset.
    pqView* pqPreferredView = smmodel->findItem<pqView*>(preferredView);
    assert(pqPreferredView);
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
    // @sa `pqPipelineBrowserWidget` for BUG #20521 fix.
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

    // If the currentViewProxy is undefined, then a new view has been created for
    // the first output port. Attempt to use it for the remaining output ports.
    if (currentViewProxy == nullptr)
    {
      currentViewProxy = preferredView;
    }
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
    Q_FOREACH (pqOutputPort* input, inputs)
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
