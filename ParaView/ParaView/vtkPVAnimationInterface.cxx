/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVAnimationInterface.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-2000 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include "vtkPVAnimationInterface.h"

#include "vtkKWCheckButton.h"
#include "vtkKWEntry.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWMenuButton.h"
#include "vtkKWPushButton.h"
#include "vtkKWScale.h"
#include "vtkKWText.h"
#include "vtkKWView.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVRenderView.h"
#include "vtkPVSource.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVWidget.h"
#include "vtkPVWidgetCollection.h"
#include "vtkPVWindow.h"

// We need to:
// Format min/max/resolution entries better.
// Add callbacks to take the place of accept button.
// Handle methods with multiple entries.
// Handle special sources (contour, probe, threshold).

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVAnimationInterface);

vtkCxxSetObjectMacro(vtkPVAnimationInterface,ControlledWidget, vtkPVWidget);

int vtkPVAnimationInterfaceCommand(ClientData cd, Tcl_Interp *interp,
			   int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVAnimationInterface::vtkPVAnimationInterface()
{
  this->CommandFunction = vtkPVAnimationInterfaceCommand;

  this->TimeStart = 0;
  this->TimeStep = 1;
  this->TimeEnd = 100;
  this->StopFlag = 0;

  this->PVSource = NULL;

  this->View = NULL;
  this->Window = NULL;

  // Time and animation control:
  this->ControlFrame = vtkKWLabeledFrame::New();
  this->ControlFrame->SetParent(this);
  this->ControlButtonFrame = vtkKWWidget::New();
  this->ControlButtonFrame->SetParent(this->ControlFrame->GetFrame());
  this->PlayButton = vtkKWPushButton::New();
  this->PlayButton->SetParent(this->ControlButtonFrame);
  this->StopButton = vtkKWPushButton::New();
  this->StopButton->SetParent(this->ControlButtonFrame);

  this->TimeFrame = vtkKWWidget::New();
  this->TimeFrame->SetParent(this->ControlFrame->GetFrame());
  this->TimeStartEntry = vtkKWLabeledEntry::New();
  this->TimeStartEntry->SetParent(this->TimeFrame);
  this->TimeEndEntry = vtkKWLabeledEntry::New();
  this->TimeEndEntry->SetParent(this->TimeFrame);
  this->TimeStepEntry = vtkKWLabeledEntry::New();
  this->TimeStepEntry->SetParent(this->TimeFrame);

  this->TimeScale = vtkKWScale::New();
  this->TimeScale->SetParent(this->ControlFrame->GetFrame());

  // Action and script editing.
  this->ActionFrame = vtkKWLabeledFrame::New();
  this->ActionFrame->SetParent(this);

  this->ScriptCheckButtonFrame = vtkKWWidget::New();
  this->ScriptCheckButtonFrame->SetParent(this->ActionFrame->GetFrame());
  this->ScriptCheckButton = vtkKWCheckButton::New();
  this->ScriptCheckButton->SetParent(this->ScriptCheckButtonFrame);
  this->ScriptEditor = vtkKWText::New();
  this->ScriptEditor->SetParent(this->ActionFrame->GetFrame());
  this->SourceMethodFrame = vtkKWWidget::New();
  this->SourceMethodFrame->SetParent(this->ActionFrame->GetFrame());

  this->SourceLabel = vtkKWLabel::New();
  this->SourceLabel->SetParent(this->SourceMethodFrame);
  this->SourceMenuButton = vtkKWMenuButton::New();
  this->SourceMenuButton->SetParent(this->SourceMethodFrame);

  this->MethodLabel = vtkKWLabel::New();
  this->MethodLabel->SetParent(this->SourceMethodFrame);
  this->MethodMenuButton = vtkKWMenuButton::New();
  this->MethodMenuButton->SetParent(this->SourceMethodFrame);

  this->ControlledWidget = NULL;
}

//----------------------------------------------------------------------------
vtkPVAnimationInterface::~vtkPVAnimationInterface()
{
  this->CommandFunction = vtkPVAnimationInterfaceCommand;

  if (this->ControlFrame)
    {
    this->ControlFrame->Delete();
    this->ControlFrame = NULL;
    }
  if (this->ControlButtonFrame)
    {
    this->ControlButtonFrame->Delete();
    this->ControlButtonFrame = NULL;
    }
  if (this->PlayButton)
    {
    this->PlayButton->Delete();
    this->PlayButton = NULL;
    }
  if (this->StopButton)
    {
    this->StopButton->Delete();
    this->StopButton = NULL;
    }
  if (this->TimeScale)
    {
    this->TimeScale->Delete();
    this->TimeScale = NULL;
    }
  if (this->TimeFrame)
    {
    this->TimeFrame->Delete();
    this->TimeFrame = NULL;
    }
  if (this->TimeStartEntry)
    {
    this->TimeStartEntry->Delete();
    this->TimeStartEntry = NULL;
    }
  if (this->TimeEndEntry)
    {
    this->TimeEndEntry->Delete();
    this->TimeEndEntry = NULL;
    }
  if (this->TimeStepEntry)
    {
    this->TimeStepEntry->Delete();
    this->TimeStepEntry = NULL;
    }

  if (this->ScriptEditor)
    {
    this->ScriptEditor->Delete();
    this->ScriptEditor = NULL;
    }
  
  if (this->ActionFrame)
    {
    this->ActionFrame->Delete();
    this->ActionFrame = NULL;
    }
  if (this->ScriptCheckButtonFrame)
    {
    this->ScriptCheckButtonFrame->Delete();
    this->ScriptCheckButtonFrame = NULL;
    }
  if (this->ScriptCheckButton)
    {
    this->ScriptCheckButton->Delete();
    this->ScriptCheckButton = NULL;
    }
  if (this->SourceMethodFrame)
    {
    this->SourceMethodFrame->Delete();
    this->SourceMethodFrame = NULL;
    }
 if (this->SourceLabel)
    {
    this->SourceLabel->Delete();
    this->SourceLabel = NULL;
    }
  if (this->SourceMenuButton)
    {
    this->SourceMenuButton->Delete();
    this->SourceMenuButton = NULL;
    }

  if (this->MethodLabel)
    {
    this->MethodLabel->Delete();
    this->MethodLabel = NULL;
    }
  if (this->MethodMenuButton)
    {
    this->MethodMenuButton->Delete();
    this->MethodMenuButton = NULL;
    }

  this->SetPVSource(NULL);
  this->SetControlledWidget(NULL);
  this->SetView(NULL);
  this->SetWindow(NULL);
}

//----------------------------------------------------------------------------
void vtkPVAnimationInterface::Create(vtkKWApplication *app, char *frameArgs)
{  
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(app);

  if (this->Application)
    {
    vtkErrorMacro("Widget has already been created.");
    return;
    }

  if (pvApp == NULL)
    {
    vtkErrorMacro("Need the subclass vtkPVApplication to create this object.");
    return;
    }

  this->SetApplication(app);


  // Create the frame for this widget.
  this->Script("frame %s %s", this->GetWidgetName(), frameArgs);

  this->ControlFrame->ShowHideFrameOn();
  this->ControlFrame->Create(this->Application);
  this->Script("pack %s -side top -expand t -fill x", 
               this->ControlFrame->GetWidgetName());
  this->ControlFrame->SetLabel("Animation Control");
  this->ControlButtonFrame->Create(this->Application, "frame", "-bd 2");
  this->Script("pack %s -side top -expand t -fill x", 
               this->ControlButtonFrame->GetWidgetName());

  // Play button to start the animation.
  this->PlayButton->Create(this->Application, "");
  this->PlayButton->SetLabel("Play");
  this->PlayButton->SetCommand(this, "Play");
  this->Script("pack %s -side left -expand t -fill x", 
               this->PlayButton->GetWidgetName());
  // Stop button to stop the animation.
  this->StopButton->Create(this->Application, "");
  this->StopButton->SetLabel("Stop");
  this->StopButton->SetCommand(this, "Stop");
  this->Script("pack %s -side left -expand t -fill x", 
               this->StopButton->GetWidgetName());

  this->TimeScale->Create(this->Application, "");
  this->TimeScale->DisplayEntry();
  this->TimeScale->DisplayLabel("Time:");
  this->TimeScale->SetEndCommand(this, "CurrentTimeCallback");
  this->Script("pack %s -side top -expand t -fill x", 
               this->TimeScale->GetWidgetName());

  this->TimeFrame->Create(this->Application, "frame", "-bd 2");
  this->Script("pack %s -side top -expand t -fill x", 
               this->TimeFrame->GetWidgetName());
  // Stop button to stop the animation.
  this->TimeStartEntry->Create(this->Application);
  this->TimeStartEntry->GetEntry()->SetWidth(7);
  this->TimeStartEntry->SetLabel("Start");
  this->TimeStartEntry->SetValue(this->TimeStart, 2);
  this->TimeStepEntry->Create(this->Application);
  this->TimeStepEntry->GetEntry()->SetWidth(7);
  this->TimeStepEntry->SetLabel("Step");
  this->TimeStepEntry->SetValue(this->TimeStep, 2);
  this->TimeEndEntry->Create(this->Application);
  this->TimeEndEntry->GetEntry()->SetWidth(7);
  this->TimeEndEntry->SetLabel("End");
  this->TimeEndEntry->SetValue(this->TimeEnd, 2);
  this->Script("pack %s %s %s -side left -expand t -fill x", 
               this->TimeStartEntry->GetWidgetName(),
               this->TimeStepEntry->GetWidgetName(),
               this->TimeEndEntry->GetWidgetName());

  // Setup the call backs that change the animation time pararmeters.
  this->Script("bind %s <KeyPress-Return> {%s EntryCallback}",
               this->TimeStartEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());
  this->Script("bind %s <FocusOut> {%s EntryCallback}",
               this->TimeStartEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());
  this->Script("bind %s <KeyPress-Return> {%s EntryCallback}",
               this->TimeStepEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());
  this->Script("bind %s <FocusOut> {%s EntryCallback}",
               this->TimeStepEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());
  this->Script("bind %s <KeyPress-Return> {%s EntryCallback}",
               this->TimeEndEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());
  this->Script("bind %s <FocusOut> {%s EntryCallback}",
               this->TimeEndEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());

  this->ActionFrame->ShowHideFrameOn();
  this->ActionFrame->Create(this->Application);
  this->ActionFrame->SetLabel("Action");
  this->Script("pack %s -side top -expand t -fill x", 
               this->ActionFrame->GetWidgetName());

  this->ScriptCheckButtonFrame->Create(this->Application, "frame", "");
  this->Script("pack %s -side top -expand t -fill x", 
               this->ScriptCheckButtonFrame->GetWidgetName());
  this->ScriptCheckButton->Create(this->Application, "-text {Script Editor}");
  this->ScriptCheckButton->SetCommand(this, "ScriptCheckButtonCallback");
  this->Script("pack %s -side left -expand f", 
               this->ScriptCheckButton->GetWidgetName());

  this->ScriptEditor->Create(this->Application, "-relief sunken -bd 2");
  this->Script("bind %s <KeyPress> {%s ScriptEditorCallback}",
               this->ScriptEditor->GetWidgetName(), this->GetTclName());

  this->SourceMethodFrame->Create(this->Application, "frame", "");
  this->Script("pack %s -side top -expand t -fill both", 
               this->SourceMethodFrame->GetWidgetName());

  // Source menu's label.
  this->SourceLabel->Create(this->Application, "-width 15 -justify right");
  this->SourceLabel->SetLabel("Filter/Source:");
  // Source menu
  this->SourceMenuButton->GetMenu()->SetTearOff(0);
  this->SourceMenuButton->Create(app, "");
  this->SourceMenuButton->SetBalloonHelpString(
    "Select the filter/source whose instance varible will change with time.");
  if (this->PVSource)
    {
    this->SourceMenuButton->SetButtonText(this->PVSource->GetName());
    }
  else
    {
    this->SourceMenuButton->SetButtonText("None");
    }


  // Method menu's label.
  this->MethodLabel->Create(this->Application, "-width 15 -justify right");
  this->MethodLabel->SetLabel("Variable:");
  // Method menu
  this->MethodMenuButton->Create(app, "");
  this->MethodMenuButton->GetMenu()->SetTearOff(0);
  this->MethodMenuButton->SetButtonText("Method");
  this->MethodMenuButton->SetBalloonHelpString(
    "Select the method that will be called.");
  this->Script("pack %s %s %s %s -side left", 
	       this->SourceLabel->GetWidgetName(),
	       this->SourceMenuButton->GetWidgetName(),
	       this->MethodLabel->GetWidgetName(),
	       this->MethodMenuButton->GetWidgetName());

  this->UpdateSourceMenu();
  this->UpdateMethodMenu();
}

//----------------------------------------------------------------------------
void vtkPVAnimationInterface::SetTimeStart(float t)
{
  this->TimeStart = t;

  if (this->Application)
    {
    this->TimeStartEntry->SetValue(this->TimeStart, 2);
    }

}

//----------------------------------------------------------------------------
void vtkPVAnimationInterface::SetTimeEnd(float t)
{
  this->TimeEnd = t;

  if (this->Application)
    {
    this->TimeEndEntry->SetValue(this->TimeEnd, 2);
    }

}

//----------------------------------------------------------------------------
void vtkPVAnimationInterface::SetTimeStep(float t)
{
  this->TimeStep = t;

  if (this->Application)
    {
    this->TimeStepEntry->SetValue(this->TimeStep, 2);
    }

}


//----------------------------------------------------------------------------
void vtkPVAnimationInterface::EntryCallback()
{
  this->TimeStart = this->TimeStartEntry->GetValueAsFloat();
  this->TimeEnd = this->TimeEndEntry->GetValueAsFloat();
  this->TimeStep = this->TimeStepEntry->GetValueAsFloat();

  this->TimeScale->SetRange(this->TimeStart, this->TimeEnd);
  this->TimeScale->SetResolution(this->TimeStep);

  this->AddTraceEntry("$kw(%s) SetTimeStart {%f}", this->GetTclName(), 
		      this->TimeStart);
  this->AddTraceEntry("$kw(%s) SetTimeEnd {%f}", this->GetTclName(), 
		      this->TimeEnd);
  this->AddTraceEntry("$kw(%s) SetTimeStep {%f}", this->GetTclName(), 
		      this->TimeStep);
  this->AddTraceEntry("$kw(%s) EntryCallback", this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkPVAnimationInterface::EntryUpdate()
{
  this->TimeStartEntry->SetValue(this->TimeStart, 2);
  this->TimeEndEntry->SetValue(this->TimeEnd, 2);
  this->TimeStepEntry->SetValue(this->TimeStep, 2);

  this->TimeScale->SetRange(this->TimeStart, this->TimeEnd);
  this->TimeScale->SetResolution(this->TimeStep);
}

//----------------------------------------------------------------------------
void vtkPVAnimationInterface::CurrentTimeCallback()
{
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(this->Application);
  if (pvApp)
    {
    pvApp->BroadcastScript("set pvTime %f", this->GetCurrentTime());
    pvApp->BroadcastScript("catch {%s}", this->ScriptEditor->GetValue());
    if (this->ControlledWidget)
      {
      this->ControlledWidget->ModifiedCallback();
      this->ControlledWidget->Reset();
      }
    if (this->View)
      {
      this->View->Render();
      }
    this->AddTraceEntry("$kw(%s) SetCurrentTime %f", this->GetTclName(),
			this->GetCurrentTime());
    this->AddTraceEntry("$kw(%s) CurrentTimeCallback", this->GetTclName());
    if (this->Window && this->Window->GetMainView())
      {
      this->AddTraceEntry("$kw(%s) ResetCameraClippingRange", 
			  this->Window->GetMainView()->GetTclName());
      }

    this->AddTraceEntry("update");
    }
}

//----------------------------------------------------------------------------
float vtkPVAnimationInterface::GetCurrentTime()
{
  return this->TimeScale->GetValue();
}

//----------------------------------------------------------------------------
void vtkPVAnimationInterface::ScriptCheckButtonCallback()
{

  if (this->ScriptCheckButton->GetState())
    {
    this->AddTraceEntry("$kw(%s) SetScriptCheckButtonState 1", 
			this->GetTclName());
    this->Script("pack %s -expand yes -fill x",
                 this->ScriptEditor->GetWidgetName());
    this->Script("pack forget %s", 
                 this->SourceMethodFrame->GetWidgetName());
    }
  else
    {
    this->AddTraceEntry("$kw(%s) SetScriptCheckButtonState 0", 
			this->GetTclName());
    this->Script("pack %s -side top -expand t -fill x", 
                 this->SourceMethodFrame->GetWidgetName());
    this->Script("pack forget %s", 
                 this->ScriptEditor->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkPVAnimationInterface::SetScriptCheckButtonState(int val)
{
  if (this->ScriptCheckButton->GetState() == val)
    {
    return;
    }

  this->ScriptCheckButton->SetState(val);
  this->ScriptCheckButtonCallback();
}

//----------------------------------------------------------------------------
void vtkPVAnimationInterface::SetPVSource(vtkPVSource *source)
{
  if (source == this->PVSource)
    {
    return;
    }

  if (this->PVSource)
    {
    //this->PVSource->UnRegister(this);
    this->PVSource = NULL;
    }

  // Special case during destruction.
  if (this->SourceMenuButton == NULL)
    {
    return;
    }

  if (source)
    {
    //source->Register(this);
    this->PVSource = source;
    this->SourceMenuButton->SetButtonText(this->PVSource->GetName());
    this->AddTraceEntry("$kw(%s) SetPVSource {%s}", this->GetTclName(), 
			source->GetTclName());
    }
  else
    {
    this->SourceMenuButton->SetButtonText("None");
    this->AddTraceEntry("$kw(%s) SetPVSource {}", this->GetTclName());
    }

  this->UpdateMethodMenu();
}

//----------------------------------------------------------------------------
void vtkPVAnimationInterface::SetCurrentTime(float time)
{  
  this->TimeScale->SetValue(time);
  // Will the scale make the call back or should we?
}


//----------------------------------------------------------------------------
void vtkPVAnimationInterface::Play()
{  
  float t, sgn;

  // Make sue we have the up to date entries for end and step.
  this->EntryCallback();

  //this->AddTraceEntry("$kw(%s) Play", this->GetTclName());

  // We need a different end test if the step is negative.
  sgn = 1.0;
  if (this->TimeStep < 0)
    {
    sgn = -1.0;
    }

  t = this->GetCurrentTime();
  if ( t == this->GetTimeEnd() )
    {
    this->SetCurrentTime(this->GetTimeStart());
    this->CurrentTimeCallback();
    t = this->GetCurrentTime();
    }
  this->StopFlag = 0;
  while ((sgn*t) < (sgn*this->TimeEnd) && ! this->StopFlag)
    {
    t = t + this->TimeStep;
    if ((sgn*t) > (sgn*this->TimeEnd))
      {
      t = this->TimeEnd;
      }
    this->SetCurrentTime(t);
    this->CurrentTimeCallback();
    // Allow the stop button to do its thing.
    this->Script("update");
    }
}

//----------------------------------------------------------------------------
void vtkPVAnimationInterface::Stop()
{  
  if ( this->StopFlag )
    {
    this->SetCurrentTime(this->GetTimeStart());
    this->CurrentTimeCallback();
    }
  this->StopFlag = 1;
}

//----------------------------------------------------------------------------
void vtkPVAnimationInterface::UpdateSourceMenu()
{
  char methodAndArgString[1024];
  int sourceValid = 0;
  
  // Remove all previous items form the menu.
  this->SourceMenuButton->GetMenu()->DeleteAllMenuItems();

  if (this->Window == NULL)
    {
    return;
    }

  // Update the selection menu.
  vtkPVSourceCollection* col = this->Window->GetSourceList("Sources");
  if (col)
    {
    vtkPVSource *source;
    col->InitTraversal();
    while ( (source = col->GetNextPVSource()) )
      {
      sprintf(methodAndArgString, "SetPVSource %s", source->GetTclName());
      this->SourceMenuButton->GetMenu()->AddCommand(source->GetName(), this,
						    methodAndArgString);
      if (this->PVSource == source)
	{
	sourceValid = 1;
	}
      }
    }

  // The source may have been deleted. If so, then set our source to NULL.
  if ( ! sourceValid)
    {
    this->SetPVSource(NULL);
    }
}

//----------------------------------------------------------------------------
void vtkPVAnimationInterface::ScriptEditorCallback()
{
  // If some one is typing in the script editor, then the method
  // selection is no longer valid.
  this->MethodMenuButton->SetButtonText("None");
}

//----------------------------------------------------------------------------
void vtkPVAnimationInterface::SetScript(const char* script)
{
  this->ScriptEditor->SetValue(script);
  this->MethodMenuButton->SetButtonText("None");
}

//----------------------------------------------------------------------------
void vtkPVAnimationInterface::SetLabelAndScript(const char* label,
                                                const char* script)
{
  this->SetScript(script);
  this->MethodMenuButton->SetButtonText(label);
  if (this->Application)
    {
    this->AddTraceEntry("$kw(%s) SetLabelAndScript {%s} {%s}", 
			this->GetTclName(), label, script);
    }
}


//----------------------------------------------------------------------------
const char* vtkPVAnimationInterface::GetScript()
{
  return this->ScriptEditor->GetValue();
}

//----------------------------------------------------------------------------
void vtkPVAnimationInterface::UpdateMethodMenu()
{
  vtkPVWidgetCollection *pvWidgets;
  vtkPVWidget *pvw;

  // Remove all previous items form the menu.
  this->MethodMenuButton->GetMenu()->DeleteAllMenuItems();

  this->MethodMenuButton->SetButtonText("None");
  if (this->PVSource == NULL)
    {
    return;
    }
  
  pvWidgets = this->PVSource->GetWidgets();
  pvWidgets->InitTraversal();
  while ( (pvw = pvWidgets->GetNextPVWidget()) )
    {
    pvw->AddAnimationScriptsToMenu(this->MethodMenuButton->GetMenu(), this);
    }
}



//----------------------------------------------------------------------------
void vtkPVAnimationInterface::SetWindow(vtkPVWindow *window)
{
  this->Window = window;
}

//----------------------------------------------------------------------------
void vtkPVAnimationInterface::SetView(vtkPVRenderView *renderView)
{
  this->View = renderView;
}


//----------------------------------------------------------------------------
void vtkPVAnimationInterface::SaveInTclScript(ofstream *file, 
                                              const char* fileRoot,
					      const char* extension,
					      const char* writerName)
{
  float t;
  float sgn;
  char countStr[100];

  // We need a different end test if the step is negative.
  sgn = 1.0;
  if (this->TimeStep < 0)
    {
    sgn = -1.0;
    }

  t = this->GetTimeStart();
  *file << "set pvTime " << t << "\n";
  *file << this->GetScript() << endl;
  *file << "if {$myProcId} {treeComp RenderRMI} else {\n\t";  
  sprintf(countStr, "%05d", (int)(t));
  if ( extension && writerName )
    {
    *file << "# This update is necessary to resolve some exposure event\n\t";
    *file << "# problems which occur with certain cards on Windows.\n\t";
    *file << "update\n\t";
    *file << "WinToImage Modified\n\t";
    *file << "Writer SetFileName {" << fileRoot << countStr << extension
	  << "}\n\t";
    *file << "Writer Write\n";
    }
  else
    {
    *file << "RenWin1 Render\n";
    }
  *file << "}\n\n"; 

  while ((sgn*t) < (sgn*this->TimeEnd))
    {
    t = t + this->TimeStep;
    if ((sgn*t) > (sgn*this->TimeEnd))
      {
      t = this->TimeEnd;
      }
    *file << "set pvTime " << t << "\n";
    *file << this->GetScript() << endl;
    *file << "if {$myProcId != 0} {treeComp RenderRMI} else {\n\t";  
    sprintf(countStr, "%05d", (int)(t));
    // Not necessary because WinToImage causes a render.
    //*file << "RenWin1 Render\n\t";
    *file << "# This update is necessary to resolve some exposure event\n\t";
    *file << "# problems which occur with certain cards on Windows.\n\t";
    *file << "update\n\t";
    *file << "WinToImage Modified\n\t";
    *file << "Writer SetFileName {" << fileRoot << countStr << extension
	  << "}\n\t";
    *file << "Writer Write\n"; 
    *file << "}\n\n";
    }
}

//----------------------------------------------------------------------------
void vtkPVAnimationInterface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ControlledWidget: " << this->GetControlledWidget();
  os << indent << "PVSource: " << this->GetPVSource();
  os << indent << "TimeEnd: " << this->GetTimeEnd();
  os << indent << "TimeStart: " << this->GetTimeStart();
  os << indent << "TimeStep: " << this->GetTimeStep();
  os << indent << "View: " << this->GetView();
  os << indent << "Window: " << this->GetWindow();
}
