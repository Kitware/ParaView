/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAnimationScene.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVAnimationScene.h"

#include "vtkAnimationScene.h"
#include "vtkCommand.h"
#include "vtkErrorCode.h"
#include "vtkKWCheckButton.h"
#include "vtkKWEntry.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameLabeled.h"
#include "vtkKWIcon.h"
#include "vtkKWLabel.h"
#include "vtkKWMenuButton.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWPushButton.h"
#include "vtkKWScale.h"
#include "vtkKWThumbWheel.h"
#include "vtkKWToolbarSet.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationCue.h"
#include "vtkPVAnimationManager.h"
#include "vtkPVApplication.h"
#include "vtkPVRenderView.h"
#include "vtkPVSource.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVVCRControl.h"
#include "vtkPVWindow.h"
#include "vtkProcessModule.h"
#include "vtkRenderWindow.h"
#include "vtkSMAnimationCueProxy.h"
#include "vtkSMAnimationSceneProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSimpleDisplayProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVSource.h"
#include "vtkErrorCode.h"
#include "vtkPVVCRControl.h"
#include "vtkKWToolbarSet.h"
#include "vtkPVTraceHelper.h"

// Some header file is defining CurrentTime so undef it
#undef CurrentTime

#ifdef _WIN32
  #include "vtkAVIWriter.h"
#endif

#if !defined(_WIN32) || defined(__CYGWIN__)
# include <unistd.h> /* unlink */
#else
# include <io.h> /* unlink */
#endif

vtkStandardNewMacro(vtkPVAnimationScene);
vtkCxxRevisionMacro(vtkPVAnimationScene, "1.31");
#define VTK_PV_PLAYMODE_SEQUENCE_TITLE "Sequence"
#define VTK_PV_PLAYMODE_REALTIME_TITLE "Real Time"

#define VTK_PV_TOOLBARS_ANIMATION_LABEL "Animation"

//*****************************************************************************
class vtkPVAnimationSceneObserver : public vtkCommand
{
public:
  static vtkPVAnimationSceneObserver* New()
    {
    return new vtkPVAnimationSceneObserver;
    }
  void SetAnimationScene(vtkPVAnimationScene* scene)
    {
    this->AnimationScene = scene;
    }
  virtual void Execute(vtkObject* obj, unsigned long event, void* calldata)
    {
    if (this->AnimationScene)
      {
      this->AnimationScene->ExecuteEvent(obj, event, calldata);
      }
    }
protected:
  vtkPVAnimationSceneObserver()
    {
    this->AnimationScene = 0;
    }
  vtkPVAnimationScene* AnimationScene;
};

//*****************************************************************************
//Helper methods to down cast the property and set value.
inline int DoubleVectPropertySetElement(vtkSMProxy *proxy, 
  const char* propertyname, double val, int index = 0)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    proxy->GetProperty(propertyname));
  if (!dvp)
    {
    return 0;
    }
  return dvp->SetElement(index, val);
}

//-----------------------------------------------------------------------------
inline int StringVectPropertySetElement(vtkSMProxy *proxy, 
  const char* propertyname, const char* val, int index = 0)
{
  vtkSMStringVectorProperty* dvp = vtkSMStringVectorProperty::SafeDownCast(
    proxy->GetProperty(propertyname));
  if (!dvp)
    {
    return 0;
    }
  return dvp->SetElement(index, val);
}

//-----------------------------------------------------------------------------
inline int IntVectPropertySetElement(vtkSMProxy *proxy, 
  const char* propertyname, int val, int index = 0)
{
  vtkSMIntVectorProperty* dvp = vtkSMIntVectorProperty::SafeDownCast(
    proxy->GetProperty(propertyname));
  if (!dvp)
    {
    return 0;
    }
  return dvp->SetElement(index, val);
}

//-----------------------------------------------------------------------------
vtkPVAnimationScene::vtkPVAnimationScene()
{
  this->Observer = vtkPVAnimationSceneObserver::New();
  this->Observer->SetAnimationScene(this);
  this->AnimationSceneProxy = NULL;
  this->AnimationSceneProxyName = NULL;

  this->VCRControl = vtkPVVCRControl::New();
  this->VCRToolbar = vtkPVVCRControl::New();

  this->TimeLabel = vtkKWLabel::New();
  this->TimeScale = vtkKWScale::New();
  this->DurationLabel = vtkKWLabel::New();
  this->DurationThumbWheel = vtkKWThumbWheel::New();
  this->PlayModeMenuButton = vtkKWMenuButton::New();
  this->PlayModeLabel = vtkKWLabel::New();

  this->RenderView = NULL;
  this->AnimationManager = NULL;
  this->Window = NULL;
  this->ErrorEventTag = 0;
  this->InPlay  = 0;

  this->InvokingError = 0;
}

//-----------------------------------------------------------------------------
vtkPVAnimationScene::~vtkPVAnimationScene()
{
  if (this->AnimationSceneProxyName)
    {
    vtkSMObject::GetProxyManager()->UnRegisterProxy("animation_scene",
      this->AnimationSceneProxyName);
    this->SetAnimationSceneProxyName(0);
    }
  if (this->AnimationSceneProxy)
    {
    this->AnimationSceneProxy->Delete();
    this->AnimationSceneProxy = 0;
    }
  this->SetWindow(NULL);
  this->Observer->Delete();
  this->VCRControl->Delete();
  this->VCRToolbar->Delete();

  this->TimeLabel->Delete();
  this->TimeScale->Delete();
  this->DurationLabel->Delete();
  this->DurationThumbWheel->Delete();
  this->PlayModeMenuButton->Delete();
  this->PlayModeLabel->Delete();
  this->SetRenderView(NULL);
  this->SetAnimationManager(NULL);

}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::PrepareForDelete()
{
  if (this->AnimationSceneProxy)
    {
    this->AnimationSceneProxy->Stop();
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      this->AnimationSceneProxy->GetProperty("RenderModule"));
    pp->RemoveAllProxies();
    this->AnimationSceneProxy->UpdateVTKObjects();
    }
  this->SetRenderView(0);
  this->SetAnimationManager(0);
  this->SetWindow(0);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SetAnimationManager(vtkPVAnimationManager* am)
{
  this->AnimationManager = am;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SetRenderView(vtkPVRenderView* view)
{
  this->RenderView = view;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SetWindow(vtkPVWindow *window)
{
  if ( this->Window == window )
    {
    return;
    }
  if ( this->Window )
    {
    this->Window->RemoveObserver(this->ErrorEventTag);
    }
  this->Window = window;
  if ( this->Window )
    {
    this->ErrorEventTag = this->Window->AddObserver(vtkKWEvent::ErrorMessageEvent, this->Observer);
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::Create(vtkKWApplication* app, const char* args)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("Widget already created.");
    return;
    }
  if (!this->AnimationManager)
    {
    vtkErrorMacro("AnimationManager must be set");
    return;
    }
  if (!this->Window)
    {
    vtkErrorMacro("Window must be set before create.");
    return;
    }
  if (!this->RenderView)
    {
    vtkErrorMacro("RenderView must be set before create.");
    return;
    }
  this->Superclass::Create(app, "frame", args);

  this->CreateProxy();

  //vtkKWIcon* icon = vtkKWIcon::New();
  
  this->Script("grid propagate %s 1",
    this->GetWidgetName());
 
  this->VCRControl->SetParent(this);
  this->VCRControl->Create(app);
  this->VCRControl->SetPlayCommand(this, "Play");
  this->VCRControl->SetStopCommand(this, "Stop");
  this->VCRControl->SetGoToBeginningCommand(this, "GoToBeginning");
  this->VCRControl->SetGoToEndCommand(this,"GoToEnd");
  this->VCRControl->SetGoToPreviousCommand(this, "GoToPrevious");
  this->VCRControl->SetGoToNextCommand(this,"GoToNext");
  this->VCRControl->SetLoopCheckCommand(this,"LoopCheckButtonCallback");
  this->VCRControl->SetRecordCheckCommand(this, "RecordCheckCallback");
  this->VCRControl->SetRecordStateCommand(this, "RecordState");
  this->VCRControl->SetSaveAnimationCommand(this, "SaveAnimationCallback");
  this->Script("grid %s -columnspan 2 -sticky {}",
    this->VCRControl->GetWidgetName());
  this->VCRControl->UpdateEnableState();

  this->VCRToolbar->SetParent(this->Window->GetLowerToolbars()->GetToolbarsFrame());
  this->VCRToolbar->Create(app);
  this->VCRToolbar->SetPlayCommand(this, "Play");
  this->VCRToolbar->SetStopCommand(this, "Stop");
  this->VCRToolbar->SetGoToBeginningCommand(this, "GoToBeginning");
  this->VCRToolbar->SetGoToEndCommand(this,"GoToEnd");
  this->VCRToolbar->SetGoToPreviousCommand(this, "GoToPrevious");
  this->VCRToolbar->SetGoToNextCommand(this,"GoToNext");
  this->VCRToolbar->SetLoopCheckCommand(this,"ToolbarLoopCheckButtonCallback");
  this->VCRToolbar->SetRecordCheckCommand(this, "ToolbarRecordCheckButtonCallback");
  this->VCRToolbar->SetRecordStateCommand(this, "RecordState");
  this->VCRToolbar->SetSaveAnimationCommand(this, "SaveAnimationCallback");
  this->Window->AddLowerToolbar(this->VCRToolbar, VTK_PV_TOOLBARS_ANIMATION_LABEL, 0);
  this->VCRToolbar->UpdateEnableState();

  // Animation Control: Time scale
  this->TimeLabel->SetParent(this);
  this->TimeLabel->Create(app, 0);
  this->TimeLabel->SetText("Current Time:");

  this->TimeScale->SetParent(this);
  this->TimeScale->Create(app, "");
  this->TimeScale->DisplayEntry();
  this->TimeScale->DisplayEntryAndLabelOnTopOff();
  this->TimeScale->SetResolution(0.01);
  this->TimeScale->SetEndCommand(this, "TimeScaleCallback");
  this->TimeScale->SetEntryCommand(this, "TimeScaleCallback");
  this->TimeScale->SetBalloonHelpString("Adjust the current time "
    "(in seconds).");
  this->Script("grid %s %s -sticky ew",
    this->TimeLabel->GetWidgetName(),
    this->TimeScale->GetWidgetName());


  this->DurationLabel->SetParent(this);
  this->DurationLabel->Create(app, 0);
  this->DurationLabel->SetText("Duration:");
  
  this->DurationThumbWheel->SetParent(this);
  this->DurationThumbWheel->PopupModeOn();
  this->DurationThumbWheel->ClampMinimumValueOn();
  this->DurationThumbWheel->SetMinimumValue(1.0);
  this->DurationThumbWheel->Create(app, NULL);
  this->DurationThumbWheel->DisplayEntryOn();
  this->DurationThumbWheel->DisplayEntryAndLabelOnTopOff();
  this->DurationThumbWheel->ExpandEntryOn();
  this->DurationThumbWheel->SetEntryCommand(this, "DurationChangedCallback");
  this->DurationThumbWheel->SetEndCommand(this, "DurationChangedCallback");
  this->DurationThumbWheel->GetEntry()->BindCommand(this, 
    "DurationChangedCallback");
  this->DurationThumbWheel->GetEntry()->SetBind("<KeyRelease>", this->GetTclName(),
    "DurationChangedKeyReleaseCallback");
  this->DurationThumbWheel->SetBalloonHelpString("Adjust the duration for "
    "the animation (in seconds).");
  this->SetDuration(10.0);
  this->Script("grid %s %s -sticky ew",
    this->DurationLabel->GetWidgetName(),
    this->DurationThumbWheel->GetWidgetName());

  // Animation Control: Play Mode
  this->PlayModeLabel->SetParent(this);
  this->PlayModeLabel->Create(app, 0);
  this->PlayModeLabel->SetText("Play Mode:" );
  this->PlayModeMenuButton->SetParent(this);
  this->PlayModeMenuButton->Create(app, 0);
  this->PlayModeMenuButton->SetBalloonHelpString("Change the mode in which the "
    "animation is played.");
  this->PlayModeMenuButton->AddCommand(VTK_PV_PLAYMODE_SEQUENCE_TITLE, this,
    "SetPlayMode 0", "Plays the animation as a sequence of images.");
  this->PlayModeMenuButton->AddCommand(VTK_PV_PLAYMODE_REALTIME_TITLE, this,
    "SetPlayMode 1", "Plays the animation in real time mode.");
  this->SetPlayModeToSequence();
  
  this->Script("grid %s %s -sticky ew",
    this->PlayModeLabel->GetWidgetName(),
    this->PlayModeMenuButton->GetWidgetName());

  this->Script("grid columnconfigure %s 0 -weight 0",
    this->GetWidgetName());
  this->Script("grid columnconfigure %s 1 -weight 2",
    this->GetWidgetName());

  // Setup key bindings 
  // Quick Keys!  (Left arrow for one step back, right arrow for one step
  // forward, up arrow for last time step, down arrow for first time step). :)
  this->Script("bind %s <Key-Left> {%s GoToPrevious}",
    this->Window->GetMainView()->GetWidgetName(), this->GetTclName());
  this->Script("bind %s <Key-Right> {%s GoToNext}",
    this->Window->GetMainView()->GetWidgetName(), this->GetTclName());
  this->Script("bind %s <Key-Up> {%s GoToEnd}",
    this->Window->GetMainView()->GetWidgetName(), this->GetTclName());
  this->Script("bind %s <Key-Down> {%s GoToBeginning}",
    this->Window->GetMainView()->GetWidgetName(), this->GetTclName());
  
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::CreateProxy()
{
  static int proxyNum = 0;
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  this->AnimationSceneProxy = vtkSMAnimationSceneProxy::SafeDownCast(
    pxm->NewProxy("animation", "AnimationScene"));
  if (!this->AnimationSceneProxy)
    {
    vtkErrorMacro("Failed to create proxy AnimationScene");
    return;
    }
  ostrstream str;
  str << "vtkPVAnimationScene_AnimationScene" << proxyNum << ends;
  this->SetAnimationSceneProxyName(str.str());
  proxyNum++;
  str.rdbuf()->freeze(0);
  pxm->RegisterProxy("animation_scene", this->AnimationSceneProxyName,
    this->AnimationSceneProxy);
 
  this->AnimationSceneProxy->AddObserver(vtkCommand::StartAnimationCueEvent,
    this->Observer);
  this->AnimationSceneProxy->AddObserver(vtkCommand::AnimationCueTickEvent,
    this->Observer);
  this->AnimationSceneProxy->AddObserver(vtkCommand::EndAnimationCueEvent,
    this->Observer);
  
  DoubleVectPropertySetElement(this->AnimationSceneProxy,"StartTime",0.0);
  DoubleVectPropertySetElement(this->AnimationSceneProxy,"EndTime", 60.0);
  DoubleVectPropertySetElement(this->AnimationSceneProxy,"TimeMode",
    VTK_ANIMATION_CUE_TIMEMODE_RELATIVE);
  DoubleVectPropertySetElement(this->AnimationSceneProxy, "FrameRate", 1.0);

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->AnimationSceneProxy->GetProperty("RenderModule"));
  pp->AddProxy(this->RenderView->GetRenderModuleProxy());

  this->AnimationSceneProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SaveImages(const char* fileRoot, const char* ext, 
  int width, int height, double framerate)
{
  this->GetTraceHelper()->AddEntry("$kw(%s) SaveImages \"%s\" \"%s\" %d %d %f",
    this->GetTclName(), fileRoot, ext, width, height, framerate);
  int savefailed = this->AnimationSceneProxy->SaveImages(fileRoot, ext, 
    width, height, framerate);
  
  if (savefailed)
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetApplication(), this->Window, "Write Error",
      "There is insufficient disk space to save the images for this "
      "animation. The file(s) already written will be deleted.");
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SaveGeometry(const char* filename)
{
  // Start at the beginning.
  this->GetTraceHelper()->AddEntry("$kw(%s) SaveGeometry %s", this->GetTclName(), filename);
  int error = this->AnimationSceneProxy->SaveGeometry(filename);
  if (error == vtkErrorCode::OutOfDiskSpaceError)
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetApplication(), this->Window,
      "Write Error", "There is insufficient disk space to save the geometry "
      "for this animation. The file(s) already written will be deleted.");
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::ExecuteEvent(vtkObject* , unsigned long event,
  void* calldata)
{
  if (event == vtkKWEvent::ErrorMessageEvent && !this->InvokingError)
    {
    this->InvokingError = 1;
    this->Stop();
    this->InvokingError = 0;
    return;
    }
  
  vtkAnimationCue::AnimationCueInfo *cueInfo = reinterpret_cast<
    vtkAnimationCue::AnimationCueInfo*>(calldata);

  switch(event)
    {
  case vtkCommand::StartAnimationCueEvent:
    break;
  case vtkCommand::EndAnimationCueEvent:
  case vtkCommand::AnimationCueTickEvent:
      {
      
      double etime = this->AnimationSceneProxy->GetEndTime();
      double stime = this->AnimationSceneProxy->GetStartTime();
      double ntime = 
        (etime==stime)?  0 : (cueInfo->CurrentTime - stime) / (etime - stime);
      this->AnimationManager->SetTimeMarker(ntime);
      this->TimeScale->SetValue(cueInfo->CurrentTime);
      }
    break;
    }
  this->Script("update");
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::DurationChangedCallback()
{
  double duration = this->DurationThumbWheel->GetEntry()->GetValueAsFloat();
  this->SetDuration(duration);
}

//-----------------------------------------------------------------------------
double vtkPVAnimationScene::GetDuration()
{
  return this->AnimationSceneProxy->GetEndTime();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SaveAnimationCallback()
{
  this->Window->SaveAnimation();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::DurationChangedKeyReleaseCallback()
{
  double duration = this->DurationThumbWheel->GetEntry()->GetValueAsFloat();
  if (duration >= 1.0)
    {
    this->SetDuration(duration);
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SetDuration(double duration)
{
  if (this->GetDuration() == duration)
    {
    return;
    }
  if (duration < 1.0)
    {
    duration = this->GetDuration();
    }
  double ntime = this->GetNormalizedCurrentTime();
  
  DoubleVectPropertySetElement(this->AnimationSceneProxy,"EndTime",duration);
  this->AnimationSceneProxy->UpdateVTKObjects();
  this->DurationThumbWheel->SetValue(duration);
  this->TimeScale->SetRange(0, duration);
  this->TimeScale->SetValue(duration*ntime);
  this->InvalidateAllGeometries();
  this->GetTraceHelper()->AddEntry("$kw(%s) SetDuration %f", this->GetTclName(), duration);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::Play()
{
  this->InPlay = 1;
  if (this->Window)
    {
    this->Window->UpdateEnableState();
    }
  this->VCRControl->SetInPlay(1);
  this->VCRControl->UpdateEnableState();
  this->VCRToolbar->SetInPlay(1);
  this->VCRToolbar->UpdateEnableState();
  
  this->AnimationSceneProxy->Play();
  this->InPlay = 0;
  if (this->Window)
    {
    this->Window->UpdateEnableState();
    }
  this->VCRControl->SetInPlay(0);
  this->VCRControl->UpdateEnableState();
  this->VCRToolbar->SetInPlay(0);
  this->VCRToolbar->UpdateEnableState();
  this->GetTraceHelper()->AddEntry("$kw(%s) Play", this->GetTclName());
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::Stop()
{
  this->AnimationSceneProxy->Stop();
  if (this->Window && this->Window->GetCurrentPVSource() )
    {
    this->Window->GetCurrentPVSource()->ResetCallback();
    }
  this->GetTraceHelper()->AddEntry("$kw(%s) Stop", this->GetTclName());
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::GoToBeginning()
{
  this->SetCurrentTime(0);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::GoToEnd()
{
  this->SetCurrentTime(this->AnimationSceneProxy->GetEndTime());
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::GoToNext()
{
  double time = this->TimeScale->GetValue();
  double duration = this->AnimationSceneProxy->GetEndTime();
  double newtime = time + 1.0/this->AnimationSceneProxy->GetFrameRate();
  newtime = (newtime > duration) ? duration : newtime;
  if (newtime != time)
    {
    this->SetCurrentTime(newtime);
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::GoToPrevious()
{
  double time = this->TimeScale->GetValue();
  double newtime = time - 1.0/this->AnimationSceneProxy->GetFrameRate();
  newtime = (newtime < 0) ? 0 : newtime;
  if (newtime != time)
    {
    this->SetCurrentTime(newtime); 
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SetPlayModeToSequence()
{
  this->SetPlayMode(VTK_ANIMATION_SCENE_PLAYMODE_SEQUENCE);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SetPlayModeToRealTime()
{
  this->SetPlayMode(VTK_ANIMATION_SCENE_PLAYMODE_REALTIME);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SetPlayMode(int mode)
{
  switch (mode)
    {
  case VTK_ANIMATION_SCENE_PLAYMODE_SEQUENCE:
    this->PlayModeMenuButton->SetButtonText(VTK_PV_PLAYMODE_SEQUENCE_TITLE);
    this->AnimationManager->EnableCacheCheck();
    // Change the time scale increment to 1.
    this->TimeScale->SetResolution(1);
    break;
  case VTK_ANIMATION_SCENE_PLAYMODE_REALTIME:
    this->PlayModeMenuButton->SetButtonText(VTK_PV_PLAYMODE_REALTIME_TITLE);
    this->AnimationManager->DisableCacheCheck();
      // disable cahce check when in real time mode.
      // Note that when we switch the mode to realtime,
      // the AnimationSceneProxy disables cacheing.
    this->TimeScale->SetResolution(0.01);
    break;
  default:
    vtkErrorMacro("Invalid play mode " << mode);
    return;
    }

  IntVectPropertySetElement(this->AnimationSceneProxy,"PlayMode", mode);
  this->AnimationSceneProxy->UpdateVTKObjects();
  this->GetTraceHelper()->AddEntry("$kw(%s) SetPlayMode %d", this->GetTclName(), mode);
}

//-----------------------------------------------------------------------------
int vtkPVAnimationScene::GetPlayMode()
{
  return this->AnimationSceneProxy->GetPlayMode();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::LoopCheckButtonCallback()
{
  this->SetLoop(this->VCRControl->GetLoopButtonState());
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::ToolbarLoopCheckButtonCallback()
{
  this->SetLoop(this->VCRToolbar->GetLoopButtonState());
}


//-----------------------------------------------------------------------------
void vtkPVAnimationScene::ToolbarRecordCheckButtonCallback()
{
  if (this->VCRToolbar->GetRecordCheckButtonState())
    {
    this->StartRecording();
    }
  else
    {
    this->StopRecording();
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::RecordCheckCallback()
{
  if (this->VCRControl->GetRecordCheckButtonState())
    {
    this->StartRecording();
    }
  else
    {
    this->StopRecording();
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::StartRecording()
{
  this->GetTraceHelper()->AddEntry("$kw(%s) StartRecording", this->GetTclName());
  this->AnimationManager->StartRecording();
  this->VCRToolbar->SetRecordCheckButtonState(1);
  this->VCRControl->SetRecordCheckButtonState(1);
  this->VCRControl->UpdateEnableState();
  this->VCRToolbar->UpdateEnableState();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::StopRecording()
{
  this->AnimationManager->StopRecording();
  this->VCRToolbar->SetRecordCheckButtonState(0);
  this->VCRControl->SetRecordCheckButtonState(0);
  this->VCRControl->UpdateEnableState();
  this->VCRToolbar->UpdateEnableState();
  this->GetTraceHelper()->AddEntry("$kw(%s) StopRecording", this->GetTclName());
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::RecordState()
{
  this->GetTraceHelper()->AddEntry("$kw(%s) RecordState", this->GetTclName());
  this->AnimationManager->RecordState();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SetFrameRate(double fps)
{
  if (this->GetFrameRate() == fps)
    {
    return;
    }
  if (fps <= 0 )
    {
    fps = this->GetFrameRate();
    }
  DoubleVectPropertySetElement(this->AnimationSceneProxy, "FrameRate", fps);
  this->AnimationSceneProxy->UpdateVTKObjects();
  this->InvalidateAllGeometries();
  this->GetTraceHelper()->AddEntry("$kw(%s) SetFrameRate %f", this->GetTclName(), fps);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SetCaching(int enable)
{
  IntVectPropertySetElement(this->AnimationSceneProxy, "Caching", enable);
  this->AnimationSceneProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
int vtkPVAnimationScene::GetCaching()
{
  return this->AnimationSceneProxy->GetCaching();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::InvalidateAllGeometries()
{
  this->AnimationSceneProxy->CleanCache();
}

//-----------------------------------------------------------------------------
double vtkPVAnimationScene::GetFrameRate()
{
  return this->AnimationSceneProxy->GetFrameRate();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SetLoop(int loop)
{
  if (this->GetLoop() == loop)
    {
    return;
    }
  this->VCRControl->SetLoopButtonState(loop);
  this->VCRToolbar->SetLoopButtonState(loop);
  IntVectPropertySetElement(this->AnimationSceneProxy, "Loop", loop);
  this->AnimationSceneProxy->UpdateVTKObjects();
  this->GetTraceHelper()->AddEntry("$kw(%s) SetLoop %d", this->GetTclName(), loop);
}

//-----------------------------------------------------------------------------
int vtkPVAnimationScene::GetLoop()
{
  return this->AnimationSceneProxy->GetLoop();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SetCurrentTime(double time)
{
  if (!this->IsCreated())
    {
    vtkErrorMacro("Scene has not been created yet.");
    return;
    }
  //Firstly, if the time resolution is 1, we round the time the time value.
  if (this->TimeScale->GetResolution() == 1)
    {
    time = (int)(time + 0.5);
    }
  DoubleVectPropertySetElement(this->AnimationSceneProxy, "CurrentTime", time);
  this->AnimationSceneProxy->UpdateVTKObjects();
  this->TimeScale->SetValue(time);
  if (this->Window && this->Window->GetCurrentPVSource())
    {
    this->Window->GetCurrentPVSource()->ResetCallback();
    }
  this->GetTraceHelper()->AddEntry("$kw(%s) SetCurrentTime %f", this->GetTclName(), time);
}

//-----------------------------------------------------------------------------
#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
# undef GetCurrentTime
// Define possible mangled names.
int vtkPVAnimationScene::GetTickCount()
{
  return this->GetCurrentTime();
}
#endif

//-----------------------------------------------------------------------------
double vtkPVAnimationScene::GetCurrentTime()
{
  return this->TimeScale->GetValue();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SetNormalizedCurrentTime(double ntime)
{
  if (!this->IsCreated())
    {
    vtkErrorMacro("Scene has not been created yet.");
    return;
    } 
  this->SetCurrentTime(ntime * this->GetDuration());
}

//-----------------------------------------------------------------------------
double vtkPVAnimationScene::GetNormalizedCurrentTime()
{
  return (this->GetCurrentTime() / this->GetDuration());
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::TimeScaleCallback()
{
  this->SetCurrentTime(this->TimeScale->GetValue());
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::AddAnimationCue(vtkPVAnimationCue *pvCue)
{
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->AnimationSceneProxy->GetProperty("Cues"));
  pp->AddProxy(pvCue->GetCueProxy());
  this->AnimationSceneProxy->UpdateVTKObjects();
  this->InvalidateAllGeometries();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::RemoveAnimationCue(vtkPVAnimationCue* pvCue)
{
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->AnimationSceneProxy->GetProperty("Cues"));
  pp->RemoveProxy(pvCue->GetCueProxy());
  this->AnimationSceneProxy->UpdateVTKObjects();
  this->InvalidateAllGeometries();
}

//-----------------------------------------------------------------------------
int vtkPVAnimationScene::IsInPlay()
{
  if (this->AnimationSceneProxy)
    {
    return ( this->InPlay || this->AnimationSceneProxy->IsInPlay());
    }
  return this->InPlay;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void vtkPVAnimationScene::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();
  if (!this->IsCreated())
    {
    return;
    }

  // These widgets are on when playing or when gui is enabled.

  int enabled = this->IsInPlay() || this->GetEnabled();
  if (this->VCRControl)
    {
    this->VCRControl->SetEnabled(enabled);
    }

  // These widgets are disabled when playing.

  enabled = !this->IsInPlay() && this->GetEnabled();

  if (this->DurationLabel)
    {
    this->DurationLabel->SetEnabled(enabled);
    }
  if (this->DurationThumbWheel)
    {
    this->DurationThumbWheel->SetEnabled(enabled);
    }
  if (this->PlayModeLabel)
    {
    this->PlayModeLabel->SetEnabled(enabled);
    }
  if (this->PlayModeMenuButton)
    {
    this->PlayModeMenuButton->SetEnabled(enabled);
    }
  if (this->TimeLabel)
    {
    this->TimeLabel->SetEnabled(enabled);
    }
  if (this->TimeScale)
    {
    this->TimeScale->SetEnabled(enabled);
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SaveState(ofstream* file)
{
  if (!this->IsCreated())
    {
    return;
    }
  *file << endl;
  *file << "# State for vtkPVAnimationScene" << endl;
  *file << "$kw(" << this->GetTclName() << ") SetDuration " <<
    this->GetDuration() << endl;
  *file << "$kw(" << this->GetTclName() << ") SetPlayMode " <<
    this->GetPlayMode() << endl;
  *file << "$kw(" << this->GetTclName() << ") SetFrameRate " <<
    this->GetFrameRate() << endl;
  *file << "$kw(" << this->GetTclName() << ") SetLoop " << 
    this->GetLoop() << endl;

  // NOTE: scene doesn't bother adding the cues, the cue add themselves 
  // to the scene.
  *file << "$kw(" << this->GetTclName() << ") SetCurrentTime " << 
    this->TimeScale->GetValue() << endl;
  
  //TODO: add all the addded cues and then set the current time.
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SaveInBatchScript(ofstream* file)
{
  if (this->AnimationSceneProxy)
    {
    this->AnimationSceneProxy->SaveInBatchScript(file);
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SetAnimationToolbarVisibility(int visible)
{
  this->Window->SetLowerToolbarVisibility(this->VCRToolbar, 
    VTK_PV_TOOLBARS_ANIMATION_LABEL, visible);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RenderView: " << this->RenderView << endl;
  os << indent << "Window: " << this->Window << endl;
  os << indent << "AnimationManager: " << this->AnimationManager << endl;
}
