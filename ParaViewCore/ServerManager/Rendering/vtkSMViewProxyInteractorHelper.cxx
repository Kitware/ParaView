/*=========================================================================

  Program:   ParaView
  Module:    vtkSMViewProxyInteractorHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMViewProxyInteractorHelper.h"

#include "vtkMemberFunctionCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMViewProxy.h"

#include <cassert>

vtkStandardNewMacro(vtkSMViewProxyInteractorHelper);
//----------------------------------------------------------------------------
vtkSMViewProxyInteractorHelper::vtkSMViewProxyInteractorHelper()
  : DelayedRenderTimerId(-1)
  , Interacting(false)
  , Interacted(false)
{
  this->Observer = vtkMakeMemberFunctionCommand(*this, &vtkSMViewProxyInteractorHelper::Execute);
}

//----------------------------------------------------------------------------
vtkSMViewProxyInteractorHelper::~vtkSMViewProxyInteractorHelper()
{
  vtkMemberFunctionCommand<vtkSMViewProxyInteractorHelper>::SafeDownCast(this->Observer)->Reset();
  this->Observer->Delete();
  this->Observer = NULL;
}

//----------------------------------------------------------------------------
void vtkSMViewProxyInteractorHelper::SetViewProxy(vtkSMViewProxy* proxy)
{
  if (this->ViewProxy != proxy)
  {
    if (this->ViewProxy)
    {
      this->ViewProxy->RemoveObserver(this->Observer);
    }
    this->ViewProxy = proxy;
    if (this->ViewProxy)
    {
      // Monitor renders that happen directly on the view, that way we don't
      // trigger extra renders if view already rendered.
      this->ViewProxy->AddObserver(vtkCommand::StartEvent, this->Observer);
    }
  }
}

//----------------------------------------------------------------------------
vtkSMViewProxy* vtkSMViewProxyInteractorHelper::GetViewProxy()
{
  return this->ViewProxy.GetPointer();
}

//----------------------------------------------------------------------------
void vtkSMViewProxyInteractorHelper::SetupInteractor(vtkRenderWindowInteractor* iren)
{
  if (this->Interactor == iren)
  {
    return;
  }
  if (this->Interactor)
  {
    this->Interactor->RemoveObserver(this->Observer);
  }
  this->Interactor = iren;
  if (this->Interactor)
  {
    // Turn off direct rendering from the interactor. The interactor calls
    // vtkRenderWindow::Render(). We don't want that. We want it to go through
    // the vtkSMViewProxy layer.
    this->Interactor->EnableRenderOff();

    this->Interactor->AddObserver(vtkCommand::RenderEvent, this->Observer);
    this->Interactor->AddObserver(vtkCommand::StartInteractionEvent, this->Observer);
    this->Interactor->AddObserver(vtkCommand::InteractionEvent, this->Observer);
    this->Interactor->AddObserver(vtkCommand::EndInteractionEvent, this->Observer);
    this->Interactor->AddObserver(vtkCommand::TimerEvent, this->Observer);

    this->Interactor->AddObserver(vtkCommand::WindowResizeEvent, this->Observer);
  }
}

//----------------------------------------------------------------------------
vtkRenderWindowInteractor* vtkSMViewProxyInteractorHelper::GetInteractor()
{
  return this->Interactor.GetPointer();
}

//----------------------------------------------------------------------------
void vtkSMViewProxyInteractorHelper::Execute(vtkObject* caller, unsigned long event, void* calldata)
{
  assert(this->ViewProxy);
  if (caller == this->ViewProxy.GetPointer())
  {
    switch (event)
    {
      case vtkCommand::StartEvent:
        // The view is rendering on its own. If we had delayed a render request,
        // that needs to be cancelled since it's not needed anymore, the view is
        // already rendering.
        this->CleanupTimer();
        break;
    }
    return;
  }

  assert(caller == this->Interactor.GetPointer());
  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::SafeDownCast(caller);
  assert(iren);
  switch (event)
  {
    case vtkCommand::WindowResizeEvent:
      this->Resize();
      break;

    case vtkCommand::RenderEvent:
      this->CleanupTimer();
      this->Render();
      break;

    case vtkCommand::StartInteractionEvent:
      this->Interacting = true;
      this->Interacted = false;
      this->CleanupTimer();
      break;

    case vtkCommand::InteractionEvent:
      this->Interacted = true;
      break;

    case vtkCommand::EndInteractionEvent:
      this->Interacting = false;
      if (this->Interacted)
      {
        this->Interacted = false;

        assert(this->DelayedRenderTimerId == -1);
        double delay =
          vtkSMPropertyHelper(this->ViewProxy, "NonInteractiveRenderDelay", /*quiet*/ true)
            .GetAsDouble();
        if (delay <= 0.01)
        {
          this->Render();
        }
        else
        {
          this->DelayedRenderTimerId = iren->CreateOneShotTimer(delay * 1000);
          this->InvokeEvent(vtkCommand::CreateTimerEvent, &this->DelayedRenderTimerId);
        }
      }
      break;

    case vtkCommand::TimerEvent:
    {
      assert(calldata);
      int timerId = *(reinterpret_cast<int*>(calldata));
      if (this->DelayedRenderTimerId == timerId)
      {
        this->InvokeEvent(vtkCommand::TimerEvent, &this->DelayedRenderTimerId);
        this->DelayedRenderTimerId = -1;
        this->Render();
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMViewProxyInteractorHelper::CleanupTimer()
{
  if (this->DelayedRenderTimerId != -1)
  {
    assert(this->Interactor);
    this->InvokeEvent(vtkCommand::DestroyTimerEvent, &this->DelayedRenderTimerId);
    this->Interactor->DestroyTimer(this->DelayedRenderTimerId);
    this->DelayedRenderTimerId = -1;
  }
}

//----------------------------------------------------------------------------
void vtkSMViewProxyInteractorHelper::Render()
{
  this->CleanupTimer();
  if (vtkSMProperty* prop = this->ViewProxy->GetProperty("EnableRenderOnInteraction"))
  {
    if (vtkSMPropertyHelper(prop).GetAsInt() != 1)
    {
      return;
    }
  }

  if (this->Interactor->GetEnableRender())
  {
    vtkWarningMacro(
      "The Interactor is set to render automatically. "
      "That is not expected. Rendering should be handled by vtkSMViewProxyInteractorHelper "
      "to avoid duplicate rendering and parallel issues");
    return;
  }

  if (this->Interacting)
  {
    this->ViewProxy->InteractiveRender();
  }
  else
  {
    this->ViewProxy->StillRender();
  }
}

//----------------------------------------------------------------------------
void vtkSMViewProxyInteractorHelper::Resize()
{
  if (auto iren = this->GetInteractor())
  {
    int size[2];
    iren->GetSize(size);

    vtkSMPropertyHelper(this->ViewProxy, "ViewSize").Set(size, 2);
    this->ViewProxy->UpdateProperty("ViewSize");
  }
}

//----------------------------------------------------------------------------
void vtkSMViewProxyInteractorHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ViewProxy: " << this->ViewProxy.GetPointer() << endl;
}
