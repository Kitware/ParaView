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
#include "vtkKWFrameWithScrollbar.h"
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

#include "vtkPVTrackEditor.h"
#include "vtkPVAnimationManager.h"
#include "vtkKWEntry.h"
#include "vtkKWScale.h"
#include "vtkKWCheckButton.h"
#include "vtkKWTkUtilities.h"
#include "vtkPVTraceHelper.h"

vtkStandardNewMacro(vtkPVVerticalAnimationInterface);
vtkCxxRevisionMacro(vtkPVVerticalAnimationInterface, "1.19");

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
vtkPVVerticalAnimationInterface::vtkPVVerticalAnimationInterface()
{
  this->TopFrame = vtkKWFrameWithScrollbar::New();
  this->ScenePropertiesFrame = vtkKWFrameLabeled::New();

  this->RecordAllButton = vtkKWCheckButton::New();
  this->SelectorFrame = vtkKWFrameLabeled::New();

  this->SaveFrame = vtkKWFrameLabeled::New();
  this->CacheGeometryCheck = vtkKWCheckButton::New();
  this->AdvancedAnimationCheck = vtkKWCheckButton::New();
 
  this->AnimationManager = NULL;

  this->CacheGeometry = 1;
  this->EnableCacheCheckButton = 1;

  this->TrackEditor = vtkPVTrackEditor::New();
  this->TrackEditor->GetTraceHelper()->SetReferenceHelper(
    this->GetTraceHelper());
  this->TrackEditor->GetTraceHelper()->SetReferenceCommand(
    "GetTrackEditor");
}

//-----------------------------------------------------------------------------
vtkPVVerticalAnimationInterface::~vtkPVVerticalAnimationInterface()
{
  this->SetAnimationCue(NULL);
  this->SetAnimationManager(NULL);
  
  this->TopFrame->Delete();
  this->ScenePropertiesFrame->Delete();
 
  this->RecordAllButton->Delete();

  this->SelectorFrame->Delete();

  this->SaveFrame->Delete();
  this->CacheGeometryCheck->Delete();
  this->AdvancedAnimationCheck->Delete();

  this->TrackEditor->Delete();
 
}

//-----------------------------------------------------------------------------
void vtkPVVerticalAnimationInterface::SetAnimationCue(vtkPVAnimationCue* cue)
{
  this->TrackEditor->SetAnimationCue(cue);
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
  this->TopFrame->Create(app, 0);
  this->Script(
    // "pack %s -side top -expand t", // -fill both  -anchor center",
    "pack %s -pady 2 -fill both -expand yes -anchor n",
    this->TopFrame->GetWidgetName());

  this->ScenePropertiesFrame->SetParent(this->TopFrame->GetFrame());
  this->ScenePropertiesFrame->ShowHideFrameOn();
  this->ScenePropertiesFrame->Create(app, 0);
  this->ScenePropertiesFrame->SetLabelText("Animation Control");
  this->Script(
    "pack %s  -side top -anchor nw -fill x -expand t -padx 2 -pady 2", // 
    this->ScenePropertiesFrame->GetWidgetName());

  // SELECTOR FRAME
  this->SelectorFrame->SetParent(this->TopFrame->GetFrame());
  this->SelectorFrame->ShowHideFrameOn();
  this->SelectorFrame->Create(app, 0);
  this->SelectorFrame->SetLabelText(VTK_PV_SELECTOR_DEFAULT_LABEL); 
  this->Script(
    "pack %s -side top -anchor nw  -fill x -expand y -padx 2 -pady 2",
    this->SelectorFrame->GetWidgetName());
  
  // KEYFRAME PROPERTIES FRAME
  this->TrackEditor->SetParent(this->TopFrame->GetFrame());
  this->TrackEditor->SetAnimationManager(this->AnimationManager);
  this->TrackEditor->Create(app, 0);
  this->Script(
    "pack %s  -side top -anchor nw -fill x -expand t -padx 2 -pady 2", 
    this->TrackEditor->GetWidgetName());
  
  // SAVE FRAME
  this->SaveFrame->SetParent(this->TopFrame->GetFrame());
  this->SaveFrame->ShowHideFrameOn();
  this->SaveFrame->SetLabelText("Animation Settings");
  this->SaveFrame->Create(app, 0);
  this->Script(
    "pack %s  -side top -anchor nw -fill x -expand t -padx 2 -pady 2", // 
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
vtkKWFrame* vtkPVVerticalAnimationInterface::GetSelectorFrame()
{
  if (!this->IsCreated())
    {
    vtkErrorMacro("Widget not created yet!");
    return NULL;
    }
  return this->SelectorFrame->GetFrame();
}

//-----------------------------------------------------------------------------
vtkKWFrame* vtkPVVerticalAnimationInterface::GetPropertiesFrame()
{
  if (!this->IsCreated())
    {
    vtkErrorMacro("Widget not created yet!");
    return NULL;
    }
  return this->TrackEditor->GetPropertiesFrame();
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
  this->AnimationManager->SetCacheGeometry(cache);
  this->CacheGeometry = this->AnimationManager->GetCacheGeometry();
  this->CacheGeometryCheck->SetState(this->CacheGeometry);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetCacheGeometry %d", 
    this->GetTclName(), cache);
}

//-----------------------------------------------------------------------------
void vtkPVVerticalAnimationInterface::EnableCacheCheck()
{
  this->EnableCacheCheckButton = 1;
  this->AnimationManager->SetCacheGeometry(
    this->CacheGeometryCheck->GetState());
  this->UpdateEnableState();
}

//-----------------------------------------------------------------------------
void vtkPVVerticalAnimationInterface::DisableCacheCheck()
{
  this->EnableCacheCheckButton = 0;
  this->AnimationManager->SetCacheGeometry(0);
  this->UpdateEnableState();
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
void vtkPVVerticalAnimationInterface::RecordAllChangedCallback()
{
  int state = this->RecordAllButton->GetState();
  this->AnimationManager->SetRecordAll(state);
}


//-----------------------------------------------------------------------------
void vtkPVVerticalAnimationInterface::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();
  this->PropagateEnableState(this->RecordAllButton);
  this->PropagateEnableState(this->ScenePropertiesFrame);
  this->PropagateEnableState(this->TrackEditor);
  
  if (this->CacheGeometryCheck)
    {
    this->CacheGeometryCheck->SetEnabled(
      !this->EnableCacheCheckButton ? 0 : this->GetEnabled());
    }
}

//-----------------------------------------------------------------------------
void vtkPVVerticalAnimationInterface::Update()
{
  this->TrackEditor->Update();

}

//-----------------------------------------------------------------------------
void vtkPVVerticalAnimationInterface::SaveState(ofstream* )
{
  // Nothing to save
}

//-----------------------------------------------------------------------------
void vtkPVVerticalAnimationInterface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AnimationManager: " << this->AnimationManager << endl;
  os << indent << "CacheGeometry: " << this->CacheGeometry << endl;
  os << indent << "TrackEditor: " << this->TrackEditor << endl;
}
