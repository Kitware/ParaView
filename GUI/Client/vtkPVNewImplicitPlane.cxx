/*=========================================================================

  Program:   ParaView
  Module:    vtkPVNewImplicitPlane.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVNewImplicitPlane.h"

#include "vtkCommand.h"
#include "vtkKWEntry.h"
#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVDataInformation.h"
#include "vtkPVSource.h"
#include "vtkPVTraceHelper.h"
#include "vtkSMDisplayProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMNew3DWidgetProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMSourceProxy.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVNewImplicitPlane);
vtkCxxRevisionMacro(vtkPVNewImplicitPlane, "1.2");

//----------------------------------------------------------------------------
class vtkPVNewImplicitPlaneObserver : public vtkCommand
{
public:
  static vtkPVNewImplicitPlaneObserver *New() 
    { return new vtkPVNewImplicitPlaneObserver; }
  virtual void Execute(vtkObject*, unsigned long, void*)
    {
      if (this->Widget)
        {
        this->Widget->ModifiedCallback();
        }
    }
  vtkPVNewImplicitPlaneObserver():Widget(0) {}
  vtkPVNewImplicitPlane* Widget;
};


//----------------------------------------------------------------------------
vtkPVNewImplicitPlane::vtkPVNewImplicitPlane()
{
  this->WidgetProxy = 0;
  this->ImplicitPlaneProxy = 0;

  this->Observer = vtkPVNewImplicitPlaneObserver::New();
  this->Observer->Widget = this;
}

//----------------------------------------------------------------------------
vtkPVNewImplicitPlane::~vtkPVNewImplicitPlane()
{
  if (this->WidgetProxy)
    {
    this->WidgetProxy->Delete();
    }
  if (this->ImplicitPlaneProxy)
    {
    this->ImplicitPlaneProxy->Delete();
    }

  this->Observer->Delete();
}


//----------------------------------------------------------------------------
void vtkPVNewImplicitPlane::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(
    this->GetApplication());

  this->WidgetProxy = vtkSMNew3DWidgetProxy::SafeDownCast(
    vtkSMObject::GetProxyManager()->NewProxy(
      "displays", "ImplicitPlaneWidgetDisplay"));

  // Force object creation
  this->WidgetProxy->UpdateVTKObjects();
  this->WidgetProxy->AddObserver(vtkCommand::PropertyModifiedEvent,
                                 this->Observer);
  // Add to the render module.
  vtkSMRenderModuleProxy* rm = pvApp->GetRenderModuleProxy();
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    rm->GetProperty("Displays"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Displays on RenderModuleProxy.");
    }
  else
    {
    pp->AddProxy(this->WidgetProxy);
    rm->UpdateVTKObjects();
    }

  this->ImplicitPlaneProxy = vtkSMObject::GetProxyManager()->NewProxy(
    "implicit_functions", "Plane");
  vtkSMProxyProperty* cf = vtkSMProxyProperty::SafeDownCast(
    this->GetPVSource()->GetProxy()->GetProperty("CutFunction"));
  cf->AddProxy(this->ImplicitPlaneProxy);
  this->GetPVSource()->GetProxy()->UpdateVTKObjects();
}


//----------------------------------------------------------------------------
void vtkPVNewImplicitPlane::Accept()
{
  vtkSMDoubleVectorProperty* outNormal = 
    vtkSMDoubleVectorProperty::SafeDownCast(
      this->ImplicitPlaneProxy->GetProperty("Normal"));
  vtkSMDoubleVectorProperty* outOrigin = 
    vtkSMDoubleVectorProperty::SafeDownCast(
      this->ImplicitPlaneProxy->GetProperty("Origin"));

  vtkSMDoubleVectorProperty* inNormal = 
    vtkSMDoubleVectorProperty::SafeDownCast(
      this->WidgetProxy->GetProperty("Normal"));
  vtkSMDoubleVectorProperty* inOrigin = 
    vtkSMDoubleVectorProperty::SafeDownCast(
      this->WidgetProxy->GetProperty("Origin"));

  outNormal->SetElements(inNormal->GetElements());
  outOrigin->SetElements(inOrigin->GetElements());

  this->ImplicitPlaneProxy->UpdateVTKObjects();

  this->Superclass::Accept();
}

//----------------------------------------------------------------------------
void vtkPVNewImplicitPlane::Initialize()
{
  double bds[6];
  if (  this->PVSource->GetPVInput(0))
    {
    this->PVSource->GetPVInput(0)->GetDataInformation()->GetBounds(bds);
    }
  else
    {
    bds[0] = bds[2] = bds[4] = 0.0;
    bds[1] = bds[3] = bds[5] = 1.0;
    }

  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("PlaceWidget"));
  dvp->SetElements(bds);
  dvp->Modified();
  this->WidgetProxy->UpdateProperty("PlaceWidget");
}

//----------------------------------------------------------------------------
void vtkPVNewImplicitPlane::ResetInternal()
{
  if (!this->ModifiedFlag)
    {
    return;
    }
  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
void vtkPVNewImplicitPlane::Select()
{
  if(this->WidgetProxy)
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->WidgetProxy->GetProperty("Visibility"));
    ivp->SetElements1(1);

    vtkSMIntVectorProperty* enabled = vtkSMIntVectorProperty::SafeDownCast(
      this->WidgetProxy->GetProperty("Enabled"));
    enabled->SetElements1(1);

    this->WidgetProxy->UpdateVTKObjects();
    }
}

//----------------------------------------------------------------------------
void vtkPVNewImplicitPlane::Deselect()
{
  if(this->WidgetProxy)
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->WidgetProxy->GetProperty("Visibility"));
    ivp->SetElements1(0);

    vtkSMIntVectorProperty* enabled = vtkSMIntVectorProperty::SafeDownCast(
      this->WidgetProxy->GetProperty("Enabled"));
    enabled->SetElements1(0);

    this->WidgetProxy->UpdateVTKObjects();
    }
}


//----------------------------------------------------------------------------
void vtkPVNewImplicitPlane::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

}

//---------------------------------------------------------------------------
void vtkPVNewImplicitPlane::Trace(ofstream *file)
{
  if ( ! this->GetTraceHelper()->Initialize(file))
    {
    return;
    }
}

//----------------------------------------------------------------------------
void vtkPVNewImplicitPlane::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
