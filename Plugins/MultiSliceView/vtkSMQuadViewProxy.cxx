/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMQuadViewProxy.h"

#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVQuadRenderView.h"
#include "vtkPVRenderViewProxy.h"
#include "vtkRenderWindow.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"

#include <assert.h>

namespace
{
  class vtkRenderHelper : public vtkPVRenderViewProxy
  {
public:
  static vtkRenderHelper* New();
  vtkTypeMacro(vtkRenderHelper, vtkPVRenderViewProxy);

  virtual void EventuallyRender()
    {
    this->Proxy->StillRender();
    }
  virtual vtkRenderWindow* GetRenderWindow() { return NULL; }
  virtual void Render()
    {
    this->Proxy->InteractiveRender();
    }
  // Description:
  // Returns true if the most recent render indeed employed low-res rendering.
  virtual bool LastRenderWasInteractive()
    {
    return this->Proxy->LastRenderWasInteractive();
    }

  vtkWeakPointer<vtkSMRenderViewProxy> Proxy;
  };
  vtkStandardNewMacro(vtkRenderHelper);
}

vtkStandardNewMacro(vtkSMQuadViewProxy);
//----------------------------------------------------------------------------
vtkSMQuadViewProxy::vtkSMQuadViewProxy()
{
}

//----------------------------------------------------------------------------
vtkSMQuadViewProxy::~vtkSMQuadViewProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMQuadViewProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects();

  // If prototype, no need to go thurther...
  if (this->Location == 0 || !this->ObjectsCreated)
    {
    return;
    }

  vtkPVQuadRenderView* quadView = vtkPVQuadRenderView::SafeDownCast(
    this->GetClientSideObject());
  for (int cc=0; cc < 3; cc++)
    {
    vtkNew<vtkRenderHelper> helper;
    helper->Proxy = this;
    quadView->GetOrthoRenderView(cc)->GetInteractor()->SetPVRenderView(
      helper.GetPointer());
    }
}

//----------------------------------------------------------------------------
void vtkSMQuadViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMQuadViewProxy::CreateDefaultRepresentation(
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
    "QuadViewCompositeMultiSliceRepresentation");
  vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool sg = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (sg)
    {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "QuadViewCompositeMultiSliceRepresentation"));
    return repr;
    }

  // Currently only images can be shown
  vtkErrorMacro("This view only supports Multi-Slice representation.");
  return 0;
}

//----------------------------------------------------------------------------
const char* vtkSMQuadViewProxy::IsSelectVisiblePointsAvailable()
{
  // The original dataset and the slice don't share the same points
  return "Multi-Slice View do not allow point selection";
}
