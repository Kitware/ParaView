/*=========================================================================

  Program:   ParaView
  Module:    vtkSMScalarBarWidgetProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMScalarBarWidgetProxy.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkClientServerStream.h"
#include "vtkScalarBarActor.h"
#include "vtkScalarBarWidget.h"
#include "vtkCoordinate.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkCommand.h"
#include "vtkSMScalarBarActorProxy.h"

class vtkSMScalarBarWidgetProxyObserver : public vtkCommand
{
public:
  static vtkSMScalarBarWidgetProxyObserver* New()
    { return new vtkSMScalarBarWidgetProxyObserver; }
  
  void SetTarget(vtkSMScalarBarWidgetProxy* t)
    {
    this->Target = t;
    }
  virtual void Execute(vtkObject* o, unsigned long event, void *p)
    {
    if (this->Target)
      {
      this->Target->ExecuteEvent(o, event, p);
      }
    }
protected:
  vtkSMScalarBarWidgetProxyObserver() { this->Target = 0; }
  vtkSMScalarBarWidgetProxy* Target;
    
};
//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkSMScalarBarWidgetProxy);
vtkCxxRevisionMacro(vtkSMScalarBarWidgetProxy, "1.10");

//----------------------------------------------------------------------------
vtkSMScalarBarWidgetProxy::vtkSMScalarBarWidgetProxy()
{
  this->Observer = vtkSMScalarBarWidgetProxyObserver::New();
  this->Observer->SetTarget(this);
  this->ScalarBarActorProxy = 0;
  this->ScalarBarWidget = vtkScalarBarWidget::New();
  this->RenderModuleProxy = 0;
  this->Visibility = 0;
}

//----------------------------------------------------------------------------
vtkSMScalarBarWidgetProxy::~vtkSMScalarBarWidgetProxy()
{
  this->Observer->SetTarget(0);
  this->Observer->Delete();
  this->ScalarBarActorProxy = 0;
  this->ScalarBarWidget->Delete();
  this->RenderModuleProxy = 0;
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetProxy::AddToRenderModule(vtkSMRenderModuleProxy* rm)
{
  // adds this->ScalarBarActorProxy to the render module.
  this->Superclass::AddToRenderModule(rm); 
  
  this->RenderModuleProxy = rm;
  this->SetVisibility(this->Visibility);
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetProxy::RemoveFromRenderModule(vtkSMRenderModuleProxy* rm)
{
  // removes this->ScalarBarActorProxy from the render module.
  this->Superclass::RemoveFromRenderModule(rm);
  
  if (this->ScalarBarWidget->GetEnabled())
    {
    this->ScalarBarWidget->SetEnabled(0);
    }
  this->ScalarBarWidget->SetInteractor(0);
  this->ScalarBarWidget->SetCurrentRenderer(0);
  this->RenderModuleProxy = 0;
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->ScalarBarActorProxy = this->GetSubProxy("Prop2D");
  if (!this->ScalarBarActorProxy)
    {
    vtkErrorMacro("Failed to find subproxy Prop2D.");
    return;
    }

  this->ScalarBarActorProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);

  this->Superclass::CreateVTKObjects(numObjects);

  // Set up the widget.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkScalarBarActor* actor = vtkScalarBarActor::SafeDownCast(
    pm->GetObjectFromID(this->ScalarBarActorProxy->GetID(0)));

  if (!actor)
    {
    vtkErrorMacro("Failed to create client side ScalarBarActor.");
    return;
    }
  this->ScalarBarWidget->SetScalarBarActor(actor);
  this->ScalarBarWidget->AddObserver(vtkCommand::InteractionEvent,
    this->Observer);
  this->ScalarBarWidget->AddObserver(vtkCommand::StartInteractionEvent,
    this->Observer);
  this->ScalarBarWidget->AddObserver(vtkCommand::EndInteractionEvent,
    this->Observer);
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetProxy::SetVisibility(int visible)
{
  this->Visibility = visible;
  if (!this->RenderModuleProxy)
    {
    return;
    }

  // Set widget interactor.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::SafeDownCast(
    pm->GetObjectFromID(
      this->GetInteractorProxy(this->RenderModuleProxy)->GetID(0)));
  if (!iren)
    {
    vtkErrorMacro("Failed to get client side Interactor.");
    return;
    }
  this->ScalarBarWidget->SetInteractor(iren);

  vtkRenderer* ren = vtkRenderer::SafeDownCast(
    pm->GetObjectFromID(
      this->GetRenderer2DProxy(this->RenderModuleProxy)->GetID(0)));
  if (!ren)
    {
    vtkErrorMacro("Failed to get client side 2D renderer.");
    return;
    }
  this->ScalarBarWidget->SetCurrentRenderer(ren);
  this->ScalarBarWidget->SetEnabled(visible);

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ScalarBarActorProxy->GetProperty("Visibility"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property Visibility on XYPlotActorProxy.");
    return;
    }
  ivp->SetElement(0, visible);
  this->ScalarBarActorProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetProxy::ExecuteEvent(vtkObject*, 
  unsigned long event, void*)
{
  vtkPVGenericRenderWindowInteractor* iren;
  iren = vtkPVGenericRenderWindowInteractor::SafeDownCast(
    this->ScalarBarWidget->GetInteractor());
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
    vtkScalarBarActor* actor = this->ScalarBarWidget->GetScalarBarActor();
    double *pos1 = actor->GetPositionCoordinate()->GetValue();
    double *pos2 = actor->GetPosition2Coordinate()->GetValue();
    int orientation = actor->GetOrientation();
    vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->ScalarBarActorProxy->GetProperty("Position"));
    if (dvp)
      {
      dvp->SetElement(0, pos1[0]);
      dvp->SetElement(1, pos1[1]);
      }
    else
      {
      vtkErrorMacro("Failed to find property Position on ScalarBarActorProxy.");
      }

    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->ScalarBarActorProxy->GetProperty("Position2"));
    if (dvp)
      {
      dvp->SetElement(0, pos2[0]);
      dvp->SetElement(1, pos2[1]);
      }
    else
      {
      vtkErrorMacro("Failed to find property Position2 on ScalarBarActorProxy.");
      }

    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->ScalarBarActorProxy->GetProperty("Orientation"));
    if (ivp)
      {
      ivp->SetElement(0, orientation);
      }
    else
      {
      vtkErrorMacro("Failed to find property Orientation on ScalarBarActorProxy.");
      }
    this->ScalarBarActorProxy->UpdateVTKObjects();
    break;
    }
  this->InvokeEvent(event); // just in case the GUI wants to know about interaction.

}
  
//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Visibility: " << this->Visibility << endl;

}
