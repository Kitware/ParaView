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
#include "vtkPVApplication.h"
#include "vtkPVWindow.h"
#include "vtkPVSourceInterface.h"
#include "vtkPVMethodInterface.h"
#include "vtkKWEntry.h"
#include "vtkKWScale.h"
#include "vtkKWPushButton.h"
#include "vtkKWView.h"


// We need to:
// Format min/max/resolution entries better.
// Add callbacks to take the place of accept button.
// Handle methods with multiple entries.
// Handle special sources (contour, probe, threshold).






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
  this->MethodInterfaceIndex = -1;

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

  this->SetView(NULL);
  this->SetWindow(NULL);
}

//----------------------------------------------------------------------------
vtkPVAnimationInterface* vtkPVAnimationInterface::New()
{
  return new vtkPVAnimationInterface();
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

  this->SourceMethodFrame->Create(this->Application, "frame", "");
  this->Script("pack %s -side top -expand t -fill both", 
               this->SourceMethodFrame->GetWidgetName());

  // Source menu's label.
  this->SourceLabel->Create(this->Application, "-width 15 -justify right");
  this->SourceLabel->SetLabel("Filter/Source:");
  // Source menu
  this->SourceMenuButton->Create(app, "");
  this->SourceMenuButton->SetBalloonHelpString("Select the filter/source whose instance varible will change with time.");
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
  this->MethodMenuButton->SetButtonText("Method");
  this->MethodMenuButton->SetBalloonHelpString("Select the method that will be called.");
  this->Script("pack %s %s %s %s -side left", this->SourceLabel->GetWidgetName(),
                                              this->SourceMenuButton->GetWidgetName(),
                                              this->MethodLabel->GetWidgetName(),
                                              this->MethodMenuButton->GetWidgetName());

  this->UpdateSourceMenu();
  this->UpdateMethodMenu();
}

//----------------------------------------------------------------------------
void vtkPVAnimationInterface::EntryCallback()
{
  this->TimeStart = this->TimeStartEntry->GetValueAsFloat();
  this->TimeEnd = this->TimeEndEntry->GetValueAsFloat();
  this->TimeStep = this->TimeStepEntry->GetValueAsFloat();

  this->TimeScale->SetRange(this->TimeStart, this->TimeEnd);
  this->TimeScale->SetResolution(this->TimeStep);
}

//----------------------------------------------------------------------------
void vtkPVAnimationInterface::EntryUpdate()
{
  this->TimeStartEntry->SetValue(this->TimeStart);
  this->TimeEndEntry->SetValue(this->TimeEnd);
  this->TimeStepEntry->SetValue(this->TimeStep);

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
    if (this->View)
      {
      this->View->Render();
      }
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
    this->Script("pack %s -expand yes -fill x",
                 this->ScriptEditor->GetWidgetName());
    this->Script("pack forget %s", 
                 this->SourceMethodFrame->GetWidgetName());
    }
  else
    {
    this->Script("pack %s -side top -expand t -fill x", 
                 this->SourceMethodFrame->GetWidgetName());
    this->Script("pack forget %s", 
                 this->ScriptEditor->GetWidgetName());
    }
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
    this->PVSource->UnRegister(this);
    this->PVSource = NULL;
    }
  if (source)
    {
    source->Register(this);
    this->PVSource = source;
    this->SourceMenuButton->SetButtonText(this->PVSource->GetName());
    }
  else
    {
    this->SourceMenuButton->SetButtonText("None");
    }

  this->MethodInterfaceIndex = -1;
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

  // We need a different end test if the step is negative.
  sgn = 1.0;
  if (this->TimeStep < 0)
    {
    sgn = -1.0;
    }

  t = this->GetCurrentTime();
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
  this->StopFlag = 1;
}

//----------------------------------------------------------------------------
void vtkPVAnimationInterface::UpdateSourceMenu()
{
  char methodAndArgString[1024];
  int numSources;
  int i;
  vtkPVSource *source;
  int sourceValid = 0;
  
  // Remove all previous items form the menu.
  this->SourceMenuButton->GetMenu()->DeleteAllMenuItems();

  if (this->Window == NULL)
    {
    return;
    }

  // Update the selection menu.
  numSources = this->Window->GetSources()->GetNumberOfItems();
  
  for (i = 0; i < numSources; i++)
    {
    source = (vtkPVSource*)this->Window->GetSources()->GetItemAsObject(i);
    sprintf(methodAndArgString, "SetPVSource %s", source->GetTclName());
    this->SourceMenuButton->GetMenu()->AddCommand(source->GetName(), this,
                                                  methodAndArgString);
    if (this->PVSource == source)
      {
      sourceValid = 1;
      }
    }

  // The source may have been deleted. If so, then set our source to NULL.
  if ( ! sourceValid)
    {
    this->SetPVSource(NULL);
    }
}


//----------------------------------------------------------------------------
void vtkPVAnimationInterface::UpdateMethodMenu()
{
  char methodAndArgStr[512];
  int count;
  vtkPVSourceInterface *sInt;
  vtkCollection *mInts;
  vtkPVMethodInterface *mInt;

  // Remove all previous items form the menu.
  this->MethodMenuButton->GetMenu()->DeleteAllMenuItems();

  if (this->PVSource == NULL)
    {
    this->MethodMenuButton->SetButtonText("None");
    return;
    }

  // Lets try to do something for contour.
  if (this->PVSource->IsA("vtkPVContour"))
    {
    if ( ! this->ScriptCheckButton->GetState())
      {
      char str[1024];
      this->MethodMenuButton->SetButtonText("Value 0");

      sprintf(str, "%s SetValue 0 $pvTime", this->PVSource->GetVTKSourceTclName());
      this->ScriptEditor->SetValue(str);
      }
    }


  sInt = this->PVSource->GetInterface();
  if (sInt == NULL)
    {
    // Not all sources have interfaces yet.
    this->MethodMenuButton->SetButtonText("None");
    return;
    }
  mInts = sInt->GetMethodInterfaces();

  // Loop through the methods.
  count = 0;
  mInts->InitTraversal();
  while ( (mInt = ((vtkPVMethodInterface*)(mInts->GetNextItemAsObject()))) )
    {
    // We should really be able to iterate over files.
    if (mInt->GetWidgetType() == VTK_PV_METHOD_WIDGET_FILE)
      {
      // do anything wil listing directories and finding pattern?
      }
    // Small chance that we want to animate a selection list.
    //if (mInt->GetWidgetType() == VTK_PV_METHOD_WIDGET_SELECTION)
    // An extent could also be animated (clip).
    //if (mInt->GetWidgetType() == VTK_PV_METHOD_WIDGET_EXTENT)
    if (mInt->GetNumberOfArguments() == 1)
      {
      sprintf(methodAndArgStr, "SetMethodInterfaceIndex %d", count);

      this->MethodMenuButton->GetMenu()->AddCommand(mInt->GetVariableName(), 
                                                    this, methodAndArgStr);
      if (this->MethodInterfaceIndex < 0)
        {
        //this->MethodMenuButton->SetButtonText(mInt->GetVariableName());
        //this->MethodInterfaceIndex = count;
        // Call the callback method which also sets the script.
        // Do we want to wait for the end to do this?
        this->SetMethodInterfaceIndex(count);
        }
      }
    // What should we do if there are more than one arguments?

    ++count;
    }
  if (this->MethodInterfaceIndex < 0)
    {
    this->MethodMenuButton->SetButtonText("None");
    }
}


//----------------------------------------------------------------------------
void vtkPVAnimationInterface::SetMethodInterfaceIndex(int idx)
{
  vtkPVSourceInterface *sInt;
  vtkCollection *mInts;
  vtkPVMethodInterface *mInt;

  this->MethodInterfaceIndex = idx;

  if (this->MethodInterfaceIndex < 0)
    {
    this->SourceMenuButton->SetButtonText("None");
    // Do not erase the users script if they are editing it.
    if ( ! this->ScriptCheckButton->GetState())
      {
      this->ScriptEditor->SetValue("");
      }
    return;
    }

  if (this->PVSource == NULL)
    {
    vtkErrorMacro("Method set with no source.");
    return;
    }

  sInt = this->PVSource->GetInterface();
  mInts = sInt->GetMethodInterfaces();
  mInt = (vtkPVMethodInterface*)(mInts->GetItemAsObject(idx));


  // Do not change the users custom script (if they are editing it).
  if ( ! this->ScriptCheckButton->GetState())
    {
    if (mInt)
      {
      this->MethodMenuButton->SetButtonText(mInt->GetVariableName());
      char str[1024];
      if (mInt->GetArgumentType(0) == VTK_FLOAT)
        {
        sprintf(str, "%s %s $pvTime", this->PVSource->GetVTKSourceTclName(),
                mInt->GetSetCommand());
        }
      else if (mInt->GetArgumentType(0) == VTK_INT)
        {
        sprintf(str, "%s %s [expr int($pvTime)]", this->PVSource->GetVTKSourceTclName(),
                mInt->GetSetCommand());
        }
      else if (mInt->GetArgumentType(0) == VTK_STRING)
        {
        // Do the best we can for a default.
        // Append the time onto the current value of the string.
        this->Script("%s %s", this->PVSource->GetVTKSourceTclName(),
                     mInt->GetGetCommand());
        sprintf(str, "%s %s \"%s[expr int($pvTime)]\"", this->PVSource->GetVTKSourceTclName(),
                mInt->GetSetCommand(),
                this->Application->GetMainInterp()->result);
        }
      else
        {
        vtkErrorMacro("We do not handle this argument type yet.");
        return;
        }
      this->ScriptEditor->SetValue(str);
      }
    else
      {
      vtkErrorMacro("Method not found.");
      }
    }
}



