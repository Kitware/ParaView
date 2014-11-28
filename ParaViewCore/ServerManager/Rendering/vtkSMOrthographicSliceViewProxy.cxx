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

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVOrthographicSliceView.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"

#include <cassert>

vtkStandardNewMacro(vtkSMOrthographicSliceViewProxy);
//----------------------------------------------------------------------------
vtkSMOrthographicSliceViewProxy::vtkSMOrthographicSliceViewProxy()
{
}

//----------------------------------------------------------------------------
vtkSMOrthographicSliceViewProxy::~vtkSMOrthographicSliceViewProxy()
{
}

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

  vtkPVOrthographicSliceView* view = vtkPVOrthographicSliceView::SafeDownCast(
    this->GetClientSideObject());
  assert(view);

  view->AddObserver(vtkCommand::MouseWheelForwardEvent,
    this, &vtkSMOrthographicSliceViewProxy::OnMouseWheelForwardEvent);
  view->AddObserver(vtkCommand::MouseWheelBackwardEvent,
    this, &vtkSMOrthographicSliceViewProxy::OnMouseWheelBackwardEvent);
}

//----------------------------------------------------------------------------
const char* vtkSMOrthographicSliceViewProxy::GetRepresentationType(
  vtkSMSourceProxy* producer, int outputPort)
{
  if (!producer)
    {
    return 0;
    }

  assert("Session should be valid" && this->GetSession());
  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();

  // Choose which type of representation proxy to create.
  vtkSMProxy* prototype = pxm->GetPrototypeProxy("representations",
    "CompositeOrthographicSliceRepresentation");
  vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(producer, outputPort);
  bool sg = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  return sg? "CompositeOrthographicSliceRepresentation" :
    this->Superclass::GetRepresentationType(producer, outputPort);
}

//----------------------------------------------------------------------------
void vtkSMOrthographicSliceViewProxy::OnMouseWheelBackwardEvent(
  vtkObject*, unsigned long, void* calldata)
{
  double* data = reinterpret_cast<double*>(calldata);
  vtkSMPropertyHelper posHelper(this, "SlicePosition");
  posHelper.Set(data, 3);
  this->UpdateVTKObjects();
  this->InvokeEvent(vtkCommand::MouseWheelBackwardEvent);
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
  this->InvokeEvent(vtkCommand::MouseWheelForwardEvent);
  this->StillRender(); // FIXME: I want the Qt layer to do this.
}

//----------------------------------------------------------------------------
void vtkSMOrthographicSliceViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
