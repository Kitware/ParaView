/*=========================================================================

  Program:   ParaView
  Module:    vtkSMOrthographicSliceViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMOrthographicSliceViewProxy.h"

#include "vtkBoundingBox.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVOrthographicSliceView.h"
#include "vtkSMInputProperty.h"
#include "vtkSMMultiSliceViewProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"

#include <cassert>

vtkStandardNewMacro(vtkSMOrthographicSliceViewProxy);
//----------------------------------------------------------------------------
vtkSMOrthographicSliceViewProxy::vtkSMOrthographicSliceViewProxy() = default;

//----------------------------------------------------------------------------
vtkSMOrthographicSliceViewProxy::~vtkSMOrthographicSliceViewProxy() = default;

//----------------------------------------------------------------------------
void vtkSMOrthographicSliceViewProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
  {
    return;
  }
  this->Superclass::CreateVTKObjects();
  if (!this->ObjectsCreated)
  {
    return;
  }

  vtkPVOrthographicSliceView* view =
    vtkPVOrthographicSliceView::SafeDownCast(this->GetClientSideObject());
  assert(view);

  view->AddObserver(vtkCommand::MouseWheelForwardEvent, this,
    &vtkSMOrthographicSliceViewProxy::OnMouseWheelForwardEvent);
  view->AddObserver(vtkCommand::MouseWheelBackwardEvent, this,
    &vtkSMOrthographicSliceViewProxy::OnMouseWheelBackwardEvent);
  view->AddObserver(
    vtkCommand::PlacePointEvent, this, &vtkSMOrthographicSliceViewProxy::OnPlacePointEvent);
}

//----------------------------------------------------------------------------
const char* vtkSMOrthographicSliceViewProxy::GetRepresentationType(
  vtkSMSourceProxy* producer, int outputPort)
{
  if (!producer)
  {
    return nullptr;
  }

  assert("Session should be valid" && this->GetSession());
  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();

  // Choose which type of representation proxy to create.
  vtkSMProxy* prototype =
    pxm->GetPrototypeProxy("representations", "CompositeOrthographicSliceRepresentation");
  vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(producer, outputPort);
  bool sg = (pp->IsInDomains() > 0);
  pp->RemoveAllUncheckedProxies();
  return sg ? "CompositeOrthographicSliceRepresentation"
            : this->Superclass::GetRepresentationType(producer, outputPort);
}

//----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMOrthographicSliceViewProxy::CreateDefaultRepresentation(
  vtkSMProxy* proxy, int outputPort)
{
  vtkSMRepresentationProxy* repr = this->Superclass::CreateDefaultRepresentation(proxy, outputPort);
  if (repr && strcmp(repr->GetXMLName(), "CompositeOrthographicSliceRepresentation") == 0)
  {
    this->InitDefaultSlices(vtkSMSourceProxy::SafeDownCast(proxy), outputPort, repr);
  }
  return repr;
}

//-----------------------------------------------------------------------------
void vtkSMOrthographicSliceViewProxy::InitDefaultSlices(
  vtkSMSourceProxy* source, int opport, vtkSMRepresentationProxy* repr)
{
  if (!source)
  {
    return;
  }

  // HACK: to set default representation type to Slices.
  vtkSMMultiSliceViewProxy::ForceRepresentationType(repr, "Slices");
  double bounds[6];
  if (vtkSMMultiSliceViewProxy::GetDataBounds(source, opport, bounds))
  {
    vtkBoundingBox bbox(bounds);

    double center[3];
    bbox.GetCenter(center);
    vtkSMPropertyHelper(this, "SlicePosition").Set(center, 3);

    double lengths[3];
    bbox.Scale(0.1, 0.1, 0.1);
    bbox.GetLengths(lengths);
    vtkSMPropertyHelper(this, "SliceIncrements").Set(lengths, 3);
    this->UpdateVTKObjects();
  }
}

//----------------------------------------------------------------------------
void vtkSMOrthographicSliceViewProxy::OnMouseWheelBackwardEvent(
  vtkObject*, unsigned long, void* calldata)
{
  double* data = reinterpret_cast<double*>(calldata);
  vtkSMPropertyHelper posHelper(this, "SlicePosition");
  posHelper.Set(data, 3);
  this->UpdateVTKObjects();
  this->InvokeEvent(vtkCommand::MouseWheelBackwardEvent, data);
  this->StillRender(); // FIXME: I want the Qt layer to do this.
}

//----------------------------------------------------------------------------
void vtkSMOrthographicSliceViewProxy::OnMouseWheelForwardEvent(
  vtkObject*, unsigned long, void* calldata)
{
  double* data = reinterpret_cast<double*>(calldata);
  vtkSMPropertyHelper posHelper(this, "SlicePosition");
  posHelper.Set(data, 3);
  this->UpdateVTKObjects();
  this->InvokeEvent(vtkCommand::MouseWheelForwardEvent, data);
  this->StillRender(); // FIXME: I want the Qt layer to do this.
}

//----------------------------------------------------------------------------
void vtkSMOrthographicSliceViewProxy::OnPlacePointEvent(vtkObject*, unsigned long, void* calldata)
{
  double* data = reinterpret_cast<double*>(calldata);
  vtkSMPropertyHelper posHelper(this, "SlicePosition");
  posHelper.Set(data, 3);
  this->UpdateVTKObjects();
  this->InvokeEvent(vtkCommand::PlacePointEvent, data);
  this->StillRender(); // FIXME: I want the Qt layer to do this.
}

//----------------------------------------------------------------------------
void vtkSMOrthographicSliceViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
