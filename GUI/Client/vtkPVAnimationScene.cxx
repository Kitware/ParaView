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
#include "vtkObjectFactory.h"
#include "vtkCommand.h"
#include "vtkAnimationScene.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWFrame.h"
#include "vtkKWPushButton.h"
#include "vtkKWCheckButton.h"
#include "vtkKWIcon.h"
#include "vtkKWScale.h"
#include "vtkKWThumbWheel.h"
#include "vtkKWMenuButton.h"
#include "vtkKWLabel.h"
#include "vtkSMAnimationSceneProxy.h"
#include "vtkSMAnimationCueProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkKWEntry.h"
#include "vtkPVAnimationCue.h"
#include "vtkPVAnimationManager.h"
#include "vtkPVRenderView.h"
#include "vtkPVWindow.h"
#include "vtkKWEvent.h"
#include "vtkProcessModule.h"
#include "vtkPVRenderModule.h"
#include "vtkImageWriter.h"
#include "vtkKWGenericMovieWriter.h"
#include "vtkWindowToImageFilter.h"
#include "vtkRenderWindow.h"
#include "vtkJPEGWriter.h"
#include "vtkTIFFWriter.h"
#include "vtkPNGWriter.h"
#include "vtkMPEG2Writer.h"
#include "vtkKWMessageDialog.h"
#include "vtkSMXMLPVAnimationWriterProxy.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVSource.h"
#include "vtkSMPartDisplay.h"
#include "vtkErrorCode.h"
#include "vtkPVVCRControl.h"
#include "vtkKWToolbarSet.h"

#ifdef _WIN32
  #include "vtkAVIWriter.h"
#endif

#if !defined(_WIN32) || defined(__CYGWIN__)
# include <unistd.h> /* unlink */
#else
# include <io.h> /* unlink */
#endif

vtkStandardNewMacro(vtkPVAnimationScene);
vtkCxxRevisionMacro(vtkPVAnimationScene, "1.10");
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
  this->FrameRateLabel = vtkKWLabel::New();
  this->FrameRateThumbWheel = vtkKWThumbWheel::New();
  this->DurationLabel = vtkKWLabel::New();
  this->DurationThumbWheel = vtkKWThumbWheel::New();
  this->PlayModeMenuButton = vtkKWMenuButton::New();
  this->PlayModeLabel = vtkKWLabel::New();

  this->RenderView = NULL;
  this->AnimationManager = NULL;
  this->Window = NULL;
  this->ErrorEventTag = 0;
  this->InPlay  = 0;

  this->WindowToImageFilter = NULL;
  this->MovieWriter = NULL;
  this->ImageWriter = NULL;
  this->FileRoot = NULL;
  this->FileExtension = NULL;

  this->GeometryWriter = NULL;

  this->InvokingError = 0;
}

//-----------------------------------------------------------------------------
vtkPVAnimationScene::~vtkPVAnimationScene()
{
  if (this->AnimationSceneProxyName)
    {
    vtkSMObject::GetProxyManager()->UnRegisterProxy("animation",
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
  this->FrameRateLabel->Delete();
  this->FrameRateThumbWheel->Delete();
  this->DurationLabel->Delete();
  this->DurationThumbWheel->Delete();
  this->PlayModeMenuButton->Delete();
  this->PlayModeLabel->Delete();
  this->SetRenderView(NULL);
  this->SetAnimationManager(NULL);
  if (this->ImageWriter)
    {
    this->ImageWriter->Delete();
    this->ImageWriter = NULL;
    }
  if (this->WindowToImageFilter)
    {
    this->WindowToImageFilter->Delete();
    this->WindowToImageFilter = NULL;
    }
  if (this->MovieWriter)
    {
    this->MovieWriter->Delete();
    this->MovieWriter = NULL;
    }
  this->SetFileRoot(0);
  this->SetFileExtension(0);

  if (this->GeometryWriter)
    {
    this->GeometryWriter->Delete();
    this->GeometryWriter = NULL;
    }
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
  this->Script("grid %s -columnspan 2 -sticky {}",
    this->VCRControl->GetWidgetName());

  this->VCRToolbar->SetParent(this->Window->GetLowerToolbars()->GetToolbarsFrame());
  this->VCRToolbar->Create(app);
  this->VCRToolbar->SetPlayCommand(this, "Play");
  this->VCRToolbar->SetStopCommand(this, "Stop");
  this->VCRToolbar->SetGoToBeginningCommand(this, "GoToBeginning");
  this->VCRToolbar->SetGoToEndCommand(this,"GoToEnd");
  this->VCRToolbar->SetGoToPreviousCommand(this, "GoToPrevious");
  this->VCRToolbar->SetGoToNextCommand(this,"GoToNext");
  this->VCRToolbar->SetLoopCheckCommand(this,"ToolbarLoopCheckButtonCallback");
  this->Window->AddLowerToolbar(this->VCRToolbar, VTK_PV_TOOLBARS_ANIMATION_LABEL, 0);
  this->VCRToolbar->UpdateEnableState();

  // Animation Control: Time scale
  this->TimeLabel->SetParent(this);
  this->TimeLabel->Create(app, 0);
  this->TimeLabel->SetLabel("Current Time:");

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
  this->DurationLabel->SetLabel("Duration:");
  
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
  this->DurationThumbWheel->SetBalloonHelpString("Adjust the duration for "
    "the animation (in seconds).");
  this->DurationThumbWheel->GetEntry()->BindCommand(this, 
    "DurationChangedCallback");
  this->SetDuration(60.0);
  this->Script("grid %s %s -sticky ew",
    this->DurationLabel->GetWidgetName(),
    this->DurationThumbWheel->GetWidgetName());

  // Animation Control: Frame rate
  this->FrameRateLabel->SetParent(this);
  this->FrameRateLabel->Create(app, 0);
  this->FrameRateLabel->SetLabel("Frame Rate:");
    
  this->FrameRateThumbWheel->SetParent(this);
  this->FrameRateThumbWheel->PopupModeOn();
  this->FrameRateThumbWheel->SetResolution(0.01);
  this->FrameRateThumbWheel->SetValue(this->AnimationSceneProxy->GetFrameRate());
  this->FrameRateThumbWheel->Create(app, NULL);
  this->FrameRateThumbWheel->DisplayEntryOn();
  this->FrameRateThumbWheel->DisplayLabelOff();
  this->FrameRateThumbWheel->DisplayEntryAndLabelOnTopOff();
  this->FrameRateThumbWheel->ExpandEntryOn();
  this->FrameRateThumbWheel->SetEntryCommand(this, "FrameRateChangedCallback");
  this->FrameRateThumbWheel->SetEndCommand(this, "FrameRateChangedCallback");
  this->FrameRateThumbWheel->GetEntry()->BindCommand(this,
    "FrameRateChangedCallback");
  this->Script("grid %s %s -sticky ew",
    this->FrameRateLabel->GetWidgetName(),
    this->FrameRateThumbWheel->GetWidgetName());
  this->SetFrameRate(1.0);

  // Animation Control: Play Mode
  this->PlayModeLabel->SetParent(this);
  this->PlayModeLabel->Create(app, 0);
  this->PlayModeLabel->SetLabel("Play Mode:" );
  this->PlayModeMenuButton->SetParent(this);
  this->PlayModeMenuButton->Create(app, 0);
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
  pxm->RegisterProxy("animation", this->AnimationSceneProxyName,
    this->AnimationSceneProxy);
 
  this->AnimationSceneProxy->UpdateVTKObjects();
  this->AnimationSceneProxy->AddObserver(vtkCommand::StartAnimationCueEvent,
    this->Observer);
  this->AnimationSceneProxy->AddObserver(vtkCommand::AnimationCueTickEvent,
    this->Observer);
  this->AnimationSceneProxy->AddObserver(vtkCommand::EndAnimationCueEvent,
    this->Observer);
  this->AnimationSceneProxy->SetStartTime(0);
  this->AnimationSceneProxy->SetEndTime(60);
  this->AnimationSceneProxy->SetTimeMode(VTK_ANIMATION_CUE_TIMEMODE_RELATIVE);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SaveImages(const char* fileRoot, const char* ext, 
  int width, int height, int aspectRatio)
{
  if (this->WindowToImageFilter || this->ImageWriter || this->MovieWriter)
    {
    vtkErrorMacro("Incosistent state. Save aborted.");
    return;
    }

  int *size = this->RenderView->GetRenderWindowSize();
  int magnification = 1;
  // determine magnification.
  if (size[0] < width || size[1] < height)
    {
    int xMag = width / size[0] + 1;
    int yMag = height / size[1] + 1;
    magnification = (xMag > yMag) ? xMag : yMag;
    width /= magnification;
    height /= magnification;
    }

  this->RenderView->SetRenderWindowSize(width, height);

  this->WindowToImageFilter = vtkWindowToImageFilter::New();
  this->WindowToImageFilter->SetInput(this->RenderView->GetRenderWindow());
  this->WindowToImageFilter->SetMagnification(magnification);
  
  if (strcmp(ext,"jpg") == 0)
    {
    this->ImageWriter = vtkJPEGWriter::New();
    }
  else if (strcmp(ext,"tif") == 0)
    {
    this->ImageWriter = vtkTIFFWriter::New();
    }
  else if (strcmp(ext,"png") == 0)
    {
    this->ImageWriter = vtkPNGWriter::New();
    }
  else if (strcmp(ext, "mp2") == 0)
    {
    this->MovieWriter = vtkMPEG2Writer::New();
    }
#ifdef _WIN32
  else if (strcmp(ext, "avi") == 0)
    {
    this->MovieWriter = vtkAVIWriter::New();
    }
#endif
  else
    {
    vtkErrorMacro("Unknown extension " << ext << ", try: jpg, tif or png.");
    return;
    }
  
  this->SetFileRoot(fileRoot);
  this->SetFileExtension(ext);
  this->FileCount = 0;
  this->SaveFailed = 0;
  if (this->ImageWriter)
    {
    this->ImageWriter->SetInput(this->WindowToImageFilter->GetOutput());
    }
  else if (this->MovieWriter)
    {
    ostrstream str;
    str << fileRoot << "." << ext << ends;
    this->MovieWriter->SetInput(this->WindowToImageFilter->GetOutput());
    this->MovieWriter->SetFileName(str.str());
    str.rdbuf()->freeze(0);
    this->MovieWriter->Start();
    }
 
  this->AddTraceEntry("$kw(%s) SaveImages \"%s\" \"%s\" %d %d %d", this->GetTclName(),
    fileRoot, ext, width, height, aspectRatio);

  // Play the animation.
  int oldMode = this->GetPlayMode();
  this->SetPlayModeToSequence();
  this->GoToBeginning();
  this->Play();
  this->SetPlayMode(oldMode);

  this->WindowToImageFilter->Delete();
  this->WindowToImageFilter = NULL;
  if (this->ImageWriter)
    {
    this->ImageWriter->Delete();
    this->ImageWriter = NULL;
    }
  else if (this->MovieWriter)
    {
    this->MovieWriter->End();
    this->MovieWriter->SetInput(0);
    this->MovieWriter->Delete();
    this->MovieWriter = NULL;
    }
  if (this->SaveFailed)
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetApplication(), this->Window, "Write Error",
      "There is insufficient disk space to save the images for this "
      "animation. The file(s) already written will be deleted.");
    if (this->ImageWriter)
      {
      char* fileName = new char[strlen(this->FileRoot) + strlen(this->FileExtension) + 25];
      for (int i=0; i < this->FileCount; i++)
        {
        sprintf(fileName, "%s%04d.%s", this->FileRoot, i, this->FileExtension);
        unlink(fileName);
        }
      delete [] fileName;
      }
    }
  // TODO:trace?  
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SaveGeometry(const char* filename)
{
  vtkSMXMLPVAnimationWriterProxy* animWriter = 
    vtkSMXMLPVAnimationWriterProxy::SafeDownCast(vtkSMObject::GetProxyManager()
      ->NewProxy("writers","XMLPVAnimationWriter"));
  if (!animWriter)
    {
    vtkErrorMacro("Failed to create XMLPVAnimationWriter proxy.");
    return;
    }

  this->AddTraceEntry("$kw(%s) SaveGeometry %s", this->GetTclName(), filename);

  this->GeometryWriter = animWriter;

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    animWriter->GetProperty("FileName"));
  svp->SetElement(0,filename);
  animWriter->UpdateVTKObjects();

//  vtkSMProxyProperty *pp = vtkSMProxyProperty::SafeDownCast(
//    animWriter->GetProperty("Input"));
  
  vtkPVSourceCollection* sources = this->Window->GetSourceList("Sources");
  sources->InitTraversal();
  vtkPVSource* source;
  while( (source = sources->GetNextPVSource()) )
    {
    if (source->GetVisibility())
      {
//      pp->RemoveAllProxies();
//      pp->AddProxy(source->GetPartDisplay());
//      animWriter->UpdateVTKObjects();
        //TODO: Since vtkSMPartDisplay is does not belong to any group,
        //we have a little difficulty is using the property interface.
        animWriter->AddInput(source->GetPartDisplay());
      }
    }

  vtkSMProperty* p = animWriter->GetProperty("Start");
  p->Modified();
  animWriter->UpdateVTKObjects();

  // Play the animation.
  int oldMode = this->GetPlayMode();
  this->SetPlayModeToSequence();
  this->GoToBeginning();
  this->Play();
  this->SetPlayMode(oldMode);
 
  p = animWriter->GetProperty("Finish");
  p->Modified();
  animWriter->UpdateVTKObjects();

  if (animWriter->GetErrorCode() == vtkErrorCode::OutOfDiskSpaceError)
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetApplication(), this->Window,
      "Write Error", "There is insufficient disk space to save the geometry "
      "for this animation. The file(s) already written will be deleted.");
    }
  animWriter->Delete();
  this->GeometryWriter = NULL;
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
  
  vtkAnimationCue::AnimationCueInfo *info = reinterpret_cast<
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
      double ntime = (etime==stime)?  0 : (info->CurrentTime - stime) / (etime - stime);
      this->AnimationManager->SetTimeMarker(ntime);
      this->TimeScale->SetValue(info->CurrentTime);
      if (this->AnimationManager->GetUseGeometryCache())
        {
        int index = static_cast<int>((info->CurrentTime - stime) * this->GetFrameRate());
        int maxindex = static_cast<int>((etime - stime) * this->GetFrameRate())+1; 
        vtkProcessModule::GetProcessModule()->GetRenderModule()->CacheUpdate(index, maxindex);
        }
      if (this->RenderView)
        {
        this->RenderView->ForceRender();
        }
      this->SaveImages();
      this->SaveGeometry(info->CurrentTime);
      }
    break;
    }
  this->Script("update");
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SaveImages()
{
  if (!this->WindowToImageFilter)
    {
    return;
    }
  int errcode = 0;
  // Saving an animation.
  this->WindowToImageFilter->Modified();
  this->WindowToImageFilter->ShouldRerenderOff();
  if (this->ImageWriter)
    {
    char* fileName = new char[strlen(this->FileRoot) + strlen(this->FileExtension) + 25];
    sprintf(fileName, "%s%04d.%s", this->FileRoot, this->FileCount, this->FileExtension);
    this->ImageWriter->SetFileName(fileName);
    this->ImageWriter->Write();
    errcode = this->ImageWriter->GetErrorCode(); 
    this->FileCount = (!errcode)? this->FileCount + 1 : this->FileCount; 
    delete [] fileName;
    }
  else if (this->MovieWriter)
    {
    this->MovieWriter->Write();
    errcode = this->MovieWriter->GetErrorCode() + this->MovieWriter->GetError();
    }
  if (errcode)
    {
    this->Stop();
    this->SaveFailed = errcode;
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SaveGeometry(double time)
{
  if (!this->GeometryWriter)
    {
    return;
    }
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GeometryWriter->GetProperty("WriteTime"));
  dvp->SetElement(0, time);
  this->GeometryWriter->UpdateVTKObjects();
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
void vtkPVAnimationScene::SetDuration(double duration)
{
  double ntime = this->GetNormalizedCurrentTime();
  
  this->AnimationSceneProxy->SetEndTime(duration);
  this->DurationThumbWheel->SetValue(duration);
  this->TimeScale->SetRange(0, duration);
  this->TimeScale->SetValue(duration*ntime);
  this->InvalidateAllGeometries();
  this->AddTraceEntry("$kw(%s) SetDuration %f", this->GetTclName(), duration);
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
  this->AddTraceEntry("$kw(%s) Play", this->GetTclName());
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::Stop()
{
  this->AnimationSceneProxy->Stop();
  this->AddTraceEntry("$kw(%s) Stop", this->GetTclName());
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
    break;
  case VTK_ANIMATION_SCENE_PLAYMODE_REALTIME:
    this->PlayModeMenuButton->SetButtonText(VTK_PV_PLAYMODE_REALTIME_TITLE);
    break;
  default:
    vtkErrorMacro("Invalid play mode " << mode);
    return;
    }
  this->AnimationSceneProxy->SetPlayMode(mode);
  this->AddTraceEntry("$kw(%s) SetPlayMode %d", this->GetTclName(), mode);
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
void vtkPVAnimationScene::FrameRateChangedCallback()
{
  this->SetFrameRate(this->FrameRateThumbWheel->GetEntry()->GetValueAsFloat());
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::SetFrameRate(double fps)
{
  if (this->GetFrameRate() == fps)
    {
    return;
    }
  this->AnimationSceneProxy->SetFrameRate(fps);
  this->FrameRateThumbWheel->SetValue(fps);
  this->InvalidateAllGeometries();
  this->AddTraceEntry("$kw(%s) SetFrameRate %f", this->GetTclName(), fps);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::InvalidateAllGeometries()
{
  vtkProcessModule::GetProcessModule()->GetRenderModule()->InvalidateAllGeometries();
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
  this->AnimationSceneProxy->SetLoop(loop);
  this->AddTraceEntry("$kw(%s) SetLoop %d", this->GetTclName(), loop);
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
  this->AnimationSceneProxy->SetCurrentTime(time);
  this->TimeScale->SetValue(time);
  this->AddTraceEntry("$kw(%s) SetCurrentTime %f", this->GetTclName(), time);
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

  this->AnimationSceneProxy->AddCue(pvCue->GetCueProxy());
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::RemoveAnimationCue(vtkPVAnimationCue* pvCue)
{
  this->AnimationSceneProxy->RemoveCue(pvCue->GetCueProxy());
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
  int enabled = this->Enabled;
  // These widgets are always off except when playing.
  this->Enabled = this->IsInPlay();
  

  // These widgets are on when playing or when gui is enabled.
  this->Enabled = this->IsInPlay() || enabled;
  this->PropagateEnableState(this->VCRControl);
 
  // These widgets are disabled when playing.
  this->Enabled = enabled && !this->IsInPlay();
  this->PropagateEnableState(this->FrameRateLabel);
  this->PropagateEnableState(this->FrameRateThumbWheel);
  this->PropagateEnableState(this->DurationLabel);
  this->PropagateEnableState(this->DurationThumbWheel);
  this->PropagateEnableState(this->PlayModeLabel);
  this->PropagateEnableState(this->PlayModeMenuButton);
  this->PropagateEnableState(this->TimeLabel);
  this->PropagateEnableState(this->TimeScale);
  this->Enabled = enabled;
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
void vtkPVAnimationScene::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RenderView: " << this->RenderView << endl;
  os << indent << "Window: " << this->Window << endl;
  os << indent << "AnimationManager: " << this->AnimationManager << endl;
}
