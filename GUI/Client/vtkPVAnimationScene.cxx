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
#include "vtkErrorCode.h"
#include "vtkKWCheckButton.h"
#include "vtkKWEntry.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWIcon.h"
#include "vtkKWLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWMenuButton.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWPushButton.h"
#include "vtkKWScaleWithEntry.h"
#include "vtkKWThumbWheel.h"
#include "vtkKWToolbarSet.h"
#include "vtkKWToolbarSet.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationCue.h"
#include "vtkPVAnimationManager.h"
#include "vtkPVApplication.h"
#include "vtkPVCornerAnnotationEditor.h"
#include "vtkPVRenderView.h"
#include "vtkPVSource.h"
#include "vtkPVSource.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVTraceHelper.h"
#include "vtkPVVCRControl.h"
#include "vtkPVWindow.h"
#include "vtkProcessModule.h"
#include "vtkRenderWindow.h"
#include "vtkSMAnimationCueProxy.h"
#include "vtkSMAnimationSceneProxy.h"
#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"

#include "vtkGenericMovieWriter.h"

#ifdef _WIN32
  #include "vtkAVIWriter.h"

#endif

#if !defined(_WIN32) || defined(__CYGWIN__)
# include <unistd.h> /* unlink */
#else
# include <io.h> /* unlink */
#endif

vtkStandardNewMacro(vtkPVAnimationScene);
vtkCxxRevisionMacro(vtkPVAnimationScene, "1.68");
#define VTK_PV_PLAYMODE_SEQUENCE_TITLE "Sequence"
#define VTK_PV_PLAYMODE_REALTIME_TITLE "Real Time"
#define VTK_PV_TOOLBARS_ANIMATION_LABEL "Animation"

#define VTK_PV_DURATION_SEQUENCE_LABEL "No. of Frames:"
#define VTK_PV_DURATION_SEQUENCE_TIP   "Adjust the number of frames in the animation."
#define VTK_PV_DURATION_REALTIME_LABEL "Duration:"
#define VTK_PV_DURATION_REALTIME_TIP  "Adjust the duration for the animation (in seconds)."
#define VTK_PV_TIME_SEQUENCE_LABEL "Current Frame:"
#define VTK_PV_TIME_REALTIME_LABEL "Current Time:"
  

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
  this->VCRToolbar->SetName(VTK_PV_TOOLBARS_ANIMATION_LABEL);

  this->TimeLabel = vtkKWLabel::New();
  this->TimeScale = vtkKWScaleWithEntry::New();
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
  this->PropertiesChangedCallbackCommand = 0;

  this->InterpretDurationAsFrameMax = 0;
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
  this->SetPropertiesChangedCallbackCommand(0);

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
    // Make sure the AnimationScene is deleted here. It holds a reference
    // to the render module (therefore, the render window) which should
    // be deleted before the render widget is destroyed.
    this->AnimationSceneProxy->Delete();
    this->AnimationSceneProxy = 0;
    }
  if (this->AnimationSceneProxyName)
    {
    vtkSMObject::GetProxyManager()->UnRegisterProxy("animation_scene",
      this->AnimationSceneProxyName);
    this->SetAnimationSceneProxyName(0);
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
  if ( this->Window  && this->ErrorEventTag)
    {
    this->Window->RemoveObserver(this->ErrorEventTag);
    this->ErrorEventTag = 0;
    }
  this->Window = window;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::CreateWidget()
{
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

  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  this->CreateProxy();

  vtkKWMenu *menu;
  int index;

  //vtkKWIcon* icon = vtkKWIcon::New();
  
  this->Script("grid propagate %s 1",
    this->GetWidgetName());
 
  this->VCRControl->SetParent(this);
//  this->VCRControl->SetMode(vtkPVVCRControl::PLAYBACK);
  this->VCRControl->Create();
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

  this->VCRToolbar->SetParent(this->Window->GetSecondaryToolbarSet()->GetToolbarsFrame());
  this->VCRToolbar->Create();
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
  this->Window->GetSecondaryToolbarSet()->AddToolbar(this->VCRToolbar, 0);
  this->VCRToolbar->UpdateEnableState();

  // Animation Control: Time scale
  this->TimeLabel->SetParent(this);
  this->TimeLabel->Create();
  //this->TimeLabel->SetText("Current Time:");

  this->TimeScale->SetParent(this);
  this->TimeScale->Create();
  this->TimeScale->SetResolution(0.01);
  this->TimeScale->SetEndCommand(this, "TimeScaleCallback");
  this->TimeScale->SetEntryCommand(this, "TimeScaleCallback");
  this->TimeScale->SetBalloonHelpString("Adjust the current time "
    "(in seconds).");
  this->Script("grid %s %s -sticky ew",
    this->TimeLabel->GetWidgetName(),
    this->TimeScale->GetWidgetName());


  this->DurationLabel->SetParent(this);
  this->DurationLabel->Create();
  
  this->DurationThumbWheel->SetParent(this);
  this->DurationThumbWheel->PopupModeOn();
  this->DurationThumbWheel->ClampMinimumValueOn();
  this->DurationThumbWheel->SetMinimumValue(1.0);
  this->DurationThumbWheel->Create();
  this->DurationThumbWheel->DisplayEntryOn();
  this->DurationThumbWheel->DisplayEntryAndLabelOnTopOff();
  this->DurationThumbWheel->ExpandEntryOn();
  this->DurationThumbWheel->SetEntryCommand(this, "DurationChangedCallback");
  this->DurationThumbWheel->SetEndCommand(this, "DurationChangedCallback");
  this->DurationThumbWheel->GetEntry()->AddBinding(
    "<KeyRelease>", this, "DurationChangedKeyReleaseCallback");
  this->SetDuration(10.0);
  this->Script("grid %s %s -sticky ew",
    this->DurationLabel->GetWidgetName(),
    this->DurationThumbWheel->GetWidgetName());

  // Animation Control: Play Mode
  this->PlayModeLabel->SetParent(this);
  this->PlayModeLabel->Create();
  this->PlayModeLabel->SetText("Play Mode:" );
  this->PlayModeMenuButton->SetParent(this);
  this->PlayModeMenuButton->Create();
  this->PlayModeMenuButton->SetBalloonHelpString("Change the mode in which the "
    "animation is played.");

  menu = this->PlayModeMenuButton->GetMenu();

  index = menu->AddCommand(
    VTK_PV_PLAYMODE_SEQUENCE_TITLE, this, "SetPlayMode 0");
  menu->SetItemHelpString(
    index, "Plays the animation as a sequence of images.");

  index = menu->AddCommand(
    VTK_PV_PLAYMODE_REALTIME_TITLE, this, "SetPlayMode 1");
  menu->SetItemHelpString(
    index, "Plays the animation in real time mode.");

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
                               vtkAnimationCue::TIMEMODE_RELATIVE);
  DoubleVectPropertySetElement(this->AnimationSceneProxy, "FrameRate", 1.0);

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->AnimationSceneProxy->GetProperty("RenderModule"));
  pp->AddProxy(this->RenderView->GetRenderModuleProxy());

  this->AnimationSceneProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SaveImages(const char* fileRoot, const char* ext, 
  int width, int height, double framerate, int quality)
{
  this->GetTraceHelper()->AddEntry("$kw(%s) SaveImages \"%s\" \"%s\" %d %d %f %d",
    this->GetTclName(), fileRoot, ext, width, height, framerate, quality);
 
  this->OnBeginPlay();
  int savefailed = this->AnimationSceneProxy->SaveImages(fileRoot, ext, 
    width, height, framerate, quality);
  this->OnEndPlay();

  if (savefailed)
    {
    char *errstring = new char[256];

    //may want separate error string functions for moviewriter and imagewriter 
    //eventually but right now imagewriters don't use usererror codes
    //so this will work alright

    const char *reason = vtkGenericMovieWriter::GetStringFromErrorCode(savefailed);
    
    char *message = new char[80];
    switch (savefailed)
      {
      case vtkErrorCode::OutOfDiskSpaceError:
      {
      strcpy(message, "There is insufficient disk space to save the images for this animation. ");
      break;     
      }
     default:
      {
      message[0] = 0;
      }
      }

    sprintf(errstring, "%.80s. %.80sAny file(s) already written have been deleted.", reason, message);

    vtkKWMessageDialog::PopupMessage(
      this->GetApplication(), this->Window, "Write Error",
      errstring);
    
    delete [] errstring;
    delete [] message;
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SaveGeometry(const char* filename)
{
  // Start at the beginning.
  this->GetTraceHelper()->AddEntry("$kw(%s) SaveGeometry %s", this->GetTclName(), filename);
  
  this->OnBeginPlay();
  int error = this->AnimationSceneProxy->SaveGeometry(filename);
  this->OnEndPlay();

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
      //PVProbe catches this for the temporal plot
      this->InvokeEvent(vtkCommand::StartAnimationCueEvent, calldata);
      break;
    case vtkCommand::EndAnimationCueEvent:
    case vtkCommand::AnimationCueTickEvent:
      {
      if (!this->AnimationSceneProxy)
        {
        return;
        }
      double etime = this->AnimationSceneProxy->GetEndTime();
      double stime = this->AnimationSceneProxy->GetStartTime();
      double ntime = 
        (etime==stime)?  0 : (cueInfo->AnimationTime - stime) / (etime - stime);
      this->AnimationManager->SetTimeMarker(ntime);
      this->TimeScale->SetValue(cueInfo->AnimationTime);
      if (this->RenderView)
        {
        this->RenderView->GetCornerAnnotation()->UpdateCornerText();
        }

      //PVProbe catches this for the temporal plot
      if (event == vtkCommand::AnimationCueTickEvent) 
        this->InvokeEvent(vtkCommand::AnimationCueTickEvent, calldata); 

      break;
      }
    }
  this->Script("update");
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::DurationChangedCallback(double value)
{
  this->SetDurationWithTrace(value);
}

//-----------------------------------------------------------------------------
double vtkPVAnimationScene::GetDuration()
{
  if (!this->AnimationSceneProxy)
    {
    return 0;
    }
  double val = this->AnimationSceneProxy->GetEndTime();
  return (this->InterpretDurationAsFrameMax)? (val + 1) : val;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SaveAnimationCallback()
{
  this->Window->SaveAnimation();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::DurationChangedKeyReleaseCallback()
{
  double duration = this->DurationThumbWheel->GetEntry()->GetValueAsDouble();
  if (duration >= 1.0)
    {
    this->SetDurationWithTrace(duration);
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SetDurationWithTrace(double duration)
{
  this->SetDuration(duration);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetDurationWithTrace %f", 
    this->GetTclName(), duration);
  
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SetDuration(double duration)
{
  if (this->GetDuration() == duration)
    {
    return;
    }
  if (this->InterpretDurationAsFrameMax)
    {
    // in sequence mode, duration has to be whole numbers.
    duration = static_cast<int>(duration);
    }
  if (duration < 1.0)
    {
    duration = this->GetDuration();
    }
  double ntime = this->GetNormalizedAnimationTime();

  double end_time = (this->InterpretDurationAsFrameMax)? (duration -1) : duration;
  DoubleVectPropertySetElement(this->AnimationSceneProxy,"EndTime", end_time);
  this->AnimationSceneProxy->UpdateVTKObjects();
  this->DurationThumbWheel->SetValue(duration);
  this->TimeScale->SetRange(0, end_time);
  double current_time = duration*ntime;
  if (this->InterpretDurationAsFrameMax)
    {
    current_time = static_cast<int>(current_time);
    }
  this->TimeScale->SetValue(current_time);
  this->InvalidateAllGeometries();
  this->InvokePropertiesChangedCallback();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::CaptureErrorEvents()
{
  if (!this->ErrorEventTag && this->Window)
    {
    this->ErrorEventTag = this->Window->AddObserver(
      vtkKWEvent::ErrorMessageEvent, this->Observer);
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::ReleaseErrorEvents()
{
  if (this->ErrorEventTag && this->Window)
    {
    this->Window->RemoveObserver(this->ErrorEventTag);
    this->ErrorEventTag = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::OnBeginPlay()
{
  if (this->InPlay)
    {
    return;
    }
  this->InPlay = 1;
  if (this->Window)
    {
    this->Window->UpdateEnableState();
    }
  this->VCRControl->SetInPlay(1);
  this->VCRControl->UpdateEnableState();
  this->VCRToolbar->SetInPlay(1);
  this->VCRToolbar->UpdateEnableState();
  this->CaptureErrorEvents(); 
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::OnEndPlay()
{
  if (!this->InPlay)
    {
    return;
    }
  this->ReleaseErrorEvents();
  this->InPlay = 0;
  if (this->Window)
    {
    this->Window->UpdateEnableState();
    }
  this->VCRControl->SetInPlay(0);
  this->VCRControl->UpdateEnableState();
  this->VCRToolbar->SetInPlay(0);
  this->VCRToolbar->UpdateEnableState();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::Play()
{
  this->OnBeginPlay();
  this->AnimationSceneProxy->Play();
  this->OnEndPlay();
  this->GetTraceHelper()->AddEntry("$kw(%s) Play", this->GetTclName());
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::Stop()
{
  if (this->AnimationSceneProxy)
    {
    this->AnimationSceneProxy->Stop();
    }
  if (this->Window && this->Window->GetCurrentPVSource() )
    {
    this->Window->GetCurrentPVSource()->ResetCallback();
    }
  this->GetTraceHelper()->AddEntry("$kw(%s) Stop", this->GetTclName());
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::GoToBeginning()
{
  this->SetAnimationTimeWithTrace(0);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::GoToEnd()
{
  if (!this->AnimationSceneProxy)
    {
    return;
    }
  this->SetAnimationTimeWithTrace(this->AnimationSceneProxy->GetEndTime());
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::GoToNext()
{
  if (!this->AnimationSceneProxy)
    {
    return;
    }
  double time = this->TimeScale->GetValue();
  double duration = this->AnimationSceneProxy->GetEndTime();
  double newtime = time + 1.0/this->AnimationSceneProxy->GetFrameRate();
  newtime = (newtime > duration) ? duration : newtime;
  if (newtime != time)
    {
    this->SetAnimationTimeWithTrace(newtime);
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::GoToPrevious()
{
  if (!this->AnimationSceneProxy)
    {
    return;
    }
  double time = this->TimeScale->GetValue();
  double newtime = time - 1.0/this->AnimationSceneProxy->GetFrameRate();
  newtime = (newtime < 0) ? 0 : newtime;
  if (newtime != time)
    {
    this->SetAnimationTimeWithTrace(newtime); 
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SetPlayModeToSequence()
{
  this->SetPlayMode(vtkAnimationScene::PLAYMODE_SEQUENCE);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SetPlayModeToRealTime()
{
  this->SetPlayMode(vtkAnimationScene::PLAYMODE_REALTIME);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SetPlayMode(int mode)
{
  switch (mode)
    {
  case vtkAnimationScene::PLAYMODE_SEQUENCE:
    this->PlayModeMenuButton->SetValue(VTK_PV_PLAYMODE_SEQUENCE_TITLE);
    // Change the time scale increment to 1.
    this->TimeScale->SetResolution(1);
    this->SetInterpretDurationAsFrameMax(1);
    this->AnimationManager->EnableCacheCheck();
    break;
  case vtkAnimationScene::PLAYMODE_REALTIME:
    this->PlayModeMenuButton->SetValue(VTK_PV_PLAYMODE_REALTIME_TITLE);
    this->SetInterpretDurationAsFrameMax(0);
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
  this->GetTraceHelper()->AddEntry("$kw(%s) SetPlayMode %d", 
    this->GetTclName(), mode);
  this->InvokePropertiesChangedCallback();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SetInterpretDurationAsFrameMax(int val)
{
  if (this->InterpretDurationAsFrameMax == val)
    {
    return;
    }
  // Interpretation of duration value is going to change.
  // Obtain the current val.
  double old_duration = this->DurationThumbWheel->GetValue();
  this->InterpretDurationAsFrameMax = val;
  this->SetDuration(old_duration);
  const char* label = (this->InterpretDurationAsFrameMax)?
    VTK_PV_DURATION_SEQUENCE_LABEL : VTK_PV_DURATION_REALTIME_LABEL;
  const char* tip = (this->InterpretDurationAsFrameMax)?
    VTK_PV_DURATION_SEQUENCE_TIP : VTK_PV_DURATION_REALTIME_TIP;
  
  this->DurationLabel->SetText(label);
  this->DurationThumbWheel->SetBalloonHelpString(tip);
  this->TimeLabel->SetText( (this->InterpretDurationAsFrameMax)?
    VTK_PV_TIME_SEQUENCE_LABEL : VTK_PV_TIME_REALTIME_LABEL);
}

//-----------------------------------------------------------------------------
int vtkPVAnimationScene::GetPlayMode()
{
  return this->AnimationSceneProxy->GetPlayMode();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::LoopCheckButtonCallback()
{
  this->SetLoopWithTrace(this->VCRControl->GetLoopButtonState());
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::ToolbarLoopCheckButtonCallback()
{
  this->SetLoopWithTrace(this->VCRToolbar->GetLoopButtonState());
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
  this->InvokePropertiesChangedCallback();

  this->GetTraceHelper()->AddEntry("$kw(%s) SetFrameRate %f", 
    this->GetTclName(), fps);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SetPropertiesChangedCallback(vtkKWWidget* target,
  const char* methodAndArgs)
{
  if (!target)
    {
    this->SetPropertiesChangedCallbackCommand(0);
    }
  else
    {
    ostrstream str;
    str << target->GetTclName() << " " ;
    if (methodAndArgs)
      {
      str << methodAndArgs;
      }
    str << ends;
    this->SetPropertiesChangedCallbackCommand(str.str());
    str.rdbuf()->freeze(0);
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::InvokePropertiesChangedCallback()
{
  if (this->PropertiesChangedCallbackCommand)
    {
    this->Script(this->PropertiesChangedCallbackCommand);
    }
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
  if (this->AnimationSceneProxy)
    {
    this->AnimationSceneProxy->CleanCache();
    }
}

//-----------------------------------------------------------------------------
double vtkPVAnimationScene::GetFrameRate()
{
  return this->AnimationSceneProxy->GetFrameRate();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SetLoopWithTrace(int loop)
{
  this->SetLoop(loop);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetLoopWithTrace %d", 
    this->GetTclName(), loop);
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
}

//-----------------------------------------------------------------------------
int vtkPVAnimationScene::GetLoop()
{
  return this->AnimationSceneProxy->GetLoop();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SetAnimationTimeWithTrace(double time)
{
  this->SetAnimationTime(time);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetAnimationTimeWithTrace %f", 
    this->GetTclName(), time);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SetAnimationTime(double time)
{
  if (!this->IsCreated())
    {
    vtkErrorMacro("Scene has not been created yet.");
    return;
    }

  this->InvokeEvent(vtkKWEvent::TimeChangedEvent);

  //Firstly, if the time resolution is 1, we round the time the time value.
  if (this->TimeScale->GetResolution() == 1)
    {
    time = (int)(time + 0.5);
    }
  DoubleVectPropertySetElement(this->AnimationSceneProxy, "CurrentTime", time);
  this->AnimationSceneProxy->GetProperty("CurrentTime")->Modified();
  this->AnimationSceneProxy->UpdateVTKObjects();
  this->TimeScale->SetValue(time);
  if (this->Window && this->Window->GetCurrentPVSource())
    {
    this->Window->GetCurrentPVSource()->ResetCallback();
    vtkSMSourceProxy *sourceProxy =
      this->Window->GetCurrentPVSource()->GetProxy();
    if (sourceProxy)
      {
      // Data information needs to be updated.
      sourceProxy->InvalidateDataInformation(1);
      sourceProxy->GetDataInformation();
      }
    }
}

//-----------------------------------------------------------------------------
double vtkPVAnimationScene::GetAnimationTime()
{
  return this->TimeScale->GetValue();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SetNormalizedAnimationTime(double ntime)
{
  if (!this->IsCreated())
    {
    vtkErrorMacro("Scene has not been created yet.");
    return;
    }
  this->SetAnimationTime(ntime * this->GetDuration());
}

//-----------------------------------------------------------------------------
double vtkPVAnimationScene::GetNormalizedAnimationTime()
{
  return (this->GetAnimationTime() / this->GetDuration());
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::TimeScaleCallback(double value)
{
  this->SetAnimationTimeWithTrace(value);
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
  if (!this->AnimationSceneProxy)
    {
    return;
    }
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
  *file << "$kw(" << this->GetTclName() << ") SetAnimationTime " << 
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
  this->Window->GetSecondaryToolbarSet()->SetToolbarVisibility(
    this->VCRToolbar, visible);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RenderView: " << this->RenderView << endl;
  os << indent << "Window: " << this->Window << endl;
  os << indent << "AnimationManager: " << this->AnimationManager << endl;
}

//-----------------------------------------------------------------------------
#ifndef VTK_LEGACY_REMOVE
# ifdef VTK_WORKAROUND_WINDOWS_MANGLE
#  undef GetCurrentTime
// Define possible mangled names.
int vtkPVAnimationScene::GetTickCount()
{
  VTK_LEGACY_REPLACED_BODY(vtkPVAnimationScene::GetCurrentTime, "ParaView 2.4",
                           vtkPVAnimationScene::GetAnimationTime);
  return this->GetAnimationTime();
}
# endif
double vtkPVAnimationScene::GetCurrentTime()
{
  VTK_LEGACY_REPLACED_BODY(vtkPVAnimationScene::GetCurrentTime, "ParaView 2.4",
                           vtkPVAnimationScene::GetAnimationTime);
  return this->GetAnimationTime();
}
void vtkPVAnimationScene::SetCurrentTime(double time)
{
  VTK_LEGACY_REPLACED_BODY(vtkPVAnimationScene::SetCurrentTime, "ParaView 2.4",
                           vtkPVAnimationScene::SetAnimationTime);
  this->SetAnimationTime(time);
}
void vtkPVAnimationScene::SetCurrentTimeWithTrace(double time)
{
  VTK_LEGACY_REPLACED_BODY(vtkPVAnimationScene::SetCurrentTimeWithTrace, "ParaView 2.4",
                           vtkPVAnimationScene::SetAnimationTimeWithTrace);
  this->SetAnimationTimeWithTrace(time);
}
#endif
