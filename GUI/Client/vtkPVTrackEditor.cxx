/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTrackEditor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVTrackEditor.h"

#include "vtkObjectFactory.h"
#include "vtkCommand.h"
#include "vtkKWFrameLabeled.h"
#include "vtkKWFrame.h"
#include "vtkKWPushButton.h"
#include "vtkKWMenuButton.h"
#include "vtkKWScale.h"
#include "vtkKWLabel.h"
#include "vtkKWEntry.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWApplication.h"
#include "vtkKWMenu.h"
#include "vtkPVSimpleAnimationCue.h"
#include "vtkSMAnimationCueProxy.h"
#include "vtkPVRampKeyFrame.h"
#include "vtkPVBooleanKeyFrame.h"
#include "vtkPVExponentialKeyFrame.h"
#include "vtkPVSinusoidKeyFrame.h"
#include "vtkPVTraceHelper.h"
#include "vtkPVAnimationManager.h"

vtkStandardNewMacro(vtkPVTrackEditor);
vtkCxxRevisionMacro(vtkPVTrackEditor, "1.1");
//-----------------------------------------------------------------------------
class vtkPVTrackEditorObserver : public vtkCommand
{
public:
  static vtkPVTrackEditorObserver* New() { return new vtkPVTrackEditorObserver; }

  void SetTarget(vtkPVTrackEditor* t) 
    {
    this->Target = t;
    }

  virtual void Execute(vtkObject* obj, unsigned long event, void* data)
    {
    if (this->Target)
      {
      this->Target->Update();
      }
    }
protected:
  vtkPVTrackEditorObserver()
    {
    this->Target = 0;
    }
  vtkPVTrackEditor* Target;
};

//-----------------------------------------------------------------------------
#define VTK_PV_RAMP_INDEX 1
#define VTK_PV_RAMP_LABEL "Ramp"
#define VTK_PV_STEP_INDEX 2
#define VTK_PV_STEP_LABEL "Step"
#define VTK_PV_EXPONENTIAL_INDEX 3
#define VTK_PV_EXPONENTIAL_LABEL "Exponential"
#define VTK_PV_SINUSOID_INDEX 4
#define VTK_PV_SINUSOID_LABEL "Sinusoid"

#define VTK_PV_KEYFRAME_PROPERTIES_DEFAULT_LABEL "Active Key Frame Properties"
#define VTK_PV_SELECTOR_DEFAULT_LABEL "Tracks" 
//-----------------------------------------------------------------------------
vtkPVTrackEditor::vtkPVTrackEditor()
{
  this->AnimationManager = NULL;
  this->KeyFramePropertiesFrame = vtkKWFrameLabeled::New();
  this->PropertiesFrame = vtkKWFrame::New();
  this->SelectKeyFrameLabel = vtkKWLabel::New();
  
  this->TypeFrame = vtkKWFrame::New();
  this->TypeImage = vtkKWPushButton::New();
  this->TypeLabel = vtkKWLabel::New();
  this->TypeMenuButton = vtkKWMenuButton::New();
  
  this->AddKeyFrameButton = vtkKWPushButton::New();
  this->DeleteKeyFrameButton = vtkKWPushButton::New();

  this->AnimationManager = NULL;
  this->SimpleAnimationCue = NULL;

  this->IndexScale = vtkKWScale::New();

  this->TitleLabelLabel = vtkKWLabel::New();
  this->TitleLabel = vtkKWLabel::New();
  this->InterpolationValid = 1;

  this->Observer = vtkPVTrackEditorObserver::New();
  this->Observer->SetTarget(this);
  this->ActiveKeyFrame = 0;
}

//-----------------------------------------------------------------------------
vtkPVTrackEditor::~vtkPVTrackEditor()
{
  this->Observer->SetTarget(0);
  this->Observer->Delete();

  this->SetAnimationManager(NULL);
  this->SetAnimationCue(NULL);
  this->KeyFramePropertiesFrame->Delete();
  this->PropertiesFrame->Delete();
  this->SelectKeyFrameLabel->Delete();
  this->TypeFrame->Delete();
  this->TypeImage->Delete();
  this->TypeLabel->Delete();
  this->TypeMenuButton->Delete();

  this->AddKeyFrameButton->Delete();
  this->DeleteKeyFrameButton->Delete();
  this->IndexScale->Delete();

  this->TitleLabelLabel->Delete();
  this->TitleLabel->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVTrackEditor::Create(vtkKWApplication* app, const char* args)
{
  if (!this->AnimationManager)
    {
    vtkErrorMacro("AnimationManager must be set");
    return;
    }

  if (!this->Superclass::Create(app, "frame", args))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }
  
  this->KeyFramePropertiesFrame->SetParent(this);
  this->KeyFramePropertiesFrame->ShowHideFrameOn();
  this->KeyFramePropertiesFrame->Create(app, 0);
  this->KeyFramePropertiesFrame->SetLabelText(VTK_PV_KEYFRAME_PROPERTIES_DEFAULT_LABEL);
  this->Script(
    "pack %s  -side top -anchor nw -fill x -expand t -padx 2 -pady 2", 
    this->KeyFramePropertiesFrame->GetWidgetName());

  this->TitleLabelLabel->SetParent(this->KeyFramePropertiesFrame->GetFrame());
  this->TitleLabelLabel->Create(app,"-relief flat");
  this->TitleLabelLabel->SetText("Current Track:");

  this->TitleLabel->SetParent(this->KeyFramePropertiesFrame->GetFrame());
  this->TitleLabel->Create(app,"-relief flat");
  vtkKWTkUtilities::ChangeFontWeightToBold(
    this->GetApplication()->GetMainInterp(), this->TitleLabel->GetWidgetName());

  this->PropertiesFrame->SetParent(this->KeyFramePropertiesFrame->GetFrame());
  this->PropertiesFrame->Create(app, 0);

  this->IndexScale->SetParent(this->PropertiesFrame);
  this->IndexScale->Create(app,0);
  this->IndexScale->DisplayEntry();
  this->IndexScale->DisplayEntryAndLabelOnTopOff();
  this->IndexScale->SetResolution(1);
  this->IndexScale->DisplayLabel("Index:");
  this->IndexScale->SetCommand(this, "IndexChangedCallback");
  this->IndexScale->SetEntryCommand(this, "IndexChangedCallback");
  this->IndexScale->SetEndCommand(this, "IndexChangedCallback");
  this->IndexScale->SetBalloonHelpString("Select a key frame at a particular index in the "
    "current track");

  this->TypeLabel->SetParent(this->PropertiesFrame);
  this->TypeLabel->Create(app, 0);
  this->TypeLabel->SetText("Interpolation:");

  this->TypeImage->SetParent(this->PropertiesFrame);
  this->TypeImage->Create(app, "-relief flat");
  this->TypeImage->SetBalloonHelpString("Specify the type of interpolation "
    "starting at the active key frame.");

  this->TypeMenuButton->SetParent(this->PropertiesFrame);
  this->TypeMenuButton->Create(app, "-image PVToolbarPullDownArrow -relief flat");
  this->TypeMenuButton->SetBalloonHelpString("Specify the type of interpolation "
    "starting at the active key frame.");
  this->TypeMenuButton->IndicatorOff();

  this->BuildTypeMenu();

  this->AddKeyFrameButton->SetParent(this->KeyFramePropertiesFrame->GetFrame());
  this->AddKeyFrameButton->Create(app, 0);
  this->AddKeyFrameButton->SetBalloonHelpString("Append a new key frame");
  this->AddKeyFrameButton->SetText("Add KeyFrame");
  this->AddKeyFrameButton->SetCommand(this, "AddKeyFrameButtonCallback");

  this->DeleteKeyFrameButton->SetParent(this->KeyFramePropertiesFrame->GetFrame());
  this->DeleteKeyFrameButton->Create(app, 0);
  this->DeleteKeyFrameButton->SetBalloonHelpString("Delete active key frame");
  this->DeleteKeyFrameButton->SetText("Delete KeyFrame");
  this->DeleteKeyFrameButton->SetCommand(this, "DeleteKeyFrameButtonCallback");

  this->SelectKeyFrameLabel->SetParent(this->KeyFramePropertiesFrame->GetFrame());
  this->SelectKeyFrameLabel->SetText("Select or Add a key frame in the Animation Tracks "
    "window to show its properties.");

  this->SelectKeyFrameLabel->Create(app, "-justify left");

  this->Script("grid %s - - -row 0 -sticky ew", this->IndexScale->GetWidgetName());

  this->Script("grid %s %s %s -columnspan 1 -row 2 -sticky w",
    this->TypeLabel->GetWidgetName(),
    this->TypeImage->GetWidgetName(),
    this->TypeMenuButton->GetWidgetName());


  this->Script("grid columnconfigure %s 2 -weight 2", 
    this->PropertiesFrame->GetWidgetName());
  this->Script("grid columnconfigure %s 1 -weight 2",
    this->KeyFramePropertiesFrame->GetFrame()->GetWidgetName());
}

//-----------------------------------------------------------------------------
void vtkPVTrackEditor::BuildTypeMenu()
{
  vtkKWMenu* menu = this->TypeMenuButton->GetMenu();
  char* var = menu->CreateRadioButtonVariable(this, "Radio");
  
  menu->AddRadioButton(VTK_PV_RAMP_INDEX, 
    VTK_PV_RAMP_LABEL, var, this, "SetKeyFrameType 0", 
    "Set the following Interpolator to Ramp.");
  menu->ConfigureItem(VTK_PV_RAMP_INDEX,"-image PVRamp"); 
  delete [] var;

  var = menu->CreateRadioButtonVariable(this, "Radio");
  menu->AddRadioButton(VTK_PV_STEP_INDEX, 
    VTK_PV_STEP_LABEL, var, this, "SetKeyFrameType 1",
    "Set the following Interpolator to Step.");
  menu->ConfigureItem(VTK_PV_STEP_INDEX,"-image PVStep");
  delete [] var;

  var = menu->CreateRadioButtonVariable(this, "Radio");
  menu->AddRadioButton(VTK_PV_EXPONENTIAL_INDEX, 
    VTK_PV_EXPONENTIAL_LABEL, var, this, "SetKeyFrameType 2",
    "Set the following Interpolator to Exponential.");
  menu->ConfigureItem(VTK_PV_EXPONENTIAL_INDEX,"-image PVExponential");
  delete [] var;

  var = menu->CreateRadioButtonVariable(this, "Radio");
  menu->AddRadioButton(VTK_PV_SINUSOID_INDEX,
    VTK_PV_SINUSOID_LABEL, var, this, "SetKeyFrameType 3",
    "Set the following Interpolator to Sinusoid.");
  menu->ConfigureItem(VTK_PV_SINUSOID_INDEX, "-image PVSinusoid");
  delete [] var;
}

//-----------------------------------------------------------------------------
void vtkPVTrackEditor::SetAnimationCue(vtkPVSimpleAnimationCue* cue)
{
  if (this->SimpleAnimationCue == cue)
    {
    return;
    }
  if (this->SimpleAnimationCue)
    {
    this->SimpleAnimationCue->RemoveObservers(vtkPVSimpleAnimationCue::SelectionChangedEvent,
      this->Observer);
    this->TitleLabel->SetText("");
    }
  
  vtkSetObjectBodyMacro(SimpleAnimationCue, vtkPVSimpleAnimationCue, cue);

  if (this->SimpleAnimationCue)
    {
    this->SimpleAnimationCue->AddObserver(vtkPVSimpleAnimationCue::SelectionChangedEvent,
      this->Observer);
    char* text = this->SimpleAnimationCue->GetTextRepresentation();
    this->TitleLabel->SetText(text);
    delete []text;
    }
  this->Update();
}

//-----------------------------------------------------------------------------
void vtkPVTrackEditor::ShowKeyFrame(int id)
{
  if (id < 0 || id >= this->SimpleAnimationCue->GetNumberOfKeyFrames())
    {
    this->SetActiveKeyFrame(NULL);
    return;
    }

  vtkPVKeyFrame* pvKeyFrame = this->SimpleAnimationCue->GetKeyFrame(id);
  this->SetActiveKeyFrame(pvKeyFrame);
  if (!pvKeyFrame)
    {
    vtkErrorMacro("Failed to get the keyframe");
    return;
    }
  //Lets try to determine time bounds, if any, for this keyframe.

  //Get rid of old times.
  pvKeyFrame->ClearTimeBounds();
  if (id > 0)
    {
    vtkPVKeyFrame* prev = this->SimpleAnimationCue->GetKeyFrame(id-1);
    if (prev)
      {
      pvKeyFrame->SetTimeMinimumBound(prev->GetKeyTime());
      }
    }

  if (id < this->SimpleAnimationCue->GetNumberOfKeyFrames()-1)
    {
    vtkPVKeyFrame* next = this->SimpleAnimationCue->GetKeyFrame(id+1);
    if (next)
      {
      pvKeyFrame->SetTimeMaximumBound(next->GetKeyTime());
      }
    this->InterpolationValid = 1;
    }
  else
    {
    this->InterpolationValid = 0;// last key frame does not use Interpolation.
    }
  pvKeyFrame->PrepareForDisplay();
  this->UpdateTypeImage(pvKeyFrame);
  
}

//-----------------------------------------------------------------------------
void vtkPVTrackEditor::SetActiveKeyFrame(vtkPVKeyFrame* keyframe)
{
  if (this->ActiveKeyFrame == keyframe)
    {
    return;
    }
  if (this->ActiveKeyFrame)
    {
    this->Script("grid forget %s", this->ActiveKeyFrame->GetWidgetName());
    this->Script("grid forget %s", this->PropertiesFrame->GetWidgetName());
    }
  vtkSetObjectBodyMacro(ActiveKeyFrame, vtkPVKeyFrame, keyframe);

  if (this->ActiveKeyFrame)
    {
    this->Script("grid forget %s", this->SelectKeyFrameLabel->GetWidgetName());
    this->Script("grid %s - -row 1 -sticky ew", this->PropertiesFrame->GetWidgetName());
    this->Script("grid %s -columnspan 3 -row 1 -sticky ew",
      this->ActiveKeyFrame->GetWidgetName());
    }
  else
    {
    this->Script("grid %s - -row 1 -sticky ew", this->SelectKeyFrameLabel->GetWidgetName());
    }
}

//-----------------------------------------------------------------------------
void vtkPVTrackEditor::UpdateTypeImage(vtkPVKeyFrame* keyframe)
{
  if (vtkPVRampKeyFrame::SafeDownCast(keyframe))
    {
    this->TypeMenuButton->GetMenu()->CheckRadioButton(this, "Radio", 
      VTK_PV_RAMP_INDEX);
    this->TypeImage->ConfigureOptions("-image PVRamp");
    }
  else if (vtkPVBooleanKeyFrame::SafeDownCast(keyframe))
    {
    this->TypeMenuButton->GetMenu()->CheckRadioButton(this, "Radio", 
      VTK_PV_STEP_INDEX);
    this->TypeImage->ConfigureOptions("-image PVStep");
    }
  else if (vtkPVExponentialKeyFrame::SafeDownCast(keyframe))
    {
    this->TypeMenuButton->GetMenu()->CheckRadioButton(this, "Radio", 
      VTK_PV_EXPONENTIAL_INDEX);
    this->TypeImage->ConfigureOptions("-image PVExponential");
    }
  else if (vtkPVSinusoidKeyFrame::SafeDownCast(keyframe))
    {
    this->TypeMenuButton->GetMenu()->CheckRadioButton(this, "Radio",
      VTK_PV_SINUSOID_INDEX);
    this->TypeImage->ConfigureOptions("-image PVSinusoid");
    }
}


//-----------------------------------------------------------------------------
void vtkPVTrackEditor::IndexChangedCallback()
{
  int val = this->IndexScale->GetEntry()->GetValueAsInt() -1;
  this->SetKeyFrameIndex(val);
}

//-----------------------------------------------------------------------------
void vtkPVTrackEditor::SetKeyFrameIndex(int val)
{
  if (!this->SimpleAnimationCue || this->SimpleAnimationCue->GetVirtual())
    {
    return;
    } 
  if (val <0 || val >= this->SimpleAnimationCue->GetNumberOfKeyFrames())
    {
    return;
    }
  this->SimpleAnimationCue->SelectKeyFrame(val);
  this->IndexScale->SetValue(val+1);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetKeyFrameIndex %d", this->GetTclName(), val);
}

//-----------------------------------------------------------------------------
void vtkPVTrackEditor::Update()
{
  // This wierd stuff is needed as otherwise if the Animation Interaface hasn't been
  // packed even once, Tcl would get stuck adjusting the wraplenth.
  if (!this->IsPacked() && this->SelectKeyFrameLabel->GetAdjustWrapLengthToWidth())
    {
    this->SelectKeyFrameLabel->AdjustWrapLengthToWidthOff();
    }
  else if (this->IsPacked() && !this->SelectKeyFrameLabel->GetAdjustWrapLengthToWidth())
    {
    this->SelectKeyFrameLabel->AdjustWrapLengthToWidthOn();
    }

  int id = -1;
  if (!this->SimpleAnimationCue || this->SimpleAnimationCue->GetVirtual()
    || (id = this->SimpleAnimationCue->GetSelectedKeyFrameIndex()) == -1)
    {
    this->SetActiveKeyFrame(NULL);
    }
  else
    {
    
    this->IndexScale->SetRange(1, this->SimpleAnimationCue->GetNumberOfKeyFrames());
    this->ShowKeyFrame(id);
    this->IndexScale->SetValue(id+1);
    this->UpdateEnableState();
    this->Script("grid %s - -row 1 -sticky ew", this->PropertiesFrame->GetWidgetName());
    }

  if (this->SimpleAnimationCue == NULL)
    {
    this->Script("grid forget %s", this->TitleLabel->GetWidgetName());
    this->Script("grid forget %s", this->TitleLabelLabel->GetWidgetName());
    this->SetAddDeleteButtonVisibility(0);
    }
  else

    {
    this->Script("grid %s %s -row 0 -sticky w", this->TitleLabelLabel->GetWidgetName(),
      this->TitleLabel->GetWidgetName());
    this->SetAddDeleteButtonVisibility(!this->SimpleAnimationCue->GetVirtual());

    }
}

//-----------------------------------------------------------------------------
void vtkPVTrackEditor::SetAddDeleteButtonVisibility(int visible)
{
  if (visible)
    {
    this->Script("grid %s x -row 3 -sticky w",
      this->AddKeyFrameButton->GetWidgetName());
    }
  else
    {
    this->Script("grid forget %s", this->AddKeyFrameButton->GetWidgetName());
    }

  if (visible)
    {
    this->Script("grid x %s -row 3 -sticky e",
      this->DeleteKeyFrameButton->GetWidgetName());
    }
  else
    {
    this->Script("grid forget %s", this->DeleteKeyFrameButton->GetWidgetName());
    }
  this->UpdateEnableState(); // so that delete buttons enable state is set properly.
}
//-----------------------------------------------------------------------------
void vtkPVTrackEditor::AddKeyFrameButtonCallback()
{
  if (!this->SimpleAnimationCue|| this->SimpleAnimationCue->GetVirtual())
    {
    vtkErrorMacro("Cannot delete any keyframe!");
    return;
    }
  this->SimpleAnimationCue->AppendNewKeyFrame();
  this->GetTraceHelper()->AddEntry("$kw(%s) AddKeyFrameButtonCallback", 
    this->GetTclName());
}

//-----------------------------------------------------------------------------
void vtkPVTrackEditor::DeleteKeyFrameButtonCallback()
{
  if (!this->SimpleAnimationCue || this->SimpleAnimationCue->GetVirtual())
    {
    vtkErrorMacro("Cannot delete any keyframe!");
    return;
    }
 
  int id = this->SimpleAnimationCue->GetSelectedKeyFrameIndex();
  if (id==-1)
    {
    vtkErrorMacro("No keyframe active. Cannot delete.");
    return;
    }
  this->SimpleAnimationCue->DeleteKeyFrame(id);
  this->Update();

  this->GetTraceHelper()->AddEntry("$kw(%s) DeleteKeyFrameButtonCallback", 
    this->GetTclName());
}

//-----------------------------------------------------------------------------
void vtkPVTrackEditor::SetKeyFrameType(int type)
{
  int id;
  if (!this->SimpleAnimationCue || !this->AnimationManager ||  
    this->SimpleAnimationCue->GetVirtual() ||
    (id = this->SimpleAnimationCue->GetSelectedKeyFrameIndex())==-1)
    {
    vtkWarningMacro("This method should not have been called at all");
    return;
    }

  this->GetTraceHelper()->AddEntry("$kw(%s) SetKeyFrameType %d", this->GetTclName(),
    type);

  this->AnimationManager->ReplaceKeyFrame(this->SimpleAnimationCue,
    type,  this->SimpleAnimationCue->GetKeyFrame(id));
  this->Update();
}

//-----------------------------------------------------------------------------
void vtkPVTrackEditor::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();
 
  this->PropagateEnableState(this->TypeMenuButton);
  this->PropagateEnableState(this->TypeImage);
  this->PropagateEnableState(this->IndexScale);
  this->PropagateEnableState(this->KeyFramePropertiesFrame);
  this->PropagateEnableState(this->ActiveKeyFrame);
  this->PropagateEnableState(this->AddKeyFrameButton);
  
  if (this->SimpleAnimationCue && 
    this->SimpleAnimationCue->CanDeleteSelectedKeyFrame())
    {
    this->PropagateEnableState(this->DeleteKeyFrameButton);
    }
  else
    {
    this->DeleteKeyFrameButton->SetEnabled(0);
    }

  this->TypeMenuButton->SetEnabled(
    !this->InterpolationValid ? 0 : this->GetEnabled());
  this->TypeImage->SetEnabled(
    !this->InterpolationValid ? 0 : this->GetEnabled()); 
}


//-----------------------------------------------------------------------------
void vtkPVTrackEditor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PropertiesFrame: " << this->PropertiesFrame << endl;
}
