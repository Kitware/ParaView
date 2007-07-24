/*=========================================================================

  Program:   ParaView
  Module:    vtkSMScalarBarWidgetRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMScalarBarWidgetRepresentationProxy.h"

#include "vtkClientServerInterpreter.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkProcessModule.h"
#include "vtkScalarBarWidget.h"
#include "vtkRenderer.h"
#include "vtkScalarBarActor.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"

#include <vtkstd/list>

vtkStandardNewMacro(vtkSMScalarBarWidgetRepresentationProxy);
vtkCxxRevisionMacro(vtkSMScalarBarWidgetRepresentationProxy, "1.5");

class vtkSMScalarBarWidgetRepresentationObserver : public vtkCommand
{
public:
  static vtkSMScalarBarWidgetRepresentationObserver *New() 
    { return new vtkSMScalarBarWidgetRepresentationObserver; }
  virtual void Execute(vtkObject*, unsigned long event, void*)
    {
      if (this->Proxy)
        {
        this->Proxy->ExecuteEvent(event);
        }
    }
  vtkSMScalarBarWidgetRepresentationObserver():Proxy(0) {}
  vtkSMScalarBarWidgetRepresentationProxy* Proxy;
};

//----------------------------------------------------------------------------
vtkSMScalarBarWidgetRepresentationProxy::vtkSMScalarBarWidgetRepresentationProxy()
{
  this->ActorProxy = 0;
  this->Widget = vtkScalarBarWidget::New();
  this->Observer = vtkSMScalarBarWidgetRepresentationObserver::New();
  this->Observer->Proxy = this;
  this->ViewProxy = 0;
  this->Visibility = 1;
}

//----------------------------------------------------------------------------
vtkSMScalarBarWidgetRepresentationProxy::~vtkSMScalarBarWidgetRepresentationProxy()
{
  this->Observer->Proxy = 0;
  this->Observer->Delete();
  this->Observer = 0;
  this->ActorProxy = 0;
  this->Widget->Delete();
  this->Widget = 0;
  this->ViewProxy = 0;
}

//----------------------------------------------------------------------------
bool vtkSMScalarBarWidgetRepresentationProxy::AddToView(vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* renderView = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!renderView)
    {
    vtkErrorMacro("View must be a vtkSMRenderViewProxy.");
    return false;
    }

  if (this->ActorProxy)
    {
    renderView->AddPropToRenderer2D(this->ActorProxy);
    }
  
  this->ViewProxy = view;
  this->SetVisibility(this->Visibility);

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMScalarBarWidgetRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* renderView = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!renderView)
    {
    vtkErrorMacro("View must be a vtkSMRenderViewProxy.");
    return false;
    }

  if (this->ActorProxy)
    {
    renderView->RemovePropFromRenderer2D(this->ActorProxy);
    }
  
  if (this->Widget->GetEnabled())
    {
    this->Widget->SetEnabled(0);
    }
  this->Widget->SetInteractor(0);
  this->Widget->SetCurrentRenderer(0);
  this->ViewProxy = 0;

  return this->Superclass::RemoveFromView(view);
}

//-----------------------------------------------------------------------------
void vtkSMScalarBarWidgetRepresentationProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->ActorProxy = this->GetSubProxy("Prop2D");
  if (!this->ActorProxy)
    {
    vtkErrorMacro("Failed to find subproxy Prop2D.");
    return;
    }

  this->ActorProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);

  this->Superclass::CreateVTKObjects();

  // Set up the widget.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkScalarBarActor* actor = vtkScalarBarActor::SafeDownCast(
    pm->GetObjectFromID(this->ActorProxy->GetID()));

  if (!actor)
    {
    vtkErrorMacro("Failed to create client side ScalarBarActor.");
    return;
    }
  this->Widget->SetScalarBarActor(actor);
  this->Widget->AddObserver(vtkCommand::InteractionEvent,
    this->Observer);
  this->Widget->AddObserver(vtkCommand::StartInteractionEvent,
    this->Observer);
  this->Widget->AddObserver(vtkCommand::EndInteractionEvent,
    this->Observer);
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetRepresentationProxy::SetVisibility(int visible)
{
  this->Visibility = visible;
  if (!this->ViewProxy)
    {
    return;
    }
  vtkSMRenderViewProxy* renderView = 
    vtkSMRenderViewProxy::SafeDownCast(this->ViewProxy);
  if (!renderView)
    {
    vtkErrorMacro("View must be a vtkSMRenderViewProxy.");
    return;
    }

  // Set widget interactor.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::SafeDownCast(
    pm->GetObjectFromID(renderView->GetInteractorProxy()->GetID()));
  if (!iren)
    {
    vtkErrorMacro("Failed to get client side Interactor.");
    return;
    }
  this->Widget->SetInteractor(iren);

  vtkRenderer* ren = vtkRenderer::SafeDownCast(
    pm->GetObjectFromID(renderView->GetRenderer2DProxy()->GetID()));
  if (!ren)
    {
    vtkErrorMacro("Failed to get client side 2D renderer.");
    return;
    }
  this->Widget->SetCurrentRenderer(ren);
  this->Widget->SetEnabled(visible);

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ActorProxy->GetProperty("Visibility"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property Visibility.");
    return;
    }
  ivp->SetElement(0, visible);
  this->ActorProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMScalarBarWidgetRepresentationProxy::ExecuteEvent(unsigned long event)
{
  vtkPVGenericRenderWindowInteractor* iren;
  iren = vtkPVGenericRenderWindowInteractor::SafeDownCast(
    this->Widget->GetInteractor());
  switch (event)
    {
  case vtkCommand::StartInteractionEvent:
    // enable Interactive rendering.
    iren->InteractiveRenderEnabledOn();
    break;
    
  case vtkCommand::EndInteractionEvent:
    // disable interactive rendering.
    iren->InteractiveRenderEnabledOff();
    break;

  case vtkCommand::InteractionEvent:
    // Take the client position values and push on to the server.
    vtkScalarBarActor* actor = this->Widget->GetScalarBarActor();
    double *pos1 = actor->GetPositionCoordinate()->GetValue();
    double *pos2 = actor->GetPosition2Coordinate()->GetValue();
    int orientation = actor->GetOrientation();
    vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->ActorProxy->GetProperty("Position"));
    if (dvp)
      {
      dvp->SetElement(0, pos1[0]);
      dvp->SetElement(1, pos1[1]);
      }
    else
      {
      vtkErrorMacro("Failed to find property Position on ActorProxy.");
      }

    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->ActorProxy->GetProperty("Position2"));
    if (dvp)
      {
      dvp->SetElement(0, pos2[0]);
      dvp->SetElement(1, pos2[1]);
      }
    else
      {
      vtkErrorMacro("Failed to find property Position2 on ActorProxy.");
      }

    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->ActorProxy->GetProperty("Orientation"));
    if (ivp)
      {
      ivp->SetElement(0, orientation);
      }
    else
      {
      vtkErrorMacro("Failed to find property Orientation on ActorProxy.");
      }
    this->ActorProxy->UpdateVTKObjects();
    break;
    }
  this->InvokeEvent(event); // just in case the GUI wants to know about interaction.
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


