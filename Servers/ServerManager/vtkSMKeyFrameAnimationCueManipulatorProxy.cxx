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

#include "vtkObjectFactory.h"
#include "vtkSMKeyFrameProxy.h"
#include "vtkCommand.h"
#include "vtkClientServerID.h"

#include <vtkstd/vector>
vtkCxxRevisionMacro(vtkSMKeyFrameAnimationCueManipulatorProxy, "1.3");
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
  typedef vtkstd::vector<vtkSMKeyFrameProxy*> DoubleToKeyFrameVector;
  DoubleToKeyFrameVector KeyFrames;
};

//****************************************************************************
//----------------------------------------------------------------------------
vtkSMKeyFrameAnimationCueManipulatorProxy::vtkSMKeyFrameAnimationCueManipulatorProxy()
{
  this->Internals = new vtkSMKeyFrameAnimationCueManipulatorProxyInternals;
  this->Observer = vtkSMKeyFrameAnimationCueManipulatorProxyObserver::New();
  this->Observer->SetKeyFrameAnimationCueManipulatorProxy(this);
  this->SendEndEvent = 0;
}

//----------------------------------------------------------------------------
vtkSMKeyFrameAnimationCueManipulatorProxy::~vtkSMKeyFrameAnimationCueManipulatorProxy()
{
  this->RemoveAllKeyFrames();
  delete this->Internals;
  this->Observer->Delete();
}

//----------------------------------------------------------------------------
void vtkSMKeyFrameAnimationCueManipulatorProxy::Initialize()
{
  this->SendEndEvent = 1;
}

//----------------------------------------------------------------------------
int vtkSMKeyFrameAnimationCueManipulatorProxy::AddKeyFrame(vtkSMKeyFrameProxy* keyframe)
{
  vtkSMKeyFrameAnimationCueManipulatorProxyInternals::DoubleToKeyFrameVector::
    iterator iter;

  int index = this->AddKeyFrameInternal(keyframe);
  if (index != -1)
    {
    keyframe->AddObserver(vtkCommand::ModifiedEvent, this->Observer);
    keyframe->Register(this);
    }
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
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSMKeyFrameAnimationCueManipulatorProxy::RemoveAllKeyFrames()
{
  vtkSMKeyFrameAnimationCueManipulatorProxyInternals::DoubleToKeyFrameVector::
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
  vtkSMKeyFrameAnimationCueManipulatorProxyInternals::DoubleToKeyFrameVector::
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
  vtkSMKeyFrameAnimationCueManipulatorProxyInternals::DoubleToKeyFrameVector::
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
  vtkSMKeyFrameAnimationCueManipulatorProxyInternals::DoubleToKeyFrameVector::
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
    vtkErrorMacro("Too few keyframe to animate.");
    return;
    }
  
  vtkSMKeyFrameProxy* startKF = 
    this->GetStartKeyFrame(currenttime);
  vtkSMKeyFrameProxy* endKF = 
    this->GetEndKeyFrame(currenttime);
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
    this->InvokeEvent(vtkSMKeyFrameAnimationCueManipulatorProxy::StateModifiedEvent);
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
      this->InvokeEvent(vtkSMKeyFrameAnimationCueManipulatorProxy::StateModifiedEvent);
      }
    }
}

//----------------------------------------------------------------------------
vtkSMKeyFrameProxy* vtkSMKeyFrameAnimationCueManipulatorProxy::GetNextKeyFrame(
  vtkSMKeyFrameProxy* keyFrame)
{
  vtkSMKeyFrameAnimationCueManipulatorProxyInternals::DoubleToKeyFrameVector::
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
  /*
  vtkSMKeyFrameAnimationCueManipulatorProxyInternals::DoubleToKeyFrameMap::
  iterator it = this->Internals->KeyFrames.find(keyFrame->GetKeyTime());
  if (it == this->Internals->KeyFrames.end())
  {
  vtkErrorMacro("Key frame does not exists in the sequence");
  return NULL;
  }
  it++;
  if (it == this->Internals->KeyFrames.end())
  {
  // no next keyframe.
  return NULL;
  }
  return it->second;
  */
}

//----------------------------------------------------------------------------
vtkSMKeyFrameProxy* vtkSMKeyFrameAnimationCueManipulatorProxy::GetPreviousKeyFrame(
  vtkSMKeyFrameProxy* keyFrame)
{
  vtkSMKeyFrameProxy* proxy = NULL;
  
  vtkSMKeyFrameAnimationCueManipulatorProxyInternals::DoubleToKeyFrameVector::
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
/*
  vtkSMKeyFrameAnimationCueManipulatorProxyInternals::DoubleToKeyFrameMap::
    iterator it = this->Internals->KeyFrames.find(keyFrame->GetKeyTime());
  if (it == this->Internals->KeyFrames.end())
    {
    vtkErrorMacro("Key frame does not exists in the sequence");
    return NULL;
    }
  if (it == this->Internals->KeyFrames.begin())
    {
    // no previous key frame.
    return NULL;
    }
  it--;
  return it->second;
  */
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
void vtkSMKeyFrameAnimationCueManipulatorProxy::SaveInBatchScript(ofstream* file)
{
  vtkClientServerID id = this->SelfID;
  this->Superclass::SaveInBatchScript(file);

  vtkSMKeyFrameAnimationCueManipulatorProxyInternals::DoubleToKeyFrameVector::
    iterator it = this->Internals->KeyFrames.begin();
  for (; it != this->Internals->KeyFrames.end(); it++)
    {
    vtkSMKeyFrameProxy* proxy = *it; 
    proxy->SaveInBatchScript(file);
    *file << "  [$pvTemp" << id << " GetProperty KeyFrame]"
      <<" RemoveAllProxies" << endl;
    *file << "  [$pvTemp" << id << " GetProperty KeyFrame]"
      <<" AddProxy $pvTemp" << proxy->GetID() << endl;
    *file << "  $pvTemp" << id << " UpdateVTKObjects" << endl;
    }
}

//----------------------------------------------------------------------------
void vtkSMKeyFrameAnimationCueManipulatorProxy::ExecuteEvent(
  vtkObject* obj, unsigned long event, void* )
{
  vtkSMKeyFrameProxy* keyframe = vtkSMKeyFrameProxy::SafeDownCast(obj);

  if (keyframe && event == vtkCommand::ModifiedEvent)
    {
    this->RemoveKeyFrameInternal(keyframe);
    this->AddKeyFrameInternal(keyframe);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
int vtkSMKeyFrameAnimationCueManipulatorProxy::AddKeyFrameInternal(
  vtkSMKeyFrameProxy* keyframe)
{
  double time = keyframe->GetKeyTime();
  int index = 0;
  vtkSMKeyFrameAnimationCueManipulatorProxyInternals::DoubleToKeyFrameVector::
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
  vtkSMKeyFrameAnimationCueManipulatorProxyInternals::DoubleToKeyFrameVector::
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
void vtkSMKeyFrameAnimationCueManipulatorProxy::PrintSelf(ostream& os, 
  vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
