/*=========================================================================

  Program:   ParaView
  Module:    vtkSMKeyFrameAnimationCueManipulatorProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMKeyFrameAnimationCueManipulatorProxy.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMKeyFrameProxy.h"
#include "vtkSMProperty.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkSMKeyFrameAnimationCueManipulatorProxy);

//****************************************************************************
class vtkSMKeyFrameAnimationCueManipulatorProxyObserver : public vtkCommand
{
public:
  static vtkSMKeyFrameAnimationCueManipulatorProxyObserver* New()
    {
    return new vtkSMKeyFrameAnimationCueManipulatorProxyObserver;
    }
  void SetKeyFrameAnimationCueManipulatorProxy(
    vtkSMKeyFrameAnimationCueManipulatorProxy* proxy)
    {
    this->KeyFrameAnimationCueManipulatorProxy = proxy;
    }

  virtual void Execute(vtkObject* obj, unsigned long event, void* calldata)
    {
    if (this->KeyFrameAnimationCueManipulatorProxy)
      {
      this->KeyFrameAnimationCueManipulatorProxy->ExecuteEvent(obj, event, 
        calldata);
      }
    }
protected:
  vtkSMKeyFrameAnimationCueManipulatorProxyObserver()
    {
    this->KeyFrameAnimationCueManipulatorProxy = 0;
    }
  vtkSMKeyFrameAnimationCueManipulatorProxy* 
    KeyFrameAnimationCueManipulatorProxy;
};


//****************************************************************************
class vtkSMKeyFrameAnimationCueManipulatorProxyInternals
{
public:
  typedef vtkstd::vector<vtkSMKeyFrameProxy*> KeyFrameVector;
  KeyFrameVector KeyFrames;
};

//****************************************************************************
//----------------------------------------------------------------------------
vtkSMKeyFrameAnimationCueManipulatorProxy::vtkSMKeyFrameAnimationCueManipulatorProxy()
{
  this->Internals = new vtkSMKeyFrameAnimationCueManipulatorProxyInternals;
  this->Observer = vtkSMKeyFrameAnimationCueManipulatorProxyObserver::New();
  this->Observer->SetKeyFrameAnimationCueManipulatorProxy(this);
  this->CueStarter = 0;
  this->SendEndEvent = 0;
  this->LastAddedKeyFrameIndex = 0;
  this->CueStarterInitialized = false;
}

//----------------------------------------------------------------------------
vtkSMKeyFrameAnimationCueManipulatorProxy::~vtkSMKeyFrameAnimationCueManipulatorProxy()
{
  this->RemoveAllKeyFrames();

  delete this->Internals;
  this->Observer->Delete();
}

//----------------------------------------------------------------------------
void vtkSMKeyFrameAnimationCueManipulatorProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->CueStarter = vtkSMKeyFrameProxy::SafeDownCast(
    this->GetSubProxy("CueStarter"));

  this->Superclass::CreateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMKeyFrameAnimationCueManipulatorProxy::Initialize(vtkSMAnimationCueProxy*)
{
  this->SendEndEvent = 1;
  this->CueStarterInitialized = false;
  if (this->CueStarter && this->GetNumberOfKeyFrames() > 0)
    {
    vtkSMKeyFrameProxy* firstKF = this->GetEndKeyFrame(0.0);
    if (firstKF && firstKF->GetKeyTime() > 0.0)
      {
      this->CueStarter->Copy(firstKF, "vtkSMProxyProperty");
      vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
        this->CueStarter->GetProperty("KeyTime"));
      dvp->SetElement(0, 0);
      this->CueStarter->UpdateVTKObjects();
      this->CueStarterInitialized = true;
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMKeyFrameAnimationCueManipulatorProxy::Finalize(vtkSMAnimationCueProxy* cue)
{
  this->CueStarterInitialized = false;
  if (this->SendEndEvent)
    {
    this->UpdateValue(1.0, cue);
    }
}

//----------------------------------------------------------------------------
int vtkSMKeyFrameAnimationCueManipulatorProxy::AddKeyFrame(vtkSMKeyFrameProxy* keyframe)
{
  int index = this->AddKeyFrameInternal(keyframe);
  if (index != -1)
    {
    keyframe->AddObserver(vtkCommand::ModifiedEvent, this->Observer);
    keyframe->Register(this);
    this->UpdateKeyTimeDomains();
    }
  this->LastAddedKeyFrameIndex = index;
  this->Modified();
  return index;
}

//----------------------------------------------------------------------------
void vtkSMKeyFrameAnimationCueManipulatorProxy::RemoveKeyFrame(
  vtkSMKeyFrameProxy* keyframe)
{
  if (this->RemoveKeyFrameInternal(keyframe))
    {
    keyframe->RemoveObservers(vtkCommand::ModifiedEvent, this->Observer);
    keyframe->UnRegister(this);
    this->UpdateKeyTimeDomains();
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSMKeyFrameAnimationCueManipulatorProxy::RemoveAllKeyFrames()
{
  vtkSMKeyFrameAnimationCueManipulatorProxyInternals::KeyFrameVector::
    iterator iter;
  for (iter = this->Internals->KeyFrames.begin();
    iter != this->Internals->KeyFrames.end();
    iter++)
    {
      (*iter)->RemoveObservers(vtkCommand::ModifiedEvent, this->Observer);
      (*iter)->UnRegister(this);
    }
  this->Internals->KeyFrames.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
unsigned int vtkSMKeyFrameAnimationCueManipulatorProxy::GetNumberOfKeyFrames()
{
  return this->Internals->KeyFrames.size();
}

//----------------------------------------------------------------------------
vtkSMKeyFrameProxy* vtkSMKeyFrameAnimationCueManipulatorProxy::GetKeyFrame(
  double time)
{
  vtkSMKeyFrameAnimationCueManipulatorProxyInternals::KeyFrameVector::
    iterator iter;
  for (iter = this->Internals->KeyFrames.begin();
    iter != this->Internals->KeyFrames.end();
    iter++)
    {
    if ( (*iter)->GetKeyTime() == time)
      {
      return *iter;
      }
    }
  
  return NULL;
}

//----------------------------------------------------------------------------
vtkSMKeyFrameProxy* vtkSMKeyFrameAnimationCueManipulatorProxy::GetStartKeyFrame(
  double time)
{
  // we use the fact that we have maintained the vector in sorted order.
  vtkSMKeyFrameProxy* proxy = NULL;
  vtkSMKeyFrameAnimationCueManipulatorProxyInternals::KeyFrameVector::
    iterator it = this->Internals->KeyFrames.begin();
  for (; it != this->Internals->KeyFrames.end(); it++)
    {
    double cur_time = (*it)->GetKeyTime();
    if ( cur_time == time)
      {
      return *it;
      }
    if ( cur_time > time)
      {
      return proxy;
      }
    proxy = *it;;
    }
  return NULL;
}

//----------------------------------------------------------------------------
vtkSMKeyFrameProxy* vtkSMKeyFrameAnimationCueManipulatorProxy::
GetEndKeyFrame(double time)
{
  vtkSMKeyFrameAnimationCueManipulatorProxyInternals::KeyFrameVector::
    iterator it = this->Internals->KeyFrames.begin();
  for (; it != this->Internals->KeyFrames.end(); it++)
    {
    if ( (*it)->GetKeyTime() >= time)
      {
      return *it;
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkSMKeyFrameAnimationCueManipulatorProxy::UpdateValue(double currenttime, 
    vtkSMAnimationCueProxy* cueproxy)
{
  if (!cueproxy)
    {
    vtkErrorMacro("UpdateValue called with invalid arguments");
    return;
    }

  if (this->GetNumberOfKeyFrames() < 2)
    {
    //vtkErrorMacro("Too few keyframe to animate.");
    return;
    }
  
  vtkSMKeyFrameProxy* startKF = this->GetStartKeyFrame(currenttime);
  if (!startKF && this->CueStarterInitialized)
    {
    // If the first keyframe use added has key time > 0.0, we create a copy of
    // the first keyframe and add it at time 0.0. The type of the copy is
    // typically a BooleanKeyFrame and is defined in the xml configuration.
    startKF = this->CueStarter;
    }
  vtkSMKeyFrameProxy* endKF = this->GetEndKeyFrame(currenttime);
  if (startKF && endKF)
    {
    // normalized time to the range between start key frame and end key frame.
    double ctime = 0;
    double tmin = startKF->GetKeyTime();
    double tmax = endKF->GetKeyTime();

    if (tmin != tmax)
      {
      ctime = (currenttime - tmin)/ (tmax-tmin);
      }
    startKF->UpdateValue(ctime, cueproxy, endKF);
    this->InvokeEvent(vtkSMAnimationCueManipulatorProxy::StateModifiedEvent);
    }
  // check to see if the curtime has crossed the last key frame and if
  // we should make the state of the property as left by the last key frame.
  else if (this->SendEndEvent)
    {
    int num = this->GetNumberOfKeyFrames();
    vtkSMKeyFrameProxy* lastKF = this->GetKeyFrameAtIndex(num-1);
    if (currenttime >= lastKF->GetKeyTime())
      {
      lastKF->UpdateValue(0, cueproxy,lastKF);
      this->SendEndEvent = 0;
      this->InvokeEvent(vtkSMAnimationCueManipulatorProxy::StateModifiedEvent);
      }
    }
}

//----------------------------------------------------------------------------
vtkSMKeyFrameProxy* vtkSMKeyFrameAnimationCueManipulatorProxy::GetNextKeyFrame(
  vtkSMKeyFrameProxy* keyFrame)
{
  vtkSMKeyFrameAnimationCueManipulatorProxyInternals::KeyFrameVector::
    iterator it = this->Internals->KeyFrames.begin();
  for (; it != this->Internals->KeyFrames.end(); it++)
    {
    if ( *it == keyFrame)
      {
      it++;
      if (it != this->Internals->KeyFrames.end())
        {
        return (*it);
        }
      break;
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
vtkSMKeyFrameProxy* vtkSMKeyFrameAnimationCueManipulatorProxy::GetPreviousKeyFrame(
  vtkSMKeyFrameProxy* keyFrame)
{
  vtkSMKeyFrameProxy* proxy = NULL;
  
  vtkSMKeyFrameAnimationCueManipulatorProxyInternals::KeyFrameVector::
    iterator it = this->Internals->KeyFrames.begin();
  for (; it != this->Internals->KeyFrames.end(); it++)
    {
    if ( *it == keyFrame)
      {
      return proxy;
      }
    proxy = *it;
    }  
  return NULL;
}

//----------------------------------------------------------------------------
vtkSMKeyFrameProxy* vtkSMKeyFrameAnimationCueManipulatorProxy::GetKeyFrameAtIndex(
  int index)
{
  if (index < 0 || index >= static_cast<int>(this->GetNumberOfKeyFrames()))
    {
    vtkErrorMacro("Index beyond range");
    return NULL;
    }
  return this->Internals->KeyFrames[index];
}

//----------------------------------------------------------------------------
void vtkSMKeyFrameAnimationCueManipulatorProxy::ExecuteEvent(
  vtkObject* obj, unsigned long event, void* )
{
  vtkSMKeyFrameProxy* keyframe = vtkSMKeyFrameProxy::SafeDownCast(obj);

  if (keyframe && event == vtkCommand::ModifiedEvent)
    {
    // Check if the keyframe position has changed.
    vtkSMKeyFrameProxy* prev = this->GetPreviousKeyFrame(keyframe);
    vtkSMKeyFrameProxy* next = this->GetNextKeyFrame(keyframe);
    double keytime = keyframe->GetKeyTime();
    if ( (next && keytime > next->GetKeyTime()) || 
      (prev && keytime < prev->GetKeyTime()))
      {
      // Position of keyframe has changed.
      this->RemoveKeyFrameInternal(keyframe);
      this->AddKeyFrameInternal(keyframe);
      }
    }
  this->UpdateKeyTimeDomains();
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkSMKeyFrameAnimationCueManipulatorProxy::AddKeyFrameInternal(
  vtkSMKeyFrameProxy* keyframe)
{
  double time = keyframe->GetKeyTime();
  int index = 0;
  vtkSMKeyFrameAnimationCueManipulatorProxyInternals::KeyFrameVector::
    iterator iter;
  for (iter = this->Internals->KeyFrames.begin();
    iter != this->Internals->KeyFrames.end();
    iter++, index++)
    {
    if ( *iter == keyframe)
      {
      vtkErrorMacro("Keyframe already exists");
      return -1;
      }
    if ( (*iter)->GetKeyTime() > time )
      {
      break;
      }
    }
  this->Internals->KeyFrames.insert(iter, keyframe);
  return index;
}

//----------------------------------------------------------------------------
int vtkSMKeyFrameAnimationCueManipulatorProxy::RemoveKeyFrameInternal(
  vtkSMKeyFrameProxy* keyframe)
{
  vtkSMKeyFrameAnimationCueManipulatorProxyInternals::KeyFrameVector::
    iterator iter;
  for (iter = this->Internals->KeyFrames.begin();
    iter != this->Internals->KeyFrames.end();
    iter++)
    {
    if ( *iter == keyframe)
      {
      this->Internals->KeyFrames.erase(iter);
      return 1;
      }
    }
  return 0;
}


//----------------------------------------------------------------------------
void vtkSMKeyFrameAnimationCueManipulatorProxy::UpdateKeyTimeDomains()
{
  vtkSMKeyFrameAnimationCueManipulatorProxyInternals::KeyFrameVector::
    iterator iter;
  unsigned int numFrames = this->Internals->KeyFrames.size();
  for (unsigned int cc=0; cc < numFrames; ++cc)
    {
    vtkSMKeyFrameProxy* kf = this->Internals->KeyFrames[cc];
    vtkSMKeyFrameProxy* prev = (cc>0)? this->Internals->KeyFrames[cc-1] : NULL;
    vtkSMKeyFrameProxy* next = (cc+1<numFrames)? this->Internals->KeyFrames[cc+1]: NULL;
    double min = (prev)? prev->GetKeyTime() : 0.0;
    double max = (next)? next->GetKeyTime() : 1.0;
    vtkSMProperty* keyTimeProp = kf->GetProperty("KeyTime");
    if (!keyTimeProp)
      {
      vtkWarningMacro("KeyFrameProxy should have a KeyTime property.");
      continue;
      }
    vtkSMDoubleRangeDomain* dr = vtkSMDoubleRangeDomain::SafeDownCast(
      keyTimeProp->GetDomain("range"));
    if (dr)
      {
      int exists;
      if (dr->GetMinimum(0, exists) != min || !exists)
        {
        dr->AddMinimum(0, min);
        }
      if (dr->GetMaximum(0, exists) != max || !exists)
        {
        dr->AddMaximum(0, max);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMKeyFrameAnimationCueManipulatorProxy::PrintSelf(ostream& os, 
  vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LastAddedKeyFrameIndex: " << this->LastAddedKeyFrameIndex
    << endl;
}
