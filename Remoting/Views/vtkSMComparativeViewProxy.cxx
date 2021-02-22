/*=========================================================================

  Program:   ParaView
  Module:    vtkSMComparativeViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMComparativeViewProxy.h"

#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVComparativeView.h"
#include "vtkPVSession.h"
#include "vtkSMProxyManager.h"
#include "vtkSMUndoStackBuilder.h"

#include <assert.h>

#define GET_PV_COMPARATIVE_VIEW() vtkPVComparativeView::SafeDownCast(this->GetClientSideObject())

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMComparativeViewProxy);
//----------------------------------------------------------------------------
vtkSMComparativeViewProxy::vtkSMComparativeViewProxy()
{
  this->SetLocation(vtkPVSession::CLIENT);
}

//----------------------------------------------------------------------------
vtkSMComparativeViewProxy::~vtkSMComparativeViewProxy() = default;

//----------------------------------------------------------------------------
void vtkSMComparativeViewProxy::Update()
{
  // Make sure we don't track in Undo/Redo the proxy update that we have set in
  // the comparative view
  vtkSMUndoStackBuilder* usb = vtkSMProxyManager::GetProxyManager()->GetUndoStackBuilder();
  if (usb)
  {
    bool prev = usb->GetIgnoreAllChanges();
    usb->SetIgnoreAllChanges(true);
    this->Superclass::Update();
    usb->SetIgnoreAllChanges(prev);
  }
  else
  {
    // Simply update
    this->Superclass::Update();
  }

  // I can't remember where is this flag coming from. Keeping it as it was for
  // now.
  this->NeedsUpdate = false;
}

//----------------------------------------------------------------------------
void vtkSMComparativeViewProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
  {
    return;
  }

  // If prototype do not setup subproxy location and do not send anything
  // to the server
  if (this->Location == 0)
  {
    this->Superclass::CreateVTKObjects();
    return;
  }

  this->GetSubProxy("RootView")->SetLocation(vtkPVSession::CLIENT_AND_SERVERS);
  this->Superclass::CreateVTKObjects();
  if (!this->ObjectsCreated)
  {
    return;
  }

  vtkSMViewProxy* rootView = vtkSMViewProxy::SafeDownCast(this->GetSubProxy("RootView"));
  if (!rootView)
  {
    vtkErrorMacro("Subproxy \"Root\" must be defined in the xml configuration.");
    return;
  }

  GET_PV_COMPARATIVE_VIEW()
    ->AddObserver(
      vtkCommand::ConfigureEvent, this, &vtkSMComparativeViewProxy::InvokeConfigureEvent);

  GET_PV_COMPARATIVE_VIEW()->Initialize(rootView);
}

//----------------------------------------------------------------------------
void vtkSMComparativeViewProxy::InvokeConfigureEvent()
{
  this->InvokeEvent(vtkCommand::ConfigureEvent);
}

//----------------------------------------------------------------------------
vtkSMViewProxy* vtkSMComparativeViewProxy::GetRootView()
{
  return GET_PV_COMPARATIVE_VIEW()->GetRootView();
}

//----------------------------------------------------------------------------
void vtkSMComparativeViewProxy::MarkDirty(vtkSMProxy* modifiedProxy)
{
  if (vtkSMViewProxy::SafeDownCast(modifiedProxy) == nullptr)
  {
    // cout << "vtkSMComparativeViewProxy::MarkDirty == " << modifiedProxy << endl;
    // The representation that gets added to this view is a consumer of it's
    // input. While this view is a consumer of the representation. So, when the
    // input source is modified, that call eventually leads to
    // vtkSMComparativeViewProxy::MarkDirty(). When that happens, we need to
    // ensure that we regenerate the comparison, so we call this->MarkOutdated().

    // TODO: We can be even smarter. We may want to try to consider only those
    // representations that are actually involved in the parameter comparison to
    // mark this view outdated. This will save on the regeneration of cache when
    // not needed.

    // TODO: Another optimization: we can enable caching only for those
    // representations that are invovled in parameter comparison, others we don't
    // even need to cache.

    // TODO: Need to update data ranges by collecting ranges from all views.
    GET_PV_COMPARATIVE_VIEW()->MarkOutdated();
  }
  this->Superclass::MarkDirty(modifiedProxy);
}

//----------------------------------------------------------------------------
const char* vtkSMComparativeViewProxy::GetRepresentationType(vtkSMSourceProxy* src, int outputport)
{
  return this->GetRootView()->GetRepresentationType(src, outputport);
}

//----------------------------------------------------------------------------
void vtkSMComparativeViewProxy::GetViews(vtkCollection* collection)
{
  if (!collection)
  {
    return;
  }

  GET_PV_COMPARATIVE_VIEW()->GetViews(collection);
}

//----------------------------------------------------------------------------
void vtkSMComparativeViewProxy::SetupInteractor(vtkRenderWindowInteractor*)
{
  vtkErrorMacro("vtkSMComparativeViewProxy doesn't support SetupInteractor. "
                "Please setup interactors on each of the internal views explicitly.");
}
//----------------------------------------------------------------------------
bool vtkSMComparativeViewProxy::MakeRenderWindowInteractor(bool quiet)
{
  bool flag = true;
  vtkNew<vtkCollection> views;
  this->GetViews(views.GetPointer());

  views->InitTraversal();
  for (vtkSMViewProxy* view = vtkSMViewProxy::SafeDownCast(views->GetNextItemAsObject());
       view != nullptr; view = vtkSMViewProxy::SafeDownCast(views->GetNextItemAsObject()))
  {
    flag = view->MakeRenderWindowInteractor(quiet) && flag;
  }
  return flag;
}

//----------------------------------------------------------------------------
vtkImageData* vtkSMComparativeViewProxy::CaptureWindowInternal(int magX, int magY)
{
  // This is needed to ensure that the views are laid out properly before trying
  // to capture images from each of them.
  this->Update();
  return GET_PV_COMPARATIVE_VIEW()->CaptureWindow(magX, magY);
}

//----------------------------------------------------------------------------
void vtkSMComparativeViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
