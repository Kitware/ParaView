/*=========================================================================

  Program:   ParaView
  Module:    vtkPVVerticalAnimationInterface.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVVerticalAnimationInterface.h"

#include "vtkObjectFactory.h"
#include "vtkKWApplication.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameLabeled.h"
#include "vtkKWLabel.h"
#include "vtkPVAnimationCue.h"
#include "vtkCommand.h"
#include "vtkKWParameterValueFunctionEditor.h"
#include "vtkPVTimeLine.h"
#include "vtkKWEvent.h"
#include "vtkPVKeyFrame.h"
#include "vtkKWMenuButton.h"
#include "vtkKWMenu.h"
#include "vtkKWPushButton.h"
#include "vtkPVRampKeyFrame.h"
#include "vtkPVBooleanKeyFrame.h"
#include "vtkPVExponentialKeyFrame.h"
#include "vtkPVSinusoidKeyFrame.h"

#include "vtkPVAnimationManager.h"
#include "vtkKWEntry.h"
#include "vtkKWScale.h"
#include "vtkKWCheckButton.h"
#include "vtkKWTkUtilities.h"
#include "vtkPVTraceHelper.h"

vtkStandardNewMacro(vtkPVVerticalAnimationInterface);
vtkCxxRevisionMacro(vtkPVVerticalAnimationInterface, "1.13");
vtkCxxSetObjectMacro(vtkPVVerticalAnimationInterface, ActiveKeyFrame, vtkPVKeyFrame);

#define VTK_PV_RAMP_INDEX 1
#define VTK_PV_RAMP_LABEL "Ramp"
#define VTK_PV_STEP_INDEX 2
#define VTK_PV_STEP_LABEL "Step"
#define VTK_PV_EXPONENTIAL_INDEX 3
#define VTK_PV_EXPONENTIAL_LABEL "Exponential"
#define VTK_PV_SINUSOID_INDEX 4
#define VTK_PV_SINUSOID_LABEL "Sinusoid"

#define VTK_PV_KEYFRAME_PROPERTIES_DEFAULT_LABEL "Active Key Frame Properties"

//*****************************************************************************
class vtkPVVerticalAnimationInterfaceObserver : public vtkCommand
{
public:
  static vtkPVVerticalAnimationInterfaceObserver* New()
    {
    return new vtkPVVerticalAnimationInterfaceObserver;
    }
  void SetVerticalAnimationInterface(vtkPVVerticalAnimationInterface* ob)
    {
    this->VerticalAnimationInterface = ob;
    }
  virtual void Execute(vtkObject* ob, unsigned long event, void* calldata)
    {
    if (this->VerticalAnimationInterface)
      {
      this->VerticalAnimationInterface->ExecuteEvent(ob, event, calldata);
      }
    }
protected:
  vtkPVVerticalAnimationInterfaceObserver()
    {
    this->VerticalAnimationInterface = NULL;
    }
  vtkPVVerticalAnimationInterface* VerticalAnimationInterface;
};

//*****************************************************************************
//-----------------------------------------------------------------------------
vtkPVVerticalAnimationInterface::vtkPVVerticalAnimationInterface()
{
  this->TopFrame = vtkKWFrame::New();
  this->KeyFramePropertiesFrame = vtkKWFrameLabeled::New();
  this->ScenePropertiesFrame = vtkKWFrameLabeled::New();
  this->SelectKeyFrameLabel = vtkKWLabel::New();
  this->PropertiesFrame = vtkKWFrame::New();
  this->TypeFrame = vtkKWFrame::New();
  this->TypeImage = vtkKWPushButton::New();
  this->TypeLabel = vtkKWLabel::New();
  this->TypeMenuButton = vtkKWMenuButton::New();
  
  this->RecordAllButton = vtkKWCheckButton::New();

  this->SaveFrame = vtkKWFrameLabeled::New();
  this->CacheGeometryCheck = vtkKWCheckButton::New();
  this->AdvancedAnimationCheck = vtkKWCheckButton::New();
 
  this->AnimationCue = NULL;
  this->Observer = vtkPVVerticalAnimationInterfaceObserver::New();
  this->Observer->SetVerticalAnimationInterface(this);
  this->AnimationManager = NULL;
  this->ActiveKeyFrame = NULL;

  this->IndexScale = vtkKWScale::New();
  this->CacheGeometry = 1;

  this->TitleLabelLabel = vtkKWLabel::New();
  this->TitleLabel = vtkKWLabel::New();
}

//-----------------------------------------------------------------------------
vtkPVVerticalAnimationInterface::~vtkPVVerticalAnimationInterface()
{
  this->Observer->Delete();
  this->SetActiveKeyFrame(NULL);
  this->SetAnimationCue(NULL);
  this->TopFrame->Delete();
  this->KeyFramePropertiesFrame->Delete();
  this->ScenePropertiesFrame->Delete();
  this->SelectKeyFrameLabel->Delete();
  this->PropertiesFrame->Delete();
  this->TypeFrame->Delete();
  this->TypeLabel->Delete();
  this->TypeImage->Delete();
  this->TypeMenuButton->Delete();
  this->SetAnimationManager(NULL);
  this->IndexScale->Delete();
  
  this->RecordAllButton->Delete();

  this->SaveFrame->Delete();
  this->CacheGeometryCheck->Delete();
  this->AdvancedAnimationCheck->Delete();
 
  this->TitleLabelLabel->Delete();
  this->TitleLabel->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVVerticalAnimationInterface::SetAnimationCue(vtkPVAnimationCue* cue)
{
  if (this->AnimationCue != cue)
    {
    if (this->AnimationCue)
      {
      this->RemoveObservers(this->AnimationCue);
      this->AnimationCue->UnRegister(this);
      this->TitleLabel->SetText("");
      }
    this->AnimationCue = cue;
    if (this->AnimationCue)
      {
      this->InitializeObservers(this->AnimationCue);
      this->AnimationCue->Register(this);
      char* text = this->AnimationCue->GetTextRepresentation();
      this->TitleLabel->SetText(text);
      delete []text;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVVerticalAnimationInterface::Create(vtkKWApplication* app,
  const char* args)
{
  if (!this->Superclass::Create(app, "frame", args ))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  if (!this->AnimationManager)
    {
    vtkErrorMacro("AnimationManager must be set");
    return;
    }
  this->TopFrame->SetParent(this);
  this->TopFrame->ScrollableOn();
  this->TopFrame->Create(app, 0);
  this->Script("pack %s -side top -fill both -expand t -anchor center",
    this->TopFrame->GetWidgetName());

  this->ScenePropertiesFrame->SetParent(this->TopFrame->GetFrame());
  this->ScenePropertiesFrame->ShowHideFrameOn();
  this->ScenePropertiesFrame->Create(app, 0);
  this->ScenePropertiesFrame->SetLabelText("Animation Control");
  this->Script("pack %s -side top -fill x -expand t -padx 2 -pady 2", 
    this->ScenePropertiesFrame->GetWidgetName());
 
  // KEYFRAME PROPERTIES FRAME
  this->KeyFramePropertiesFrame->SetParent(this->TopFrame->GetFrame());
  this->KeyFramePropertiesFrame->ShowHideFrameOn();
  this->KeyFramePropertiesFrame->Create(app, 0);
  this->KeyFramePropertiesFrame->SetLabelText(VTK_PV_KEYFRAME_PROPERTIES_DEFAULT_LABEL);
  this->Script("pack %s -side top -fill x -expand t -padx 2 -pady 2", 
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

  this->IndexScale->SetParent(this->PropertiesFrame->GetFrame());
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
  
  this->TypeLabel->SetParent(this->PropertiesFrame->GetFrame());
  this->TypeLabel->Create(app, 0);
  this->TypeLabel->SetText("Interpolation:");
 
  this->TypeImage->SetParent(this->PropertiesFrame->GetFrame());
  this->TypeImage->Create(app, "-relief flat");
  this->TypeImage->SetBalloonHelpString("Specify the type of interpolation "
    "following the active key frame.");
  
  this->TypeMenuButton->SetParent(this->PropertiesFrame->GetFrame());
  this->TypeMenuButton->Create(app, "-image PVToolbarPullDownArrow -relief flat");
  this->TypeMenuButton->SetBalloonHelpString("Specify the type of interpolation "
    "following the active key frame.");
  this->TypeMenuButton->IndicatorOff();

  this->BuildTypeMenu();
    
  this->SelectKeyFrameLabel->SetParent(this->KeyFramePropertiesFrame->GetFrame());
  this->SelectKeyFrameLabel->SetText("Select or Add a key frame in the Animation Tracks "
    "window to show its properties.");

  this->SelectKeyFrameLabel->Create(app, "-justify left");

  this->Script("grid %s - - -row 0 -sticky ew", this->IndexScale->GetWidgetName());
  
  this->Script("grid %s %s %s -columnspan 1 -row 2 -sticky w",
    this->TypeLabel->GetWidgetName(),
    this->TypeImage->GetWidgetName(),
    this->TypeMenuButton->GetWidgetName());

  this->Script("grid columnconfigure %s 2 -weight 2", this->PropertiesFrame->GetWidgetName());

  // SAVE FRAME
  this->SaveFrame->SetParent(this->TopFrame->GetFrame());
  this->SaveFrame->ShowHideFrameOn();
  this->SaveFrame->SetLabelText("Animation Settings");
  this->SaveFrame->Create(app, 0);
  this->Script("pack %s -side top -fill x -expand t -padx 2 -pady 2", 
    this->SaveFrame->GetWidgetName());


  this->CacheGeometryCheck->SetParent(this->SaveFrame->GetFrame());
  this->CacheGeometryCheck->Create(app, 0);
  this->CacheGeometryCheck->SetText("Cache Geometry");
  this->CacheGeometryCheck->SetCommand(this, "CacheGeometryCheckCallback");
  this->CacheGeometryCheck->SetState(this->CacheGeometry);
  this->CacheGeometryCheck->SetBalloonHelpString(
    "Specify caching of geometry for the animation. Note that cache can be "
    "used only in Sequence mode.");
  this->Script("grid %s x -sticky w", this->CacheGeometryCheck->GetWidgetName());

  this->RecordAllButton->SetParent(this->SaveFrame->GetFrame());
  this->RecordAllButton->Create(app, 0);
  this->RecordAllButton->SetText("Record All properties");
  this->RecordAllButton->SetState(this->AnimationManager->GetRecordAll());
  this->RecordAllButton->SetCommand(this, "RecordAllChangedCallback");
  this->RecordAllButton->SetBalloonHelpString("Specify if changes in all properties "
    "are to be recorded or only for the highlighted property.");
  this->Script("grid %s x -sticky w", this->RecordAllButton->GetWidgetName());

  this->AdvancedAnimationCheck->SetParent(this->SaveFrame->GetFrame());
  this->AdvancedAnimationCheck->Create(app, 0);
  this->AdvancedAnimationCheck->SetText("Show all animatable properties");
  this->AdvancedAnimationCheck->SetCommand(this, "AdvancedAnimationViewCallback");
  this->AdvancedAnimationCheck->SetState(this->AnimationManager->GetAdvancedView());
  this->AdvancedAnimationCheck->SetBalloonHelpString(
    "When checked, all properties that can be animated are shown. Otherwise only a "
    "small usually used subset of these properties are shown in the keyframe animation "
    "interface.");
  this->Script("grid %s x -sticky w", this->AdvancedAnimationCheck->GetWidgetName());

  this->Script("grid columnconfigure %s 2 -weight 2",
    this->SaveFrame->GetFrame()->GetWidgetName());

  this->Script("grid columnconfigure %s 1 -weight 2",
    this->KeyFramePropertiesFrame->GetFrame()->GetWidgetName());

}

//-----------------------------------------------------------------------------
void vtkPVVerticalAnimationInterface::BuildTypeMenu()
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
void vtkPVVerticalAnimationInterface::SetKeyFrameType(int type)
{
  int id;
  if (!this->AnimationCue || !this->AnimationManager ||  
    this->AnimationCue->GetVirtual() ||
    (id = this->AnimationCue->GetTimeLine()->GetSelectedPoint())==-1)
    {
    vtkWarningMacro("This method should not have been called at all");
    return;
    }
   
  this->GetTraceHelper()->AddEntry("$kw(%s) SetKeyFrameType %d", this->GetTclName(),
    type);

  this->AnimationManager->ReplaceKeyFrame(this->AnimationCue,
    type,  this->AnimationCue->GetKeyFrame(id));
  this->Update();
}

//-----------------------------------------------------------------------------
void vtkPVVerticalAnimationInterface::UpdateTypeImage(vtkPVKeyFrame* keyframe)
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
vtkKWFrame* vtkPVVerticalAnimationInterface::GetScenePropertiesFrame()
{
  if (!this->IsCreated())
    {
    vtkErrorMacro("Widget not created yet!");
    return NULL;
    }
  return this->ScenePropertiesFrame->GetFrame();
}

//-----------------------------------------------------------------------------
vtkKWFrame* vtkPVVerticalAnimationInterface::GetPropertiesFrame()
{
  if (!this->IsCreated())
    {
    vtkErrorMacro("Widget not created yet!");
    return NULL;
    }
  return this->PropertiesFrame;
}

//-----------------------------------------------------------------------------
void vtkPVVerticalAnimationInterface::CacheGeometryCheckCallback()
{
  this->SetCacheGeometry(this->CacheGeometryCheck->GetState());
}

//-----------------------------------------------------------------------------
void vtkPVVerticalAnimationInterface::SetCacheGeometry(int cache)
{
  if (cache == this->CacheGeometry)
    {
    return;
    }
  this->CacheGeometry = cache;
  this->CacheGeometryCheck->SetState(cache);
  this->AnimationManager->InvalidateAllGeometries();
  this->GetTraceHelper()->AddEntry("$kw(%s) SetCacheGeometry %d", this->GetTclName(), cache);
}

//-----------------------------------------------------------------------------
void vtkPVVerticalAnimationInterface::AdvancedAnimationViewCallback()
{
  this->SetAdvancedAnimationView(this->AdvancedAnimationCheck->GetState());
}

//-----------------------------------------------------------------------------
void vtkPVVerticalAnimationInterface::SetAdvancedAnimationView(int advanced)
{
  this->AnimationManager->SetAdvancedView(advanced);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetAdvancedAnimationView %d", this->GetTclName(),
    advanced);
}
//-----------------------------------------------------------------------------
void vtkPVVerticalAnimationInterface::InitializeObservers(vtkPVAnimationCue* cue)
{
  cue->AddObserver(vtkKWParameterValueFunctionEditor::SelectionChangedEvent,
    this->Observer);
}

//-----------------------------------------------------------------------------
void vtkPVVerticalAnimationInterface::RemoveObservers(vtkPVAnimationCue* cue)
{
  cue->RemoveObservers(vtkKWParameterValueFunctionEditor::SelectionChangedEvent,
    this->Observer);
  cue->RemoveObservers(vtkKWEvent::FocusOutEvent, this->Observer);
}

//-----------------------------------------------------------------------------
void vtkPVVerticalAnimationInterface::Update()
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
  int id;
  if (this->ActiveKeyFrame)
    {
    // unpack the old keyframe.
    this->Script("grid forget %s", this->ActiveKeyFrame->GetWidgetName());
    this->SetActiveKeyFrame(NULL);
    }

  if (this->AnimationCue == NULL || this->AnimationCue->GetVirtual() ||
    (id = this->AnimationCue->GetTimeLine()->GetSelectedPoint())==-1)
    {
    this->Script("grid forget %s", this->PropertiesFrame->GetWidgetName());
    this->Script("grid %s - -row 1 -sticky ew", this->SelectKeyFrameLabel->GetWidgetName());
    this->SetActiveKeyFrame(NULL);
    }
  else
    {
    this->IndexScale->SetRange(1, this->AnimationCue->GetNumberOfKeyFrames());
    this->ShowKeyFrame(id);
    this->IndexScale->SetValue(id+1);
    this->UpdateEnableState();
    this->Script("grid forget %s", this->SelectKeyFrameLabel->GetWidgetName());
    this->Script("grid %s - -row 1 -sticky ew", this->PropertiesFrame->GetWidgetName());
    }
  if (this->AnimationCue == NULL)
    {
    this->Script("grid forget %s", this->TitleLabel->GetWidgetName());
    this->Script("grid forget %s", this->TitleLabelLabel->GetWidgetName());
    }
  else
    
    {
    this->Script("grid %s %s -row 0 -sticky w", this->TitleLabelLabel->GetWidgetName(),
      this->TitleLabel->GetWidgetName());
    }
}

//-----------------------------------------------------------------------------
void vtkPVVerticalAnimationInterface::ShowKeyFrame(int id)
{
  vtkPVKeyFrame* pvKeyFrame = this->AnimationCue->GetKeyFrame(id);
  this->SetActiveKeyFrame(pvKeyFrame);
  if (!pvKeyFrame)
    {
    vtkErrorMacro("Failed to get the keyframe");
    return;
    }
  //Lets try to determine time bounds if any for this keyframe.

  //Get rid of old times.
  pvKeyFrame->ClearTimeBounds();
  if (id > 0)
    {
    vtkPVKeyFrame* prev = this->AnimationCue->GetKeyFrame(id-1);
    if (prev)
      {
      pvKeyFrame->SetTimeMinimumBound(prev->GetKeyTime());
      }
    }

  if (id < this->AnimationCue->GetNumberOfKeyFrames()-1)
    {
    vtkPVKeyFrame* next = this->AnimationCue->GetKeyFrame(id+1);
    if (next)
      {
      pvKeyFrame->SetTimeMaximumBound(next->GetKeyTime());
      }
    }
  pvKeyFrame->PrepareForDisplay();
  this->UpdateTypeImage(pvKeyFrame);
  
  this->Script("grid %s -columnspan 3 -row 1 -sticky ew",
    pvKeyFrame->GetWidgetName());
}

//-----------------------------------------------------------------------------
void vtkPVVerticalAnimationInterface::ExecuteEvent(vtkObject*, unsigned long, void* )
{
  this->Update();
}

//-----------------------------------------------------------------------------
void vtkPVVerticalAnimationInterface::IndexChangedCallback()
{
  int val = this->IndexScale->GetEntry()->GetValueAsInt() - 1;
  this->SetKeyFrameIndex(val); 
}

//-----------------------------------------------------------------------------
void vtkPVVerticalAnimationInterface::SetKeyFrameIndex(int val)
{
  if (!this->AnimationCue || this->AnimationCue->GetVirtual())
    {
    return;
    } 
  if (val <0 || val >= this->AnimationCue->GetNumberOfKeyFrames())
    {
    return;
    }
  this->AnimationCue->GetTimeLine()->SelectPoint(val);
  this->IndexScale->SetValue(val+1);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetKeyFrameIndex %d", this->GetTclName(), val);
}

//-----------------------------------------------------------------------------
void vtkPVVerticalAnimationInterface::RecordAllChangedCallback()
{
  int state = this->RecordAllButton->GetState();
  this->AnimationManager->SetRecordAll(state);
}


//-----------------------------------------------------------------------------
void vtkPVVerticalAnimationInterface::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();
  this->PropagateEnableState(this->TypeMenuButton);
  this->PropagateEnableState(this->TypeImage);
  this->PropagateEnableState(this->IndexScale);
  this->PropagateEnableState(this->RecordAllButton);
  this->PropagateEnableState(this->ScenePropertiesFrame);
  this->PropagateEnableState(this->KeyFramePropertiesFrame);
  this->PropagateEnableState(this->SelectKeyFrameLabel);
  if (this->ActiveKeyFrame)
    {
    this->PropagateEnableState(this->ActiveKeyFrame);
    }
}

//-----------------------------------------------------------------------------
void vtkPVVerticalAnimationInterface::SaveState(ofstream* )
{
}

//-----------------------------------------------------------------------------
void vtkPVVerticalAnimationInterface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AnimationManager: " << this->AnimationManager << endl;
  os << indent << "AnimationCue: " << this->AnimationCue << endl;
  os << indent << "ActiveKeyFrame: " << this->ActiveKeyFrame << endl;
  os << indent << "CacheGeometry: " << this->CacheGeometry << endl;
}
