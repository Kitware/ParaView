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

#include "vtkObjectFactory.h"
#include "vtkPVQuadRenderView.h"
#include "vtkPVRenderViewProxy.h"
#include "vtkRenderWindow.h"
#include "vtkNew.h"
#include "vtkPVGenericRenderWindowInteractor.h"
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
