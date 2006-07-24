/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPickBoxWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPickBoxWidget.h"

#include "vtkArrayMap.txx"
#include "vtkKWEntry.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkKWScaleWithEntry.h"
#include "vtkKWThumbWheel.h"
#include "vtkKWView.h"
#include "vtkMatrix4x4.h" 
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVApplication.h"
#include "vtkPVDataInformation.h"
#include "vtkPVInputMenu.h"
#include "vtkPVSource.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVXMLElement.h"
// ATTRIBUTE EDITOR
//#include "vtkSMBoxWidgetProxy.h"
#include "vtkSMPickBoxWidgetProxy.h"
#include "vtkKWCheckButton.h"
#include "vtkPickBoxWidget.h"
#include "vtkSMIntVectorProperty.h"
// ATTRIBUTE EDITOR
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkCommand.h"
#include "vtkPVWindow.h"
#include "vtkPVTraceHelper.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVWindow.h"

vtkStandardNewMacro(vtkPVPickBoxWidget);
vtkCxxRevisionMacro(vtkPVPickBoxWidget, "1.5");

//----------------------------------------------------------------------------
vtkPVPickBoxWidget::vtkPVPickBoxWidget()
{
// ATTRIBUTE EDITOR 
  this->MouseControlToggle = vtkKWCheckButton::New();
  this->MouseControlFlag = 0;
  this->InstructionsLabel = vtkKWLabel::New();

// ATTRIBUTE EDITOR 
//  this->SetWidgetProxyXMLName("BoxWidgetProxy");
  this->SetWidgetProxyXMLName("PickBoxWidgetProxy");

  this->EventCallbackCommand = vtkCallbackCommand::New();
  this->EventCallbackCommand->SetClientData(this); 
  this->EventCallbackCommand->SetCallback(vtkPVPickBoxWidget::ProcessEvents);
}

//----------------------------------------------------------------------------
vtkPVPickBoxWidget::~vtkPVPickBoxWidget()
{
// ATTRIBUTE EDITOR
  this->MouseControlToggle->Delete();
  this->InstructionsLabel->Delete();

  this->EventCallbackCommand->SetClientData(0);
  this->EventCallbackCommand->SetCallback(0);
  this->EventCallbackCommand->Delete();
  this->EventCallbackCommand = 0;
}

//----------------------------------------------------------------------------
void vtkPVPickBoxWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "MouseControlToggle: " << this->GetMouseControlToggle() << endl;

}

//----------------------------------------------------------------------------
void vtkPVPickBoxWidget::ChildCreate()
{
  this->Superclass::ChildCreate();

// ATTRIBUTE EDITOR
  // Widget needs the RenderModuleProxy for picking
  for (unsigned int ui=0; ui<this->WidgetProxy->GetNumberOfIDs(); ui++)
    {
    vtkPickBoxWidget* widget = vtkPickBoxWidget::SafeDownCast(
      this->GetPVApplication()->GetProcessModule()->GetObjectFromID(this->WidgetProxy->GetID(ui)));
    if (widget)
      {
      widget->SetRenderModuleProxy(this->GetPVApplication()->GetRenderModuleProxy());
      }
    }

// ATTRIBUTE EDITOR
  this->InstructionsLabel->SetParent(this->ControlFrame);
  this->InstructionsLabel->Create();
  this->InstructionsLabel->SetText("Press 'r' to relocate\nPress 't' to toggle interactions");
  this->Script("grid %s -sticky nws",
    this->InstructionsLabel->GetWidgetName());

// ATTRIBUTE EDITOR
  this->MouseControlToggle->SetParent(this->ControlFrame);
  this->MouseControlToggle->SetIndicatorVisibility(1);
  this->MouseControlToggle->Create();
  this->MouseControlToggle->SetText("Turn on/off interactions with widget");
  this->MouseControlToggle->SetSelectedState(1);
  this->MouseControlToggle->SetBalloonHelpString(
    "Enable interaction with the box widget.");
  this->MouseControlToggle->SetCommand(this, "SetMouseControlToggle");
  this->Script("grid %s -sticky nws",
    this->MouseControlToggle->GetWidgetName());

  // listen for the following events
  vtkPVGenericRenderWindowInteractor *interactor = this->GetPVApplication()->GetMainWindow()->GetInteractor();
  if(interactor)
    {
    interactor->AddObserver(vtkCommand::CharEvent, this->EventCallbackCommand, 1);
    }

//  this->Script("pack %s -fill x -expand t -pady 2",
//    this->ControlFrame->GetWidgetName());
}

//---------------------------------------------------------------------------
void vtkPVPickBoxWidget::Trace(ofstream *file)
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
void vtkPVPickBoxWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->MouseControlToggle);
}

// ATTRIBUTE EDITOR

//----------------------------------------------------------------------------
int vtkPVPickBoxWidget::GetMouseControlToggleInternal()
{
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("MouseControlToggle"));
  if (ivp)
    {
    return ivp->GetElement(0);
    }
 
  return -1;
}

void vtkPVPickBoxWidget::SetMouseControlToggle(int state)
{
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("MouseControlToggle"));
  if (ivp)
    {
    ivp->SetElements1(state);
    }
  this->WidgetProxy->UpdateVTKObjects();
}


//----------------------------------------------------------------------------
void vtkPVPickBoxWidget::ProcessEvents(vtkObject* vtkNotUsed(object), 
                                       unsigned long event, 
                                       void* clientdata, 
                                       void* vtkNotUsed(calldata))
{
  vtkPVPickBoxWidget* self = reinterpret_cast<vtkPVPickBoxWidget *>( clientdata );

  //look for char and delete events
  switch(event)
    {
    case vtkCommand::CharEvent:
      self->OnChar();
      break;
    }
}


//----------------------------------------------------------------------------
void vtkPVPickBoxWidget::OnChar()
{
  if (this->GetPVApplication()->GetMainWindow()->GetInteractor()->GetKeyCode() == 't' ||
      this->GetPVApplication()->GetMainWindow()->GetInteractor()->GetKeyCode() == 'T' )
    {
    this->GetMouseControlToggle()->ToggleSelectedState();
    this->SetMouseControlToggle(this->GetMouseControlToggle()->GetSelectedState());
    }
}
