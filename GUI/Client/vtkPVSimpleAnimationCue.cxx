/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSimpleAnimationCue.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVSimpleAnimationCue.h"

#include "vtkObjectFactory.h"
#include "vtkPVTraceHelper.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMAnimationCueProxy.h"
#include "vtkSMKeyFrameAnimationCueManipulatorProxy.h"
#include "vtkCollectionIterator.h"
#include "vtkCollection.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkAnimationCue.h"
#include "vtkCommand.h"
#include "vtkPVKeyFrame.h"
#include "vtkSMKeyFrameProxy.h"
#include "vtkPVAnimationManager.h"
#include "vtkPVApplication.h"
#include "vtkPVWindow.h"
#include "vtkSMPropertyStatusManager.h"

vtkStandardNewMacro(vtkPVSimpleAnimationCue);
vtkCxxRevisionMacro(vtkPVSimpleAnimationCue,"1.1");

//***************************************************************************
class vtkPVSimpleAnimationCueObserver : public vtkCommand
{
public:
  static vtkPVSimpleAnimationCueObserver* New()
    { return new vtkPVSimpleAnimationCueObserver; }
  void SetTarget(vtkPVSimpleAnimationCue* cue)
    {
    this->Target = cue;
    }
  virtual void Execute(vtkObject* wdg, unsigned long event, void* data)
    {
    if (this->Target)
      {
      this->Target->ExecuteEvent(wdg, event, data);
      }
    }
protected:
  vtkPVSimpleAnimationCueObserver()
    {
    this->Target = 0;
    }
  vtkPVSimpleAnimationCue* Target;
};

//***************************************************************************
//Helper methods to down cast the property and set value.
inline static int DoubleVectPropertySetElement(vtkSMProxy *proxy, 
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
inline static int StringVectPropertySetElement(vtkSMProxy *proxy, 
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
inline static int IntVectPropertySetElement(vtkSMProxy *proxy, 
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
vtkPVSimpleAnimationCue::vtkPVSimpleAnimationCue()
{
  this->ParentCue = 0;
  this->Virtual = 0;
  this->NumberOfPoints = 0;
  this->PointParameters[0] = this->PointParameters[1] = 0.0;

  this->CueProxy = 0;
  this->CueProxyName = 0;
  this->KeyFrameManipulatorProxy = 0; 
  this->KeyFrameManipulatorProxyName = 0;

  this->PVKeyFrames = vtkCollection::New();
  this->PVKeyFramesIterator = this->PVKeyFrames->NewIterator();

  this->LabelText = 0;

  this->ProxiesRegistered = 0;
  this->InRecording = 0;
  this->PreviousStepKeyFrameAdded = 0;
  this->KeyFramesCreatedCount = 0;
  this->PropertyStatusManager = 0;

  this->SelectedKeyFrameIndex = -1;

  this->Observer = vtkPVSimpleAnimationCueObserver::New();
  this->Observer->SetTarget(this);
}

//-----------------------------------------------------------------------------
vtkPVSimpleAnimationCue::~vtkPVSimpleAnimationCue()
{
  this->Observer->SetTarget(0);
  this->Observer->Delete();

  this->PVKeyFrames->Delete();
  this->PVKeyFramesIterator->Delete();

  this->SetCueProxyName(0);
  if (this->CueProxy)
    {
    this->CueProxy->Delete();
    this->CueProxy = 0;
    }

  this->SetKeyFrameManipulatorProxyName(0);
  if (this->KeyFrameManipulatorProxy)
    {
    this->KeyFrameManipulatorProxy->Delete();
    this->KeyFrameManipulatorProxy = 0;
    }
  if (this->PropertyStatusManager)
    {
    this->PropertyStatusManager->Delete();
    this->PropertyStatusManager = 0;
    }
  this->SetParentAnimationCue(0);
  this->SetLabelText(0);
}

//----------------------------------------------------------------------------
void vtkPVSimpleAnimationCue::Observe(vtkObject* toObserve, unsigned long event)
{
  toObserve->AddObserver(event, this->Observer);
}

//-----------------------------------------------------------------------------
void vtkPVSimpleAnimationCue::Create(vtkKWApplication* app, const char* type, 
  const char* args)
{
  if (!this->Superclass::Create(app, type, args))
    {
    return;
    }
  this->CreateProxy();
}

//-----------------------------------------------------------------------------
void vtkPVSimpleAnimationCue::CreateProxy()
{
  if (this->Virtual)
    {
    return;
    }
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  static int proxyNum = 0;
  this->CueProxy = vtkSMAnimationCueProxy::SafeDownCast(
    pxm->NewProxy("animation","AnimationCue"));
  if (!this->CueProxy)
    {
    vtkErrorMacro("Failed to create proxy " << "AnimationCue");
    return;
    }
  ostrstream str;
  str << "AnimationCue" << proxyNum << ends;
  this->SetCueProxyName(str.str());

  this->KeyFrameManipulatorProxy = vtkSMKeyFrameAnimationCueManipulatorProxy::
    SafeDownCast(pxm->NewProxy("animation_manipulators",
        "KeyFrameAnimationCueManipulator"));
  if (!this->KeyFrameManipulatorProxy)
    {
    vtkErrorMacro("Failed to create proxy KeyFrameAnimationCueManipulator");
    return;
    }
  ostrstream str1;
  str1 << "KeyFrameAnimationCueManipulator" << proxyNum << ends;
  this->SetKeyFrameManipulatorProxyName(str1.str());

  proxyNum++;
  str.rdbuf()->freeze(0);
  str1.rdbuf()->freeze(0);


  this->KeyFrameManipulatorProxy->UpdateVTKObjects();

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->CueProxy->GetProperty("Manipulator"));
  if (pp)
    {
    pp->RemoveAllProxies();
    pp->AddProxy(this->KeyFrameManipulatorProxy);
    }
  IntVectPropertySetElement(this->CueProxy, "TimeMode", 
    VTK_ANIMATION_CUE_TIMEMODE_NORMALIZED);
  DoubleVectPropertySetElement(this->CueProxy, "StartTime", 0.0);
  DoubleVectPropertySetElement(this->CueProxy, "EndTime", 1.0);
  this->CueProxy->UpdateVTKObjects(); //calls CreateVTKObjects(1) internally.

  this->Observe(this->KeyFrameManipulatorProxy, vtkCommand::ModifiedEvent);
}

//-----------------------------------------------------------------------------
void vtkPVSimpleAnimationCue::ExecuteEvent(vtkObject* wdg, unsigned long event,
  void* data)
{
  if (vtkSMKeyFrameAnimationCueManipulatorProxy::SafeDownCast(wdg))
    {
    switch(event)
      {
    case vtkCommand::ModifiedEvent:
      // Triggerred when the keyframes have been changed in someway.
      if (this->GetNumberOfKeyFrames() >= 2 )
        {
        this->RegisterProxies();
        }
      if (this->GetNumberOfKeyFrames() < 2)
        {
        this->UnregisterProxies();
        }
      this->InvokeEvent(vtkPVSimpleAnimationCue::KeysModifiedEvent);
      return;
      }
    }
}

//-----------------------------------------------------------------------------
char* vtkPVSimpleAnimationCue::GetTextRepresentation()
{
  ostrstream str;
  if (this->ParentCue)
    {
    char * ptext = this->ParentCue->GetTextRepresentation();
    if (ptext)
      {
      str <<  ptext << " : ";
      delete [] ptext;
      }
    // text is returned only if it has parent to avoid returning the
    // label "Animation Tracks"
    str << this->GetLabelText() << ends;
    return str.str();
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkPVSimpleAnimationCue::RegisterProxies()
{
  if (this->Virtual || !this->CueProxyName || !this->KeyFrameManipulatorProxyName)
    {
    return;
    }

  if (this->ProxiesRegistered)
    {
    return;
    }

  vtkSMObject::GetProxyManager()->RegisterProxy("animation",
    this->CueProxyName, this->CueProxy);
  vtkSMObject::GetProxyManager()->RegisterProxy("animation_manipulators",
    this->KeyFrameManipulatorProxyName, this->KeyFrameManipulatorProxy);
  this->ProxiesRegistered = 1;
}

//-----------------------------------------------------------------------------
void vtkPVSimpleAnimationCue::UnregisterProxies()
{
  if (this->Virtual || !this->CueProxyName || !this->KeyFrameManipulatorProxyName)
    {
    return;
    }
  if (!this->ProxiesRegistered)
    {
    return;
    }
  vtkSMObject::GetProxyManager()->UnRegisterProxy("animation",
    this->CueProxyName);
  vtkSMObject::GetProxyManager()->UnRegisterProxy("animation_manipulators",
    this->KeyFrameManipulatorProxyName);
  this->ProxiesRegistered = 0;
}

//-----------------------------------------------------------------------------
unsigned long vtkPVSimpleAnimationCue::GetKeyFramesMTime()
{
  return (this->Virtual)? this->GetMTime() :
      this->KeyFrameManipulatorProxy->GetMTime();
}

//-----------------------------------------------------------------------------
int vtkPVSimpleAnimationCue::GetNumberOfKeyFrames()
{
  return (this->Virtual)? this->NumberOfPoints :
    this->KeyFrameManipulatorProxy->GetNumberOfKeyFrames();
}

//-----------------------------------------------------------------------------
double vtkPVSimpleAnimationCue::GetKeyFrameTime(int id)
{
  if (id < 0 || id >= this->GetNumberOfKeyFrames())
    {
    vtkErrorMacro("Id beyond range");
    return 0.0;
    }
  if (this->Virtual)
    {
    return this->PointParameters[id];
    }
  else
    {
    vtkSMKeyFrameProxy* keyframe = this->KeyFrameManipulatorProxy->
      GetKeyFrameAtIndex(id);
    if (!keyframe)
      {
      vtkErrorMacro("Failed to get keyframe for index " << id );
      return 0.0;
      }
    return keyframe->GetKeyTime();
    }
}

//-----------------------------------------------------------------------------
void vtkPVSimpleAnimationCue::SetKeyFrameTime(int id, double time)
{
  if (id < 0 || id >= this->GetNumberOfKeyFrames())
    {
    vtkErrorMacro("Id beyond range: " << id << ", " << time);
    return;
    }
  if (this->Virtual)
    {
    this->PointParameters[id] = time;
    this->Modified(); // Since the function modifed time in Virtual mode is
                      // PVCue modified time.
    this->InvokeEvent(vtkPVSimpleAnimationCue::KeysModifiedEvent);
    }
  else
    {
    vtkSMKeyFrameProxy* keyframe = this->KeyFrameManipulatorProxy->
      GetKeyFrameAtIndex(id);
     if (!keyframe)
      {
      vtkErrorMacro("Failed to get keyframe for index " << id );
      return;
      }
     keyframe->SetKeyTime(time);
    }
}

//-----------------------------------------------------------------------------
int vtkPVSimpleAnimationCue::AddNewKeyFrame(double time)
{
  int id = -1;
  if (this->Virtual)
    {
    if (this->NumberOfPoints >= 2)
      {
      vtkErrorMacro("When PVCue doesn't have a proxy associated with it "
        "it can only have two points.");
      return id;
      }
    this->PointParameters[this->NumberOfPoints] = time;
    id = this->NumberOfPoints;
    this->NumberOfPoints++;
    this->Modified(); // Since the function modifed time in Virtual mode is
                      // PVCue modified time.
    this->InvokeEvent(vtkPVSimpleAnimationCue::KeysModifiedEvent);
    }
  else
    {
    int numOfKeyFrames = this->GetNumberOfKeyFrames();
    if (numOfKeyFrames == 0 && time != 0)
      {
      // This stuff is to add the a keyframe at the start of the animation 
      // (let's call it pilot key frame). Note that 
      // vtkPVSimpleAnimationCue::RecordState has the additional 
      // responsibility to initialize the pilot keyframe value propertly.
      // Actually, vtkPVSimpleAnimationCue::AddNewKeyFrame gives up any 
      // gurantees about the key frames values in recording mode (only 
      // that it will be the properties current value) and the RecordState 
      // should init all the  keyframes the way it wants.
      if (this->AddNewKeyFrame(0.0) == -1) 
        {
        vtkErrorMacro("Failed to add Pilot keyframe!");
        return -1;
        }
      }
    id = this->CreateAndAddKeyFrame(time, vtkPVAnimationManager::RAMP);
    vtkPVKeyFrame* keyframe = this->GetKeyFrame(id);
    if (!this->InRecording)
      {
      if (id == 0)
        {
        keyframe->SetValueToMinimum();
        }
      else if (id == this->GetNumberOfKeyFrames()-1)
        {
        keyframe->SetValueToMaximum();
        }
      }
    }
  return id;
}
//-----------------------------------------------------------------------------
int vtkPVSimpleAnimationCue::CanDeleteKeyFrame(int index)
{
  if (this->Virtual)
    {
    return 0;
    }
  int num_keyframes = this->GetNumberOfKeyFrames();
  if (index <0 || index >= num_keyframes)
    {
    return 0;
    }
  if (index != 0 || num_keyframes == 1)
    {
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkPVSimpleAnimationCue::CanDeleteSelectedKeyFrame()
{
  if (this->Virtual)
    {
    return 0;
    }
  return this->CanDeleteKeyFrame(this->SelectedKeyFrameIndex);
}

//-----------------------------------------------------------------------------
int vtkPVSimpleAnimationCue::AppendNewKeyFrame()
{
  // First determine time.
  double step = 0.25;
  double curbounds[2];
  this->GetTimeBounds(curbounds);

  if (curbounds[1] + step > 1.0)
    {
    curbounds[1] -= step;
    this->SetTimeBounds(curbounds, 1);
    }
  int id = this->AddNewKeyFrame(1.0);
  if (id != -1)
    {
    this->SelectKeyFrame(id);
    }
  return id;
}

//-----------------------------------------------------------------------------
int vtkPVSimpleAnimationCue::GetTimeBounds(double bounds[2])
{
  int num = this->GetNumberOfKeyFrames();
  if (num <= 0)
    {
    return 0;
    }
  bounds[0] = this->GetKeyFrameTime(0);
  if (num!=1)
    {
    bounds[1] = this->GetKeyFrameTime(num-1);
    }
  else
    {
    bounds[1] = bounds[0];
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkPVSimpleAnimationCue::SetTimeBounds(double bounds[2], int enable_scaling)
{
  int num = this->GetNumberOfKeyFrames();
  if (num <0)
    {
    return;
    }
  if (enable_scaling)
    {
    double cur_bounds[2];
    if (!this->GetTimeBounds(cur_bounds))
      {
      //this should not happen!
      vtkErrorMacro("Failed to obtained current time bounds!");
      return;
      }


    double fraction = (cur_bounds[1] != cur_bounds[0]) ?
      (bounds[1] - bounds[0]) /(cur_bounds[1] - cur_bounds[0]) :
        0;

    vtkCollectionIterator* iter = this->PVKeyFrames->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); 
      iter->GoToNextItem())
      {
      vtkPVKeyFrame* pvKeyFrame = vtkPVKeyFrame::SafeDownCast(
        iter->GetCurrentObject());
      double time = pvKeyFrame->GetKeyTime();
      time = (time - cur_bounds[0]) * fraction + bounds[0];
      pvKeyFrame->SetKeyTime(time);
      }
    iter->Delete();
    }
  else
    {
    vtkPVKeyFrame* pvStart = this->GetKeyFrame(0);
    vtkPVKeyFrame* pvEnd = this->GetKeyFrame(num-1);
    if (num == 1)
      {
      pvStart->SetKeyTime(bounds[0]);
      }
    else if (num == 2)
      {
      pvStart->SetKeyTime(bounds[0]);
      pvEnd->SetKeyTime(bounds[1]);
      }
    else /* when  (num > 2) */
      {
      vtkPVKeyFrame* pvNext = this->GetKeyFrame(1);
      double time_next = pvNext->GetKeyTime();
      pvStart->SetKeyTime( (bounds[0] <= time_next)? bounds[0] : time_next );

      vtkPVKeyFrame* pvPrev = this->GetKeyFrame(num-2);
      double time_prev = pvPrev->GetKeyTime();
      pvEnd->SetKeyTime( (bounds[1] >= time_prev)? bounds[1] : time_prev );
      }
    }
}

//-----------------------------------------------------------------------------
int vtkPVSimpleAnimationCue::CreateAndAddKeyFrame(double time, int type)
{
  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(
    this->GetApplication());
  vtkPVWindow* pvWin = pvApp->GetMainWindow();
  vtkPVAnimationManager* pvAM = pvWin->GetAnimationManager(); 

  // First, synchronize the system state to the keyframe time to get proper
  // domain and property values.
  if (!this->InRecording)
    {
    // Don't do this since it disturbs the entire system.
    // Issues with domain sync are as such not addressed fully by doing this.
    // pvAM->SetCurrentTime(time);
    }
  
  ostrstream str ;
  str << "KeyFrameName_" << this->KeyFramesCreatedCount++ << ends;
  
  vtkPVKeyFrame* keyframe = pvAM->NewKeyFrame(type);
  keyframe->SetName(str.str());
  str.rdbuf()->freeze(0);
  
  keyframe->GetTraceHelper()->SetReferenceHelper(this->GetTraceHelper());
  ostrstream sCommand;
  sCommand << "GetKeyFrame \"" << keyframe->GetName() << "\"" << ends;
  keyframe->GetTraceHelper()->SetReferenceCommand(sCommand.str());
  sCommand.rdbuf()->freeze(0);

  keyframe->SetAnimationCueProxy(this->GetCueProxy()); 
  // provide a pointer to cue, so that the interace
  // can be in accordance with the animated proeprty.
  keyframe->Create(this->GetApplication(),NULL);
  keyframe->SetKeyTime(time);
  keyframe->SetKeyValue(0);
  int id = this->AddKeyFrame(keyframe);
  keyframe->Delete();

  this->InitializeKeyFrameUsingCurrentState(keyframe);
  return id;
}
//-----------------------------------------------------------------------------
void vtkPVSimpleAnimationCue::InitializeKeyFrameUsingCurrentState(
  vtkPVKeyFrame* keyframe)
{
  keyframe->InitializeKeyValueDomainUsingCurrentState();
  keyframe->InitializeKeyValueUsingCurrentState();
}

//-----------------------------------------------------------------------------
int vtkPVSimpleAnimationCue::AddKeyFrame(vtkPVKeyFrame* keyframe)
{
  if (this->Virtual)
    {
    vtkErrorMacro("Attempt to added keyframe to a Virtual Cue");
    return -1;
    }
  if (!keyframe)
    {
    return -1;
    }
  if (this->PVKeyFrames->IsItemPresent(keyframe))
    {
    vtkErrorMacro("Key frame already exists");
    return -1;
    }

  this->PVKeyFrames->AddItem(keyframe);
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->KeyFrameManipulatorProxy->GetProperty("KeyFrames"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property KeyFrames on "
      "KeyFrameManipulatorProxy.");
    return -1;
    }
  pp->AddProxy(keyframe->GetKeyFrameProxy());
  this->KeyFrameManipulatorProxy->UpdateVTKObjects();
  
  // I hate this...but what can I do, I need the index returned by the manipulator.
  this->KeyFrameManipulatorProxy->UpdateInformation();
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->KeyFrameManipulatorProxy->GetProperty("LastAddedKeyFrameIndex"));
  return ivp->GetElement(0);
}

//-----------------------------------------------------------------------------
void vtkPVSimpleAnimationCue::DeleteKeyFrame(int id)
{
  this->RemoveKeyFrame(id);
}

//-----------------------------------------------------------------------------
int vtkPVSimpleAnimationCue::RemoveKeyFrame(int id)
{
  if (id < 0 || id >= this->GetNumberOfKeyFrames())
    {
    return 0;
    }
  if (this->Virtual)
    {
    if (id == 0)
      {
      this->PointParameters[0] = this->PointParameters[1];
      }
    this->NumberOfPoints--;
    this->Modified(); // Since the function modifed time in Virtual mode is
                      // PVCue modified time.
    this->InvokeEvent(vtkPVSimpleAnimationCue::KeysModifiedEvent);
    }
  else
    {
    if (this->SelectedKeyFrameIndex == id)
      {
      this->SelectedKeyFrameIndex--;
      }
    vtkPVKeyFrame* keyframe = this->GetKeyFrame(id);
    this->RemoveKeyFrame(keyframe);
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkPVSimpleAnimationCue::RemoveKeyFrame(vtkPVKeyFrame* keyframe)
{
  if (this->Virtual)
    {
    vtkErrorMacro("Cue has no actual keyframes.");
    return;
    }
  if (!keyframe)
    {
    return;
    }
  keyframe->SetParent(NULL);
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->KeyFrameManipulatorProxy->GetProperty("KeyFrames"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property KeyFrames on "
      "KeyFrameManipulatorProxy.");
    return;
    }
  pp->RemoveProxy(keyframe->GetKeyFrameProxy());
  this->KeyFrameManipulatorProxy->UpdateVTKObjects();

  this->PVKeyFrames->RemoveItem(keyframe);
}

//-----------------------------------------------------------------------------
void vtkPVSimpleAnimationCue::ReplaceKeyFrame(vtkPVKeyFrame* oldFrame, 
  vtkPVKeyFrame* newFrame)
{
  if (this->Virtual)
    {
    vtkErrorMacro("Cue has no actual keyframes.");
    return;
    }

  newFrame->SetName(oldFrame->GetName());
  newFrame->GetTraceHelper()->SetReferenceHelper(this->GetTraceHelper());
  ostrstream sCommand;
  sCommand << "GetKeyFrame \"" << newFrame->GetName() << "\"" << ends;
  newFrame->GetTraceHelper()->SetReferenceCommand(sCommand.str());
  sCommand.rdbuf()->freeze(0);

  this->InitializeKeyFrameUsingCurrentState(newFrame);
  newFrame->SetKeyTime(oldFrame->GetKeyTime());
  newFrame->SetKeyValue(oldFrame->GetKeyValue());
  
  this->RemoveKeyFrame(oldFrame);
  this->AddKeyFrame(newFrame);

}

//-----------------------------------------------------------------------------
vtkPVKeyFrame* vtkPVSimpleAnimationCue::GetKeyFrame(const char* name)
{
  if (this->Virtual)
    {
    vtkErrorMacro("Cue has no actual keyframes");
    return NULL;
    }
  if (name == NULL)
    {
    return NULL;
    }
  vtkCollectionIterator* iter = this->PVKeyFramesIterator;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); 
    iter->GoToNextItem())
    {
    vtkPVKeyFrame* pvKeyFrame = vtkPVKeyFrame::SafeDownCast(
      iter->GetCurrentObject());
    const char* framename = pvKeyFrame->GetName();
    if (framename && strcmp(framename, name)==0)
      {
      return pvKeyFrame;
      }
    }
  return NULL;
}

//-----------------------------------------------------------------------------
vtkPVKeyFrame* vtkPVSimpleAnimationCue::GetKeyFrame(int id)
{
  if (id < 0 || id >= this->GetNumberOfKeyFrames())
    {
    vtkErrorMacro("Id out of range");
    return NULL;
    }
  if (this->Virtual)
    {
    vtkErrorMacro("Cue has no actual keyframes");
    return NULL;
    }
  vtkSMKeyFrameProxy* kfProxy = this->KeyFrameManipulatorProxy->
    GetKeyFrameAtIndex(id);
  if (!kfProxy)
    {
    vtkErrorMacro("Cannot find keyframe at index " << id );
    return NULL;
    }
  vtkCollectionIterator* iter = this->PVKeyFramesIterator;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); 
    iter->GoToNextItem())
    {
    vtkPVKeyFrame* pvKeyFrame = vtkPVKeyFrame::SafeDownCast(
      iter->GetCurrentObject());
    if (pvKeyFrame->GetKeyFrameProxy() == kfProxy)
      {
      return pvKeyFrame;
      }
    }
  return NULL;
}

//-----------------------------------------------------------------------------
void vtkPVSimpleAnimationCue::SetAnimatedProxy(vtkSMProxy *proxy)
{
  if (this->Virtual)
    {
    vtkErrorMacro("Cue does not have any actual proxies associated with it.");
    return;
    }
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->CueProxy->GetProperty("AnimatedProxy"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property AnimatedProxy.");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(proxy);
  this->CueProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkPVSimpleAnimationCue::SetAnimatedPropertyName(const char* name)
{
  if (this->Virtual)
    {
    vtkErrorMacro("Cue does not have any actual proxies associated with it.");
    return;
    }
  StringVectPropertySetElement(this->CueProxy, "AnimatedPropertyName", name);
  this->CueProxy->UpdateVTKObjects();
  if (!this->PropertyStatusManager)
    {
    this->PropertyStatusManager = vtkSMPropertyStatusManager::New();
    }
  this->PropertyStatusManager->UnregisterAllProperties();
  this->PropertyStatusManager->RegisterProperty(
    vtkSMVectorProperty::SafeDownCast(this->CueProxy->GetAnimatedProperty()));
  this->PropertyStatusManager->InitializeStatus();
}

//-----------------------------------------------------------------------------
const char* vtkPVSimpleAnimationCue::GetAnimatedPropertyName()
{
  if (this->Virtual)
    {
    return NULL;
    }
  return this->CueProxy->GetAnimatedPropertyName();
}

//-----------------------------------------------------------------------------
void vtkPVSimpleAnimationCue::SetAnimatedDomainName(const char* name)
{
  if (this->Virtual)
    {
    vtkErrorMacro("Cue does not have any actual proxies associated with it.");
    return;
    }
  StringVectPropertySetElement(this->CueProxy, "AnimatedDomainName", name);
  this->CueProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkPVSimpleAnimationCue::SetAnimatedElement(int index)
{
  if (this->Virtual)
    {
    vtkErrorMacro("Cue does not have any actual proxies associated with it.");
    return;
    }
  IntVectPropertySetElement(this->CueProxy,"AnimatedElement", index);
  this->CueProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkPVSimpleAnimationCue::StartRecording()
{
  if (this->InRecording)
    {
    return;
    }
  if (this->PropertyStatusManager)
    {
    this->PropertyStatusManager->InitializeStatus();
    }
  this->InRecording = 1;
  /*
  if (!this->Virtual)
    {
    this->TimeLine->DisableAddAndRemoveOn();
    }
    */
  this->PreviousStepKeyFrameAdded = 0;
}

//-----------------------------------------------------------------------------
void vtkPVSimpleAnimationCue::StopRecording()
{
  if (!this->InRecording)
    {
    return;
    }
  this->InRecording = 0;
  /*
  if (!this->Virtual)
    {
    this->TimeLine->SetDisableAddAndRemove(0);
    }
    */
}

//-----------------------------------------------------------------------------
void vtkPVSimpleAnimationCue::RecordState(double ntime, double offset)
{
  if (!this->InRecording)
    {
    vtkErrorMacro("Not in recording mode.");  
    return;
    }
  
  if (this->Virtual || !this->PropertyStatusManager)
    {
    return;
    }

  vtkSMProperty* property = this->CueProxy->GetAnimatedProperty();
  int index = this->CueProxy->GetAnimatedElement();
  
  if (!this->PropertyStatusManager->HasPropertyChanged(
      vtkSMVectorProperty::SafeDownCast(property), index ))
    {
    this->PreviousStepKeyFrameAdded = 0;
    return;
    }
  // animated property has changed.
  // add a keyframe at ntime.
  int old_numOfKeyFrames = this->GetNumberOfKeyFrames();
  
  if (!this->PreviousStepKeyFrameAdded)
    {
    int id = this->AddNewKeyFrame(ntime);
    if (id == -1)
      {
      vtkErrorMacro("Failed to add new key frame");
      return;
      }
    this->GetKeyFrame(id)->InitializeKeyValueUsingProperty(
      this->PropertyStatusManager->GetInternalProperty(vtkSMVectorProperty::SafeDownCast(property)),
      index);
    
    if (old_numOfKeyFrames == 0)
      {
      //Pilot keyframe also needs to be initilaized.
      this->GetKeyFrame(0)->InitializeKeyValueUsingProperty(
        this->PropertyStatusManager->GetInternalProperty(vtkSMVectorProperty::SafeDownCast(property)),
        index);
      }
    }
  int id2 = this->AddNewKeyFrame(ntime + offset);
  if (id2 == -1)
    {
    vtkErrorMacro("Failed to add new key frame");
    return;
    }
  this->PreviousStepKeyFrameAdded = 1;
  if (this->PropertyStatusManager)
    {
    this->PropertyStatusManager->InitializeStatus();
    }
}
//-----------------------------------------------------------------------------
void vtkPVSimpleAnimationCue::SelectKeyFrame(int id)
{
  this->SelectKeyFrameInternal(id);
}

//-----------------------------------------------------------------------------
void vtkPVSimpleAnimationCue::SelectKeyFrameInternal(int id)
{
  this->SelectedKeyFrameIndex = id;
  this->InvokeEvent(vtkPVSimpleAnimationCue::SelectionChangedEvent);
}

//-----------------------------------------------------------------------------
void vtkPVSimpleAnimationCue::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();
}

//-----------------------------------------------------------------------------
void vtkPVSimpleAnimationCue::PrintSelf(ostream& os ,vtkIndent indent) 
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LabelText: " << this->LabelText << endl;
  os << indent << "SelectedKeyFrameIndex: " << this->SelectedKeyFrameIndex << endl;
  os << indent << "CueProxy: " << this->CueProxy << endl;
  os << indent << "Virtual: " << this->Virtual << endl;
  os << indent << "ProxiesRegistered: " << this->ProxiesRegistered << endl;
  os << indent << "NumberOfPoints: " << this->NumberOfPoints << endl;
  os << indent << "PointParameters: " << this->PointParameters[0] <<
    ", " << this->PointParameters[1] << endl;
  os << indent << "CueProxyName: " << 
    ((this->CueProxyName)? this->CueProxyName : "NULL") << endl;
  os << indent << "CueProxy: " << this->CueProxy << endl;
  os << indent << "KeyFrameManipulatorProxyName: " <<
    ((this->KeyFrameManipulatorProxyName)? 
     this->KeyFrameManipulatorProxyName : "NULL") << endl;
  os << indent << "KeyFrameManipulatorProxy: " << 
    this->KeyFrameManipulatorProxy << endl;
}
