/*=========================================================================

  Program:   ParaView
  Module:    vtkPVNew3DWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVNew3DWidget.h"

#include "vtkCommand.h"
#include "vtkKWEntry.h"
#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVTraceHelper.h"
#include "vtkSMDisplayProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMNew3DWidgetProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVNew3DWidget);
vtkCxxRevisionMacro(vtkPVNew3DWidget, "1.3");

//----------------------------------------------------------------------------
class vtkPVNew3DWidgetObserver : public vtkCommand
{
public:
  static vtkPVNew3DWidgetObserver *New() 
    { return new vtkPVNew3DWidgetObserver; }
  virtual void Execute(vtkObject*, unsigned long, void*)
    {
      if (this->Widget)
        {
        this->Widget->WidgetModified();
        }
    }
  vtkPVNew3DWidgetObserver():Widget(0) {}
  vtkPVNew3DWidget* Widget;
};


//----------------------------------------------------------------------------
vtkPVNew3DWidget::vtkPVNew3DWidget()
{
  this->LabelWidget  = vtkKWLabel::New();
  this->LabelWidget->SetParent(this);

  this->EntryWidget  = vtkKWEntry::New();
  this->EntryWidget->SetParent(this);

  this->WidgetProxy = 0;

  this->Observer = vtkPVNew3DWidgetObserver::New();
  this->Observer->Widget = this;
}

//----------------------------------------------------------------------------
vtkPVNew3DWidget::~vtkPVNew3DWidget()
{
  this->LabelWidget->Delete();
  this->EntryWidget->Delete();
  
  if (this->WidgetProxy)
    {
    this->WidgetProxy->Delete();
    }

  this->Observer->Delete();
}


//----------------------------------------------------------------------------
void vtkPVNew3DWidget::CreateWidget()
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

  this->LabelWidget->Create();
  this->LabelWidget->SetWidth(18);
  this->LabelWidget->SetJustificationToRight();
  this->LabelWidget->SetText("Slider value:");
  this->Script("pack %s -side left", this->LabelWidget->GetWidgetName());

  this->EntryWidget->Create();
  this->EntryWidget->SetWidth(5);
  this->Script("bind %s <KeyPress> {%s EntryModifiedCallback %K}",
               this->EntryWidget->GetWidgetName(), this->GetTclName());
  this->Script("bind %s <FocusOut> {%s EntryModifiedCallback {}}",
               this->EntryWidget->GetWidgetName(), this->GetTclName());
  this->Script("pack %s -side left -fill x -expand t", 
               this->EntryWidget->GetWidgetName());
  
  this->WidgetProxy = vtkSMNew3DWidgetProxy::SafeDownCast(
    vtkSMObject::GetProxyManager()->NewProxy(
      "displays", "SliderWidgetDisplay"));

  this->WidgetProxy->UpdateVTKObjects();
  vtkSMDoubleVectorProperty* p1 = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("Point1"));
  p1->SetElements3(-0.75, -0.5, 0);

  vtkSMDoubleVectorProperty* p2 = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("Point2"));
  p2->SetElements3( 0.75, -0.5, 0);

  vtkSMDoubleVectorProperty* min = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("MinimumValue"));
  min->SetElements1(6);

  vtkSMDoubleVectorProperty* max = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("MaximumValue"));
  max->SetElements1(128);

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

}

//----------------------------------------------------------------------------
void vtkPVNew3DWidget::Accept()
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());
  ivp->SetElements1(this->EntryWidget->GetValueAsInt());

  this->Superclass::Accept();
}

//----------------------------------------------------------------------------
void vtkPVNew3DWidget::Initialize()
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());
  this->EntryWidget->SetValueAsInt(ivp->GetElement(0));
  
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("Value"));
  dvp->SetElements1(ivp->GetElement(0));
  this->WidgetProxy->UpdateVTKObjects();
  this->GetPVApplication()->GetRenderModuleProxy()->StillRender();
}

//----------------------------------------------------------------------------
void vtkPVNew3DWidget::ResetInternal()
{
  if (!this->ModifiedFlag)
    {
    return;
    }
  this->Initialize();
  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
void vtkPVNew3DWidget::Select()
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
void vtkPVNew3DWidget::Deselect()
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
void vtkPVNew3DWidget::WidgetModified()
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("Value"));
  if (!dvp)
    {
    return;
    }

  double val = dvp->GetElement(0);
  if (val != this->EntryWidget->GetValueAsDouble())
    {
    this->EntryWidget->SetValueAsDouble(val);
    this->ModifiedCallback();
    }
}

//-----------------------------------------------------------------------------
void vtkPVNew3DWidget::EntryModifiedCallback(const char* key)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("Value"));
  if (!dvp)
    {
    return;
    }

  if ((key && (!strcmp(key, "Tab") ||
              !strcmp(key, "ISO_Left_Tab") ||
              !strcmp(key, "Return") ||
              !strcmp(key, ""))) || !key)
    {
    double val = dvp->GetElement(0);
    double eVal = this->EntryWidget->GetValueAsDouble();
    if (val != eVal)
      {
      dvp->SetElements1(eVal);
      this->WidgetProxy->UpdateVTKObjects();
      this->GetPVApplication()->GetRenderModuleProxy()->StillRender();
      this->ModifiedCallback();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVNew3DWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->LabelWidget);
  this->PropagateEnableState(this->EntryWidget);
}

//---------------------------------------------------------------------------
void vtkPVNew3DWidget::Trace(ofstream *file)
{
  if ( ! this->GetTraceHelper()->Initialize(file))
    {
    return;
    }
}

//----------------------------------------------------------------------------
void vtkPVNew3DWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
