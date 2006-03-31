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
#include "vtkKWFrameWithLabel.h"
#include "vtkKWFrame.h"
#include "vtkKWPushButton.h"
#include "vtkKWMenuButton.h"
#include "vtkKWScaleWithEntry.h"
#include "vtkKWLabel.h"
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

vtkStandardNewMacro(vtkPVTrackEditor);
vtkCxxRevisionMacro(vtkPVTrackEditor, "1.19");
//-----------------------------------------------------------------------------
class vtkPVTrackEditorObserver : public vtkCommand
{
public:
  static vtkPVTrackEditorObserver* New() { return new vtkPVTrackEditorObserver; }

  void SetTarget(vtkPVTrackEditor* t) 
    {
    this->Target = t;
    }

  virtual void Execute(vtkObject* , unsigned long , void* )
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
  this->KeyFramePropertiesFrame = vtkKWFrameWithLabel::New();
  this->PropertiesFrame = vtkKWFrame::New();
  this->SelectKeyFrameLabel = vtkKWLabel::New();
  
  this->TypeFrame = vtkKWFrame::New();
  this->TypeImage = vtkKWPushButton::New();
  this->TypeLabel = vtkKWLabel::New();
  this->TypeMenuButton = vtkKWMenuButton::New();
  
  this->AddKeyFrameButton = vtkKWPushButton::New();
  this->DeleteKeyFrameButton = vtkKWPushButton::New();

  this->SimpleAnimationCue = NULL;

  this->IndexScale = vtkKWScaleWithEntry::New();

  this->TitleLabelLabel = vtkKWLabel::New();
  this->TitleLabel = vtkKWLabel::New();
  this->InterpolationValid = 1;

  this->Observer = vtkPVTrackEditorObserver::New();
  this->Observer->SetTarget(this);
  this->ActiveKeyFrame = 0;
  this->FixedTimeKeyframeFlag = 
    vtkPVTrackEditor::FIRST_KEYFRAME_TIME_NOTCHANGABLE;
}

//-----------------------------------------------------------------------------
vtkPVTrackEditor::~vtkPVTrackEditor()
{
  this->Observer->SetTarget(0);
  this->Observer->Delete();

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
void vtkPVTrackEditor::Create()
{
  
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create();

  this->KeyFramePropertiesFrame->SetParent(this);
  this->KeyFramePropertiesFrame->Create();
  this->KeyFramePropertiesFrame->SetLabelText(
    VTK_PV_KEYFRAME_PROPERTIES_DEFAULT_LABEL);
  this->Script(
    "pack %s  -side top -anchor nw -fill x -expand t -padx 2 -pady 2", 
    this->KeyFramePropertiesFrame->GetWidgetName());

  this->TitleLabelLabel->SetParent(this->KeyFramePropertiesFrame->GetFrame());
  this->TitleLabelLabel->Create();
  this->TitleLabelLabel->SetText("Current Track:");
  
  this->TitleLabel->SetParent(this->KeyFramePropertiesFrame->GetFrame());
  this->TitleLabel->Create();
  vtkKWTkUtilities::ChangeFontWeightToBold(
    this->GetApplication()->GetMainInterp(), 
    this->TitleLabel->GetWidgetName());


  this->PropertiesFrame->SetParent(this->KeyFramePropertiesFrame->GetFrame());
  this->PropertiesFrame->Create();

  this->IndexScale->SetParent(this->PropertiesFrame);
  this->IndexScale->Create();
  this->IndexScale->SetResolution(1);
  this->IndexScale->SetLabelText("Index:");
  this->IndexScale->SetCommand(this, "IndexChangedCallback");
  this->IndexScale->SetEntryCommand(this, "IndexChangedCallback");
  this->IndexScale->SetEndCommand(this, "IndexChangedCallback");
  this->IndexScale->SetBalloonHelpString(
    "Select a key frame at a particular index in the "
    "current track");

  this->TypeLabel->SetParent(this->PropertiesFrame);
  this->TypeLabel->Create();
  this->TypeLabel->SetText("Interpolation:");

  this->TypeImage->SetParent(this->PropertiesFrame);
  this->TypeImage->Create();
  this->TypeImage->SetReliefToFlat();
  this->TypeImage->SetBalloonHelpString("Specify the type of interpolation "
                                        "starting at the active key frame.");

  this->TypeMenuButton->SetParent(this->PropertiesFrame);
  this->TypeMenuButton->Create();
  this->TypeMenuButton->SetReliefToFlat();
  this->TypeMenuButton->SetConfigurationOption(
    "-image", "PVToolbarPullDownArrow");
  this->TypeMenuButton->SetBalloonHelpString(
    "Specify the type of interpolation "
    "starting at the active key frame.");
  this->TypeMenuButton->IndicatorVisibilityOff();

  this->BuildTypeMenu();

  this->AddKeyFrameButton->SetParent(this->KeyFramePropertiesFrame->GetFrame());
  this->AddKeyFrameButton->Create();
  this->AddKeyFrameButton->SetBalloonHelpString("Append a new key frame");
  this->AddKeyFrameButton->SetText("Add KeyFrame");
  this->AddKeyFrameButton->SetCommand(this, "AddKeyFrameButtonCallback");

  this->DeleteKeyFrameButton->SetParent(
    this->KeyFramePropertiesFrame->GetFrame());
  this->DeleteKeyFrameButton->Create();
  this->DeleteKeyFrameButton->SetBalloonHelpString("Delete active key frame");
  this->DeleteKeyFrameButton->SetText("Delete KeyFrame");
  this->DeleteKeyFrameButton->SetCommand(this, "DeleteKeyFrameButtonCallback");

  this->SelectKeyFrameLabel->SetParent(
    this->KeyFramePropertiesFrame->GetFrame());
  this->SelectKeyFrameLabel->SetText("No source selected.");
  
  
  this->SelectKeyFrameLabel->Create();
  this->SelectKeyFrameLabel->SetJustificationToLeft();
  this->Script("grid %s - -row 1 -sticky ew", 
               this->SelectKeyFrameLabel->GetWidgetName());

  this->Script("grid %s - - -row 0 -sticky ew", 
               this->IndexScale->GetWidgetName());

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
  int index;
  const char* var = "Radio";
  
  index = menu->AddRadioButton(
    VTK_PV_RAMP_LABEL, this, "SetKeyFrameType 0"); 
  menu->SetItemGroupName(index, var);
  menu->SetItemSelectedValueAsInt(index, VTK_PV_RAMP_INDEX);
  menu->SetItemImage(index, "PVRamp"); 
  menu->SetItemHelpString(
    index, "Set the following Interpolator to Ramp.");

  index = menu->AddRadioButton(
    VTK_PV_STEP_LABEL, this, "SetKeyFrameType 1");
  menu->SetItemGroupName(index, var);
  menu->SetItemSelectedValueAsInt(index, VTK_PV_STEP_INDEX);
  menu->SetItemImage(index, "PVStep"); 
  menu->SetItemHelpString(
    index, "Set the following Interpolator to Step.");

  index = menu->AddRadioButton(
    VTK_PV_EXPONENTIAL_LABEL, this, "SetKeyFrameType 2");
  menu->SetItemGroupName(index, var);
  menu->SetItemSelectedValueAsInt(index, VTK_PV_EXPONENTIAL_INDEX);
  menu->SetItemImage(index, "PVExponential"); 
  menu->SetItemHelpString(
    index, "Set the following Interpolator to Exponential.");

  index = menu->AddRadioButton(
    VTK_PV_SINUSOID_LABEL, this, "SetKeyFrameType 3");
  menu->SetItemGroupName(index, var);
  menu->SetItemSelectedValueAsInt(index, VTK_PV_SINUSOID_INDEX);
  menu->SetItemImage(index, "PVSinusoid"); 
  menu->SetItemHelpString(
    index, "Set the following Interpolator to Sinusoid.");
}

//-----------------------------------------------------------------------------
void vtkPVTrackEditor::SetAnimationCue(vtkPVSimpleAnimationCue* cue)
{
  if (this->SimpleAnimationCue == cue)
    {
    return;
    }

  if (!cue)
    {
    this->SelectKeyFrameLabel->SetText("No source selected.");
    }
  else if (cue->GetVirtual())
    {
    this->SelectKeyFrameLabel->SetText("No property selected.");
    }
  else
    {
    this->SelectKeyFrameLabel->SetText("");
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
  double min_bound = 0.0;
  if (id > 0)
    {
    vtkPVKeyFrame* prev = this->SimpleAnimationCue->GetKeyFrame(id-1);
    if (prev)
      {
      min_bound = prev->GetKeyTime();
      }
    }
  pvKeyFrame->SetTimeMinimumBound(min_bound);

  double max_time = 1.0;
  if (id < this->SimpleAnimationCue->GetNumberOfKeyFrames()-1)
    {
    vtkPVKeyFrame* next = this->SimpleAnimationCue->GetKeyFrame(id+1);
    if (next)
      {
      max_time = next->GetKeyTime();
      }
    this->InterpolationValid = 1;
    }
  else
    {
    this->InterpolationValid = 0;// last key frame does not use Interpolation.
    }
  pvKeyFrame->SetTimeMaximumBound(max_time);
  pvKeyFrame->SetBlankTimeEntry(0);
 
  int time_changeable = 1;
  if (this->FixedTimeKeyframeFlag & vtkPVTrackEditor::FIRST_KEYFRAME_TIME_NOTCHANGABLE
    && id == 0)
    {
    time_changeable = 0;
    }
  if (this->FixedTimeKeyframeFlag & vtkPVTrackEditor::LAST_KEYFRAME_TIME_NOTCHANGABLE
    && id == this->SimpleAnimationCue->GetNumberOfKeyFrames()-1)
    {
    time_changeable = 0;
    pvKeyFrame->SetBlankTimeEntry(1);
    }
  pvKeyFrame->SetTimeChangeable(time_changeable);
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
    this->Script("grid %s - -row 1 -sticky ew", 
                 this->PropertiesFrame->GetWidgetName());
    this->Script("grid %s -columnspan 3 -row 1 -sticky ew",
      this->ActiveKeyFrame->GetWidgetName());
    }
  else
    {
    this->Script("grid %s - -row 1 -sticky ew", 
                 this->SelectKeyFrameLabel->GetWidgetName());
    }
}

//-----------------------------------------------------------------------------
void vtkPVTrackEditor::UpdateTypeImage(vtkPVKeyFrame* keyframe)
{
  const char *rbv = "Radio";
  if (vtkPVRampKeyFrame::SafeDownCast(keyframe))
    {
    this->TypeMenuButton->GetMenu()->SelectItemInGroupWithSelectedValueAsInt(
      rbv, VTK_PV_RAMP_INDEX);
    this->TypeImage->SetConfigurationOption("-image", "PVRamp");
    }
  else if (vtkPVBooleanKeyFrame::SafeDownCast(keyframe))
    {
    this->TypeMenuButton->GetMenu()->SelectItemInGroupWithSelectedValueAsInt(
      rbv, VTK_PV_STEP_INDEX);
    this->TypeImage->SetConfigurationOption("-image", "PVStep");
    }
  else if (vtkPVExponentialKeyFrame::SafeDownCast(keyframe))
    {
    this->TypeMenuButton->GetMenu()->SelectItemInGroupWithSelectedValueAsInt(
      rbv, VTK_PV_EXPONENTIAL_INDEX);
    this->TypeImage->SetConfigurationOption("-image", "PVExponential");
    }
  else if (vtkPVSinusoidKeyFrame::SafeDownCast(keyframe))
    {
    this->TypeMenuButton->GetMenu()->SelectItemInGroupWithSelectedValueAsInt(
      rbv, VTK_PV_SINUSOID_INDEX);
    this->TypeImage->SetConfigurationOption("-image", "PVSinusoid");
    }
  else
    {
    this->InterpolationValid = 0;
    }
}


//-----------------------------------------------------------------------------
void vtkPVTrackEditor::IndexChangedCallback(double value)
{
  int val = static_cast<int>(value) - 1;
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
    this->Script("grid %s - -row 1 -sticky ew", this->PropertiesFrame->GetWidgetName());
    this->UpdateEnableState();
    }

  if (this->SimpleAnimationCue == NULL)
    {
    this->Script("grid forget %s", this->TitleLabel->GetWidgetName());
    this->Script("grid forget %s", this->TitleLabelLabel->GetWidgetName());
    this->SetAddDeleteButtonVisibility(0);
    }
  else

    {
    this->Script("grid %s %s -row 0 -sticky w", 
                 this->TitleLabelLabel->GetWidgetName(),
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
  this->GetTraceHelper()->AddEntry("$kw(%s) AddKeyFrameButtonCallback", 
    this->GetTclName());
  this->SimpleAnimationCue->AppendNewKeyFrame();
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
  if (!this->SimpleAnimationCue ||   
    this->SimpleAnimationCue->GetVirtual() ||
    (id = this->SimpleAnimationCue->GetSelectedKeyFrameIndex())==-1)
    {
    vtkWarningMacro("This method should not have been called at all");
    return;
    }

  this->GetTraceHelper()->AddEntry("$kw(%s) SetKeyFrameType %d", this->GetTclName(),
    type);

  this->SimpleAnimationCue->ReplaceKeyFrame(type,  
    this->SimpleAnimationCue->GetKeyFrame(id));
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
  if (this->ActiveKeyFrame)
    {
    this->PropagateEnableState(this->ActiveKeyFrame);
    this->ActiveKeyFrame->UpdateEnableState();
    }
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
  os << indent << "TitleLabel: " << this->TitleLabel << endl;
  os << indent << "FixedTimeKeyframeFlag: 0x" << hex  
    << this->FixedTimeKeyframeFlag << endl;
}
