/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPickSphereWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPickSphereWidget.h"

#include "vtkArrayMap.txx"
#include "vtkCamera.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkKWView.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVApplication.h"
#include "vtkPVDisplayGUI.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVInputMenu.h"
#include "vtkPVSource.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVWindow.h"
#include "vtkPVXMLElement.h"
#include "vtkRenderer.h"
#include "vtkPVWindow.h"
#include "vtkPVTraceHelper.h"

#include "vtkKWEvent.h"

// ATTRIBUTE EDITOR
//#include "vtkSMSphereWidgetProxy.h"
#include "vtkSMPickSphereWidgetProxy.h"
#include "vtkPickSphereWidget.h"
#include "vtkKWCheckButton.h"
#include "vtkSMIntVectorProperty.h"

#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkCommand.h"

vtkStandardNewMacro(vtkPVPickSphereWidget);
vtkCxxRevisionMacro(vtkPVPickSphereWidget, "1.4");

//*****************************************************************************
//----------------------------------------------------------------------------
vtkPVPickSphereWidget::vtkPVPickSphereWidget()
{

// ATTRIBUTE EDITOR
//  this->SetWidgetProxyXMLName("SphereWidgetProxy");
  this->SetWidgetProxyXMLName("PickSphereWidgetProxy");
  this->InstructionsLabel = vtkKWLabel::New();
  this->MouseControlToggle = vtkKWCheckButton::New();
  this->MouseControlFlag = 0;

}

//----------------------------------------------------------------------------
vtkPVPickSphereWidget::~vtkPVPickSphereWidget()
{
// ATTRIBUTE EDITOR
  this->InstructionsLabel->Delete();
  this->MouseControlToggle->Delete();
}

//---------------------------------------------------------------------------
void vtkPVPickSphereWidget::Trace(ofstream *file)
{
  if ( ! this->GetTraceHelper()->Initialize(file))
    {
    return;
    }

  // Called to save the state of the widget's visibility
  this->Superclass::Trace(file);

  *file << "$kw(" << this->GetTclName() << ") SetMouseControlToggle "
        << this->MouseControlFlag << endl;
}

//----------------------------------------------------------------------------
void vtkPVPickSphereWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "SetMouseControlToggle" << this->GetMouseControlToggle() << endl;

}

//----------------------------------------------------------------------------
void vtkPVPickSphereWidget::ChildCreate()
{
  this->Superclass::ChildCreate();

  vtkPVApplication *pvApp = 
    vtkPVApplication::SafeDownCast(this->GetApplication());
// ATTRIBUTE EDITOR
  // Widget needs the RenderModuleProxy for picking
  unsigned int ui;
//  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  for (ui=0; ui<this->WidgetProxy->GetNumberOfIDs(); ui++)
    {
    vtkPickSphereWidget* widget = vtkPickSphereWidget::SafeDownCast(pvApp->GetProcessModule()->GetObjectFromID(this->WidgetProxy->GetID(ui)));
    if (widget)
      {
      widget->SetRenderModuleProxy(pvApp->GetRenderModuleProxy());
      }
    }

// ATTRIBUTE EDITOR
  this->InstructionsLabel->SetParent(this->Frame);
  this->InstructionsLabel->Create();
  this->InstructionsLabel->SetText("Press 'r' to relocate to mouse position\nPress 'e' to edit current region\nPress 't' to toggle mouse control between the model and widget");
  this->Script("grid %s - - -sticky e",
    this->InstructionsLabel->GetWidgetName());

// ATTRIBUTE EDITOR
  this->MouseControlToggle->SetParent(this->Frame);
  this->MouseControlToggle->Create();
  this->MouseControlToggle->SetText("Control Widget Only");
  this->MouseControlToggle->SetSelectedState(0);
  this->MouseControlToggle->SetBalloonHelpString(
    "Scale the model from anywhere in the view.");
  this->MouseControlToggle->SetCommand(this, "SetMouseControlToggle");

// ATTRIBUTE EDITOR
  this->Script("grid %s -sticky nws",
    this->MouseControlToggle->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVPickSphereWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->MouseControlToggle);

}

// ATTRIBUTE EDITOR
//----------------------------------------------------------------------------
int vtkPVPickSphereWidget::GetMouseControlToggleInternal()
{
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("MouseControlToggle"));
  if (ivp)
    {
    return ivp->GetElement(0);
    }
 
  return -1;
}

void vtkPVPickSphereWidget::SetMouseControlToggle(int state)
{
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("MouseControlToggle"));
  if (ivp)
    {
    ivp->SetElements1(state);
    }
  this->WidgetProxy->UpdateVTKObjects();

}
