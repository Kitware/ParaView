/*=========================================================================

  Program:   ParaView
  Module:    vtkPVVCRControl.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVVCRControl.h"
#include "vtkObjectFactory.h"
#include "vtkKWIcon.h"
#include "vtkKWPushButton.h"
#include "vtkKWCheckButton.h"
#include "vtkKWApplication.h"
#include "vtkKWFrame.h"

vtkStandardNewMacro(vtkPVVCRControl);
vtkCxxRevisionMacro(vtkPVVCRControl, "1.1");
//-----------------------------------------------------------------------------
vtkPVVCRControl::vtkPVVCRControl()
{
  this->PlayButton = vtkKWPushButton::New();
  this->StopButton = vtkKWPushButton::New();
  this->GoToBeginningButton = vtkKWPushButton::New();
  this->GoToEndButton = vtkKWPushButton::New();
  this->GoToPreviousButton = vtkKWPushButton::New();
  this->GoToNextButton = vtkKWPushButton::New();
  this->LoopCheckButton = vtkKWCheckButton::New();
  this->InPlay = 0;

  this->PlayCommand=0;
  this->StopCommand=0;
  this->GoToBeginningCommand=0;
  this->GoToEndCommand=0;
  this->GoToPreviousCommand=0;
  this->GoToNextCommand=0;
  this->LoopCheckCommand=0;

}

//-----------------------------------------------------------------------------
vtkPVVCRControl::~vtkPVVCRControl()
{
  this->PlayButton->Delete();
  this->StopButton->Delete();
  this->GoToBeginningButton->Delete();
  this->GoToEndButton->Delete();
  this->GoToPreviousButton->Delete();
  this->GoToNextButton->Delete();
  this->LoopCheckButton->Delete();
  this->SetPlayCommand(0);
  this->SetStopCommand(0);
  this->SetGoToBeginningCommand(0);
  this->SetGoToEndCommand(0);
  this->SetGoToPreviousCommand(0);
  this->SetGoToNextCommand(0);
  this->SetLoopCheckCommand(0);

}

//-----------------------------------------------------------------------------
void vtkPVVCRControl::Create(vtkKWApplication* app)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("Widget already created.");
    return;
    }

  this->Superclass::Create(app);

  vtkKWIcon* icon = vtkKWIcon::New();
  // Animation Control: Play button to start the animation.
  this->PlayButton->SetParent(this->GetFrame());
  this->PlayButton->Create(app, "");
  icon->SetImage(vtkKWIcon::ICON_TRANSPORT_PLAY);
  this->PlayButton->SetImageOption(icon);
  this->PlayButton->SetCommand(this, "PlayCallback");

  // Animation Control: Stop button to stop the animation.
  this->StopButton->SetParent(this->GetFrame());
  this->StopButton->Create(app, "");
  icon->SetImage(vtkKWIcon::ICON_TRANSPORT_STOP);
  this->StopButton->SetImageOption(icon);
  this->StopButton->SetCommand(this, "StopCallback");

  // Animation Control: "go to beginning" button.
  this->GoToBeginningButton->SetParent(this->GetFrame());
  this->GoToBeginningButton->Create(app, "");
  icon->SetImage(vtkKWIcon::ICON_TRANSPORT_BEGINNING);
  this->GoToBeginningButton->SetImageOption(icon);
  this->GoToBeginningButton->SetCommand(this, "GoToBeginningCallback");

  // Animation Control: "go to end" button.
  this->GoToEndButton->SetParent(this->GetFrame());
  this->GoToEndButton->Create(app, "");
  icon->SetImage(vtkKWIcon::ICON_TRANSPORT_END);
  this->GoToEndButton->SetImageOption(icon);
  this->GoToEndButton->SetCommand(this, "GoToEndCallback");

  // Animation Control: "go to previous frame" button.
  this->GoToPreviousButton->SetParent(this->GetFrame());
  this->GoToPreviousButton->Create(app, 0);
  icon->SetImage(vtkKWIcon::ICON_TRANSPORT_REWIND_TO_KEY);
  this->GoToPreviousButton->SetImageOption(icon);
  this->GoToPreviousButton->SetCommand(this, "GoToPreviousCallback");

  // Animation Control: "go to next frame" button.
  this->GoToNextButton->SetParent(this->GetFrame());
  this->GoToNextButton->Create(app, 0);
  icon->SetImage(vtkKWIcon::ICON_TRANSPORT_FAST_FORWARD_TO_KEY);
  this->GoToNextButton->SetImageOption(icon);
  this->GoToNextButton->SetCommand(this, "GoToNextCallback");

  //  Animation Control: loop button to loop the animation.
  this->LoopCheckButton->SetParent(this->GetFrame());
  this->LoopCheckButton->Create(app, "");
  this->LoopCheckButton->SetState(0);
  this->LoopCheckButton->SetIndicator(0);
  icon->SetImage(vtkKWIcon::ICON_TRANSPORT_LOOP);
  this->LoopCheckButton->SetImageOption(icon);
  this->LoopCheckButton->SetCommand(this, "LoopCheckCallback");

  //  Animation Control: pack the transport buttons
  /*
  this->Script("pack %s %s %s %s %s %s %s -side left -fill both -padx 1", 
    this->GoToBeginningButton->GetWidgetName(), 
    this->GoToPreviousButton->GetWidgetName(),
    this->PlayButton->GetWidgetName(), 
    this->StopButton->GetWidgetName(),
    this->GoToNextButton->GetWidgetName(),
    this->GoToEndButton->GetWidgetName(), 
    this->LoopCheckButton->GetWidgetName());
  */
  this->AddWidget(this->GoToBeginningButton);
  this->AddWidget(this->GoToPreviousButton);
  this->AddWidget(this->PlayButton);
  this->AddWidget(this->StopButton);
  this->AddWidget(this->GoToNextButton);
  this->AddWidget(this->GoToEndButton);
  this->AddWidget(this->LoopCheckButton);
  icon->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVVCRControl::SetPlayCommand(vtkKWObject* calledObject, const char* commandString)
{
  this->SetObjectMethodCommand(&this->PlayCommand, calledObject, commandString);
}

//-----------------------------------------------------------------------------
void vtkPVVCRControl::SetStopCommand(vtkKWObject* calledObject, const char* commandString)
{
  this->SetObjectMethodCommand(&this->StopCommand, calledObject, commandString);
}

//-----------------------------------------------------------------------------
void vtkPVVCRControl::SetGoToBeginningCommand(vtkKWObject* calledObject, const char* commandString)
{
  this->SetObjectMethodCommand(&this->GoToBeginningCommand, calledObject, commandString);
}

//-----------------------------------------------------------------------------
void vtkPVVCRControl::SetGoToEndCommand(vtkKWObject* calledObject, const char* commandString)
{
  this->SetObjectMethodCommand(&this->GoToEndCommand, calledObject, commandString);
}

//-----------------------------------------------------------------------------
void vtkPVVCRControl::SetGoToPreviousCommand(vtkKWObject* calledObject, const char* commandString)
{
  this->SetObjectMethodCommand(&this->GoToPreviousCommand, calledObject, commandString);
}

//-----------------------------------------------------------------------------
void vtkPVVCRControl::SetGoToNextCommand(vtkKWObject* calledObject, const char* commandString)
{
  this->SetObjectMethodCommand(&this->GoToNextCommand, calledObject, commandString);
}

//-----------------------------------------------------------------------------
void vtkPVVCRControl::SetLoopCheckCommand(vtkKWObject* calledObject, const char* commandString)
{
  this->SetObjectMethodCommand(&this->LoopCheckCommand, calledObject, commandString);
}

//-----------------------------------------------------------------------------
void vtkPVVCRControl::SetLoopButtonState(int state)
{
  this->LoopCheckButton->SetState(state);
}

//-----------------------------------------------------------------------------
int vtkPVVCRControl::GetLoopButtonState()
{
  return this->LoopCheckButton->GetState();
}

//-----------------------------------------------------------------------------
void vtkPVVCRControl::PlayCallback()
{
  this->InvokeCommand(this->PlayCommand);
}

//-----------------------------------------------------------------------------
void vtkPVVCRControl::StopCallback()
{
  this->InvokeCommand(this->StopCommand);
}

//-----------------------------------------------------------------------------
void vtkPVVCRControl::GoToBeginningCallback()
{
  this->InvokeCommand(this->GoToBeginningCommand);
}

//-----------------------------------------------------------------------------
void vtkPVVCRControl::GoToEndCallback()
{
  this->InvokeCommand(this->GoToEndCommand);
}

//-----------------------------------------------------------------------------
void vtkPVVCRControl::GoToPreviousCallback()
{
  this->InvokeCommand(this->GoToPreviousCommand);
}

//-----------------------------------------------------------------------------
void vtkPVVCRControl::GoToNextCallback()
{
  this->InvokeCommand(this->GoToNextCommand);
}

//-----------------------------------------------------------------------------
void vtkPVVCRControl::LoopCheckCallback()
{
  this->InvokeCommand(this->LoopCheckCommand);
}

//-----------------------------------------------------------------------------
void vtkPVVCRControl::InvokeCommand(const char *command)
{
  if (command && *command)
    {
    this->Script("eval %s", command);
    }
}
//-----------------------------------------------------------------------------
void vtkPVVCRControl::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();
  if (!this->IsCreated())
    {
    return;
    }
  int enabled = this->Enabled;

  //These widgets are always off except when playing.
  this->Enabled = this->GetInPlay();
  this->PropagateEnableState(this->StopButton);

  //These widgets are on when playing or when GUI is enabled.
  this->Enabled = this->GetInPlay() || enabled;
  this->PropagateEnableState(this->LoopCheckButton);

  //These widgets are disabled when playing.
  this->Enabled = enabled && !this->GetInPlay();
  this->PropagateEnableState(this->PlayButton);
  this->PropagateEnableState(this->GoToBeginningButton);
  this->PropagateEnableState(this->GoToEndButton);
  this->PropagateEnableState(this->GoToPreviousButton);
  this->PropagateEnableState(this->GoToNextButton);
  
  this->Enabled = enabled;
}

//-----------------------------------------------------------------------------
void vtkPVVCRControl::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "InPlay: " << this->InPlay << endl;
}
