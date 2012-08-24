/*=========================================================================

  Program:   ParaView
  Module:    vtkSMThreeSliceViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMThreeSliceViewProxy.h"

#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVThreeSliceView.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMProxyLink.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

#include <assert.h>
#include <vector>

vtkStandardNewMacro(vtkSMThreeSliceViewProxy);
//----------------------------------------------------------------------------
vtkSMThreeSliceViewProxy::vtkSMThreeSliceViewProxy()
{
  this->NeedToBeInitialized = true;
}

//----------------------------------------------------------------------------
vtkSMThreeSliceViewProxy::~vtkSMThreeSliceViewProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMThreeSliceViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
void vtkSMThreeSliceViewProxy::CreateVTKObjects()
{
  this->Superclass::CreateVTKObjects();

  if(this->Location != 0 && this->NeedToBeInitialized)
    {
    this->NeedToBeInitialized = false;

    this->GetInternalClientSideView()->AddObserver(
          vtkCommand::ConfigureEvent,
          this, &vtkSMThreeSliceViewProxy::InvokeConfigureEvent);

    this->GetInternalClientSideView()->Initialize(
          this->GetTopLeftViewProxy(),this->GetTopRightViewProxy(),
          this->GetBottomLeftViewProxy(),this->GetBottomRightViewProxy());
    }
}

//----------------------------------------------------------------------------
vtkSMRenderViewProxy* vtkSMThreeSliceViewProxy::GetTopLeftViewProxy()
{
  return vtkSMRenderViewProxy::SafeDownCast(this->GetSubProxy("TopLeft"));
}

//----------------------------------------------------------------------------
vtkSMRenderViewProxy* vtkSMThreeSliceViewProxy::GetTopRightViewProxy()
{
  return vtkSMRenderViewProxy::SafeDownCast(this->GetSubProxy("TopRight"));
}

//----------------------------------------------------------------------------
vtkSMRenderViewProxy* vtkSMThreeSliceViewProxy::GetBottomLeftViewProxy()
{
  return vtkSMRenderViewProxy::SafeDownCast(this->GetSubProxy("BottomLeft"));
}

//----------------------------------------------------------------------------
vtkSMRenderViewProxy* vtkSMThreeSliceViewProxy::GetBottomRightViewProxy()
{
  return vtkSMRenderViewProxy::SafeDownCast(this->GetSubProxy("BottomRight"));
}

//----------------------------------------------------------------------------
vtkPVThreeSliceView* vtkSMThreeSliceViewProxy::GetInternalClientSideView()
{
  return vtkPVThreeSliceView::SafeDownCast(this->GetClientSideObject());
}
//----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMThreeSliceViewProxy::CreateDefaultRepresentation(
  vtkSMProxy* source, int opport)
{
  if (!source)
    {
    return 0;
    }

  assert("Session should be valid" && this->GetSession());
  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();

  // Update with time to avoid domains updating without time later.
  vtkSMSourceProxy* sproxy = vtkSMSourceProxy::SafeDownCast(source);
  if (sproxy)
    {
    double view_time = vtkSMPropertyHelper(this, "ViewTime").GetAsDouble();
    sproxy->UpdatePipeline(view_time);
    }

  // Choose which type of representation proxy to create.
  vtkSMProxy* prototype = pxm->GetPrototypeProxy("representations",
    "CompositeMultiSliceRepresentation");
  vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool sg = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (sg)
    {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "CompositeMultiSliceRepresentation"));
    return repr;
    }

  // Currently only images can be shown
  vtkErrorMacro("This view only supports Multi-Slice representation.");
  return 0;
}

//----------------------------------------------------------------------------
const char* vtkSMThreeSliceViewProxy::IsSelectVisiblePointsAvailable()
{
  // The original dataset and the slice don't share the same points
  return "Multi-Slice View do not allow point selection";
}
//----------------------------------------------------------------------------
void vtkSMThreeSliceViewProxy::MarkDirty(vtkSMProxy* modifiedProxy)
{
  if (vtkSMViewProxy::SafeDownCast(modifiedProxy) == NULL)
    {
    this->GetInternalClientSideView()->MarkOutdated();
    }
  this->Superclass::MarkDirty(modifiedProxy);
}
//----------------------------------------------------------------------------
void vtkSMThreeSliceViewProxy::InvokeConfigureEvent()
{
  this->InvokeEvent(vtkCommand::ConfigureEvent);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
