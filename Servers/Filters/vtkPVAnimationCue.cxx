/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAnimationCue.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVAnimationCue.h"

#include "vtkAnimationCue.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVCueManipulator.h"

//***************************************************************************
class vtkPVAnimationCueObserver : public vtkCommand
{
public:
  static vtkPVAnimationCueObserver* New()
    {return new vtkPVAnimationCueObserver;}

  vtkPVAnimationCueObserver()
    {
    this->AnimationCueProxy = 0;
    }

  void SetAnimationCueProxy(vtkPVAnimationCue* proxy)
    {
    this->AnimationCueProxy = proxy;
    }

  virtual void Execute(vtkObject* wdg, unsigned long event, void* calldata)
    {
    if (this->AnimationCueProxy)
      {
      this->AnimationCueProxy->ExecuteEvent(wdg, event, calldata);
      }
    }
  vtkPVAnimationCue* AnimationCueProxy;
};

//***************************************************************************

//----------------------------------------------------------------------------
vtkPVAnimationCue::vtkPVAnimationCue()
{
  this->AnimationCue = vtkAnimationCue::New();
  this->AnimatedElement= 0;
  this->Manipulator = 0;
  this->Enabled = true;

  vtkPVAnimationCueObserver* obs = vtkPVAnimationCueObserver::New();
  obs->SetAnimationCueProxy(this);
  this->Observer = obs;
  this->InitializeObservers(this->AnimationCue);
}

//----------------------------------------------------------------------------
vtkPVAnimationCue::~vtkPVAnimationCue()
{
  this->AnimationCue->Delete();
  this->Observer->Delete();
  this->SetManipulator(0);
}

//----------------------------------------------------------------------------
void vtkPVAnimationCue::SetManipulator(vtkPVCueManipulator* manipulator)
{
  if (manipulator == this->Manipulator)
    {
    return;
    }

  if (this->Manipulator)
    {
    this->Manipulator->RemoveObserver(this->Observer);
    }
  vtkSetObjectBodyMacro(Manipulator, vtkPVCueManipulator, manipulator);

  if (this->Manipulator)
    {
    // Listen to the manipulator's ModifiedEvent. The manipilator fires this
    // event when the manipulator changes, its keyframes change or the values of
    // those key frames change. We simply propagate that event out so
    // applications can only listen to vtkPVAnimationCue for modification
    // of the entire track.
    this->Manipulator->AddObserver(
      vtkCommand::ModifiedEvent, this->Observer);
    }
}

//----------------------------------------------------------------------------
void vtkPVAnimationCue::InitializeObservers(vtkAnimationCue* cue)
{
  if (cue)
    {
    cue->AddObserver(vtkCommand::StartAnimationCueEvent, this->Observer);
    cue->AddObserver(vtkCommand::EndAnimationCueEvent, this->Observer);
    cue->AddObserver(vtkCommand::AnimationCueTickEvent, this->Observer);
    }
}

//----------------------------------------------------------------------------
void vtkPVAnimationCue::ExecuteEvent(vtkObject* obj, unsigned long event,
  void* calldata)
{
  if (!this->Enabled)
    {
    // Ignore all animation events if the cue has been disabled.
    return;
    }

  vtkAnimationCue* cue = vtkAnimationCue::SafeDownCast(obj);
  if (cue)
    {
    switch (event)
      {
    case vtkCommand::StartAnimationCueEvent:
      this->StartCueInternal(calldata);
      break;

    case vtkCommand::EndAnimationCueEvent:
      this->EndCueInternal(calldata);
      break;

    case vtkCommand::AnimationCueTickEvent:
      this->TickInternal(calldata);
      break;
      }
    }

  vtkPVCueManipulator* manip =
    vtkPVCueManipulator::SafeDownCast(obj);
  if (manip && event == vtkCommand::ModifiedEvent)
    {
    this->Modified();
    //this->MarkConsumersAsDirty(this);
    }
}

//----------------------------------------------------------------------------
void vtkPVAnimationCue::StartCueInternal(
  void* info)
{
  if (this->Manipulator)
    {
    // let the manipulator know that the cue has been restarted.
    this->Manipulator->Initialize(this);
    }
  this->InvokeEvent(vtkCommand::StartAnimationCueEvent, info);
}

//----------------------------------------------------------------------------
void vtkPVAnimationCue::EndCueInternal(
  void* info)
{
  if (this->Manipulator)
    {
    // let the manipulator know that the cue has ended.
    this->Manipulator->Finalize(this);
    }
  this->InvokeEvent(vtkCommand::EndAnimationCueEvent, info);
}

//----------------------------------------------------------------------------
void vtkPVAnimationCue::TickInternal(
  void* info)
{
  // determine normalized  currenttime.
  vtkAnimationCue::AnimationCueInfo *cueInfo =
    reinterpret_cast<vtkAnimationCue::AnimationCueInfo*>(info);
  if (!cueInfo)
    {
    vtkErrorMacro("Invalid object thrown by Tick event");
    return;
    }

  double ctime = 0.0;
  if (cueInfo->StartTime != cueInfo->EndTime)
    {
    ctime = (cueInfo->AnimationTime - cueInfo->StartTime) /
      (cueInfo->EndTime - cueInfo->StartTime);
    }

  if (this->Manipulator)
    {
    this->Manipulator->UpdateValue(ctime, this);
    }
  this->InvokeEvent(vtkCommand::AnimationCueTickEvent, info);
}

//----------------------------------------------------------------------------
double vtkPVAnimationCue::GetAnimationTime()
{
  return (this->AnimationCue)? this->AnimationCue->GetAnimationTime() : 0.0;
}

//----------------------------------------------------------------------------
double vtkPVAnimationCue::GetDeltaTime()
{
  return (this->AnimationCue)? this->AnimationCue->GetDeltaTime() : 0.0;
}

//----------------------------------------------------------------------------
double vtkPVAnimationCue::GetClockTime()
{
  return (this->AnimationCue)? this->AnimationCue->GetClockTime() : 0.0;
}

//----------------------------------------------------------------------------
void vtkPVAnimationCue::SetTimeMode(int mode)
{
  this->AnimationCue->SetTimeMode(mode);
}

//----------------------------------------------------------------------------
void vtkPVAnimationCue::SetStartTime(double val)
{
  this->AnimationCue->SetStartTime(val);
}

//----------------------------------------------------------------------------
void vtkPVAnimationCue::SetEndTime(double val)
{
  this->AnimationCue->SetEndTime(val);
}

//----------------------------------------------------------------------------
void vtkPVAnimationCue::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AnimatedElement: " << this->AnimatedElement << endl;
  os << indent << "Enabled: " << this->Enabled << endl;
  os << indent << "Manipulator: " << this->Manipulator << endl;
}
