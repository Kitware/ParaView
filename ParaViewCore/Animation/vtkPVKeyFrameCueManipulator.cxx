/*=========================================================================

  Program:   ParaView
  Module:    vtkPVKeyFrameCueManipulator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVKeyFrameCueManipulator.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVKeyFrame.h"

#include <vector>

//****************************************************************************
class vtkPVKeyFrameCueManipulatorObserver : public vtkCommand
{
public:
  static vtkPVKeyFrameCueManipulatorObserver* New()
  {
    return new vtkPVKeyFrameCueManipulatorObserver;
  }
  void SetKeyFrameAnimationCueManipulatorProxy(vtkPVKeyFrameCueManipulator* proxy)
  {
    this->KeyFrameAnimationCueManipulatorProxy = proxy;
  }

  void Execute(vtkObject* obj, unsigned long event, void* calldata) override
  {
    if (this->KeyFrameAnimationCueManipulatorProxy)
    {
      this->KeyFrameAnimationCueManipulatorProxy->ExecuteEvent(obj, event, calldata);
    }
  }

protected:
  vtkPVKeyFrameCueManipulatorObserver() { this->KeyFrameAnimationCueManipulatorProxy = 0; }
  vtkPVKeyFrameCueManipulator* KeyFrameAnimationCueManipulatorProxy;
};
//----------------------------------------------------------------------------
class vtkPVKeyFrameCueManipulatorInternals
{
public:
  typedef std::vector<vtkPVKeyFrame*> KeyFrameVector;
  KeyFrameVector KeyFrames;
};
//****************************************************************************
vtkStandardNewMacro(vtkPVKeyFrameCueManipulator);
//----------------------------------------------------------------------------
vtkPVKeyFrameCueManipulator::vtkPVKeyFrameCueManipulator()
{
  this->Internals = new vtkPVKeyFrameCueManipulatorInternals;
  this->Observer = vtkPVKeyFrameCueManipulatorObserver::New();
  this->Observer->SetKeyFrameAnimationCueManipulatorProxy(this);
  this->SendEndEvent = 0;
  this->LastAddedKeyFrameIndex = 0;
}

//----------------------------------------------------------------------------
vtkPVKeyFrameCueManipulator::~vtkPVKeyFrameCueManipulator()
{
  this->RemoveAllKeyFrames();

  delete this->Internals;
  this->Observer->Delete();
}

//----------------------------------------------------------------------------
void vtkPVKeyFrameCueManipulator::Initialize(vtkPVAnimationCue*)
{
  this->SendEndEvent = 1;
}

//----------------------------------------------------------------------------
void vtkPVKeyFrameCueManipulator::Finalize(vtkPVAnimationCue* cue)
{
  if (this->SendEndEvent)
  {
    this->UpdateValue(1.0, cue);
  }
}

//----------------------------------------------------------------------------
int vtkPVKeyFrameCueManipulator::AddKeyFrame(vtkPVKeyFrame* keyframe)
{
  int index = this->AddKeyFrameInternal(keyframe);
  if (index != -1)
  {
    keyframe->AddObserver(vtkCommand::ModifiedEvent, this->Observer);
    keyframe->Register(this);
    // this->UpdateKeyTimeDomains();
  }
  this->LastAddedKeyFrameIndex = index;
  this->Modified();
  return index;
}

//----------------------------------------------------------------------------
void vtkPVKeyFrameCueManipulator::RemoveKeyFrame(vtkPVKeyFrame* keyframe)
{
  if (this->RemoveKeyFrameInternal(keyframe))
  {
    keyframe->RemoveObservers(vtkCommand::ModifiedEvent, this->Observer);
    keyframe->UnRegister(this);
    // this->UpdateKeyTimeDomains();
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVKeyFrameCueManipulator::RemoveAllKeyFrames()
{
  vtkPVKeyFrameCueManipulatorInternals::KeyFrameVector::iterator iter;
  for (iter = this->Internals->KeyFrames.begin(); iter != this->Internals->KeyFrames.end(); iter++)
  {
    (*iter)->RemoveObservers(vtkCommand::ModifiedEvent, this->Observer);
    (*iter)->UnRegister(this);
  }
  this->Internals->KeyFrames.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
unsigned int vtkPVKeyFrameCueManipulator::GetNumberOfKeyFrames()
{
  return static_cast<unsigned int>(this->Internals->KeyFrames.size());
}

//----------------------------------------------------------------------------
vtkPVKeyFrame* vtkPVKeyFrameCueManipulator::GetKeyFrame(double time)
{
  vtkPVKeyFrameCueManipulatorInternals::KeyFrameVector::iterator iter;
  for (iter = this->Internals->KeyFrames.begin(); iter != this->Internals->KeyFrames.end(); iter++)
  {
    if ((*iter)->GetKeyTime() == time)
    {
      return *iter;
    }
  }

  return NULL;
}

//----------------------------------------------------------------------------
vtkPVKeyFrame* vtkPVKeyFrameCueManipulator::GetStartKeyFrame(double time)
{
  // we use the fact that we have maintained the vector in sorted order.
  vtkPVKeyFrame* proxy = NULL;
  vtkPVKeyFrameCueManipulatorInternals::KeyFrameVector::iterator it =
    this->Internals->KeyFrames.begin();
  for (; it != this->Internals->KeyFrames.end(); it++)
  {
    double cur_time = (*it)->GetKeyTime();
    if (cur_time == time)
    {
      return *it;
    }
    if (cur_time > time)
    {
      return proxy;
    }
    proxy = *it;
    ;
  }
  return NULL;
}

//----------------------------------------------------------------------------
vtkPVKeyFrame* vtkPVKeyFrameCueManipulator::GetEndKeyFrame(double time)
{
  vtkPVKeyFrameCueManipulatorInternals::KeyFrameVector::iterator it =
    this->Internals->KeyFrames.begin();
  for (; it != this->Internals->KeyFrames.end(); it++)
  {
    if ((*it)->GetKeyTime() >= time)
    {
      return *it;
    }
  }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkPVKeyFrameCueManipulator::UpdateValue(double currenttime, vtkPVAnimationCue* cue)
{
  if (!cue)
  {
    vtkErrorMacro("UpdateValue called with invalid arguments");
    return;
  }

  if (this->GetNumberOfKeyFrames() < 2)
  {
    // vtkErrorMacro("Too few keyframe to animate.");
    return;
  }

  vtkPVKeyFrame* startKF = this->GetStartKeyFrame(currenttime);
  vtkPVKeyFrame* endKF = this->GetEndKeyFrame(currenttime);
  if (endKF && startKF == NULL)
  {
    // This means that we are at a time location before the first keyframe in
    // this cue. In that case, simply duplicate the first key-frame as the
    // chosen one.
    endKF->UpdateValue(0, cue, endKF);
    this->InvokeEvent(vtkPVCueManipulator::StateModifiedEvent);
  }
  if (startKF && endKF)
  {
    // normalized time to the range between start key frame and end key frame.
    double ctime = 0;
    double tmin = startKF->GetKeyTime();
    double tmax = endKF->GetKeyTime();

    if (tmin != tmax)
    {
      ctime = (currenttime - tmin) / (tmax - tmin);
    }
    startKF->UpdateValue(ctime, cue, endKF);
    this->InvokeEvent(vtkPVCueManipulator::StateModifiedEvent);
  }
  // check to see if the curtime has crossed the last key frame and if
  // we should make the state of the property as left by the last key frame.
  else if (this->SendEndEvent)
  {
    int num = this->GetNumberOfKeyFrames();
    vtkPVKeyFrame* lastKF = this->GetKeyFrameAtIndex(num - 1);
    if (currenttime >= lastKF->GetKeyTime())
    {
      lastKF->UpdateValue(0, cue, lastKF);
      this->SendEndEvent = 0;
      this->InvokeEvent(vtkPVCueManipulator::StateModifiedEvent);
    }
  }
}

//----------------------------------------------------------------------------
vtkPVKeyFrame* vtkPVKeyFrameCueManipulator::GetNextKeyFrame(vtkPVKeyFrame* keyFrame)
{
  vtkPVKeyFrameCueManipulatorInternals::KeyFrameVector::iterator it =
    this->Internals->KeyFrames.begin();
  for (; it != this->Internals->KeyFrames.end(); it++)
  {
    if (*it == keyFrame)
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
vtkPVKeyFrame* vtkPVKeyFrameCueManipulator::GetPreviousKeyFrame(vtkPVKeyFrame* keyFrame)
{
  vtkPVKeyFrame* proxy = NULL;

  vtkPVKeyFrameCueManipulatorInternals::KeyFrameVector::iterator it =
    this->Internals->KeyFrames.begin();
  for (; it != this->Internals->KeyFrames.end(); it++)
  {
    if (*it == keyFrame)
    {
      return proxy;
    }
    proxy = *it;
  }
  return NULL;
}

//----------------------------------------------------------------------------
vtkPVKeyFrame* vtkPVKeyFrameCueManipulator::GetKeyFrameAtIndex(int index)
{
  if (index < 0 || index >= static_cast<int>(this->GetNumberOfKeyFrames()))
  {
    vtkErrorMacro("Index beyond range");
    return NULL;
  }
  return this->Internals->KeyFrames[index];
}

//----------------------------------------------------------------------------
void vtkPVKeyFrameCueManipulator::ExecuteEvent(vtkObject* obj, unsigned long event, void*)
{
  vtkPVKeyFrame* keyframe = vtkPVKeyFrame::SafeDownCast(obj);

  if (keyframe && event == vtkCommand::ModifiedEvent)
  {
    // Check if the keyframe position has changed.
    vtkPVKeyFrame* prev = this->GetPreviousKeyFrame(keyframe);
    vtkPVKeyFrame* next = this->GetNextKeyFrame(keyframe);
    double keytime = keyframe->GetKeyTime();
    if ((next && keytime > next->GetKeyTime()) || (prev && keytime < prev->GetKeyTime()))
    {
      // Position of keyframe has changed.
      this->RemoveKeyFrameInternal(keyframe);
      this->AddKeyFrameInternal(keyframe);
    }
  }
  // this->UpdateKeyTimeDomains();
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkPVKeyFrameCueManipulator::AddKeyFrameInternal(vtkPVKeyFrame* keyframe)
{
  double time = keyframe->GetKeyTime();
  int index = 0;
  vtkPVKeyFrameCueManipulatorInternals::KeyFrameVector::iterator iter;
  for (iter = this->Internals->KeyFrames.begin(); iter != this->Internals->KeyFrames.end();
       iter++, index++)
  {
    if (*iter == keyframe)
    {
      vtkErrorMacro("Keyframe already exists");
      return -1;
    }
    if ((*iter)->GetKeyTime() > time)
    {
      break;
    }
  }
  this->Internals->KeyFrames.insert(iter, keyframe);
  return index;
}

//----------------------------------------------------------------------------
int vtkPVKeyFrameCueManipulator::RemoveKeyFrameInternal(vtkPVKeyFrame* kf)
{
  vtkPVKeyFrameCueManipulatorInternals::KeyFrameVector::iterator iter;
  for (iter = this->Internals->KeyFrames.begin(); iter != this->Internals->KeyFrames.end(); iter++)
  {
    if (*iter == kf)
    {
      this->Internals->KeyFrames.erase(iter);
      return 1;
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
#ifdef FIXME
void vtkPVKeyFrameCueManipulator::UpdateKeyTimeDomains()
{
  vtkPVKeyFrameCueManipulatorInternals::KeyFrameVector::iterator iter;
  unsigned int numFrames = this->Internals->KeyFrames.size();
  for (unsigned int cc = 0; cc < numFrames; ++cc)
  {
    vtkPVKeyFrame* kf = this->Internals->KeyFrames[cc];
    vtkPVKeyFrame* prev = (cc > 0) ? this->Internals->KeyFrames[cc - 1] : NULL;
    vtkPVKeyFrame* next = (cc + 1 < numFrames) ? this->Internals->KeyFrames[cc + 1] : NULL;
    double min = (prev) ? prev->GetKeyTime() : 0.0;
    double max = (next) ? next->GetKeyTime() : 1.0;
    vtkSMProperty* keyTimeProp = kf->GetProperty("KeyTime");
    if (!keyTimeProp)
    {
      vtkWarningMacro("KeyFrameProxy should have a KeyTime property.");
      continue;
    }
    auto dr = keyTimeProp->FindDomain<vtkSMDoubleRangeDomain>();
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
#endif

//----------------------------------------------------------------------------
void vtkPVKeyFrameCueManipulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LastAddedKeyFrameIndex: " << this->LastAddedKeyFrameIndex << endl;
}
