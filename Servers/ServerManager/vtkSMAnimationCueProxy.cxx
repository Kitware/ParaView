/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAnimationCueProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMAnimationCueProxy.h"

#include "vtkAnimationCue.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"
#include "vtkSMDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMAnimationCueManipulatorProxy.h"
#include "vtkSMDomainIterator.h"
#include "vtkClientServerID.h"

vtkCxxRevisionMacro(vtkSMAnimationCueProxy, "1.2");
vtkStandardNewMacro(vtkSMAnimationCueProxy);

vtkCxxSetObjectMacro(vtkSMAnimationCueProxy, AnimatedProxy, vtkSMProxy);
vtkCxxSetObjectMacro(vtkSMAnimationCueProxy, Manipulator,
  vtkSMAnimationCueManipulatorProxy);
//----------------------------------------------------------------------------

//***************************************************************************
class vtkSMAnimationCueProxyObserver : public vtkCommand
{
public:
  static vtkSMAnimationCueProxyObserver* New()
    {return new vtkSMAnimationCueProxyObserver;}

  vtkSMAnimationCueProxyObserver()
    {
    this->AnimationCueProxy = 0;
    }

  void SetAnimationCueProxy(vtkSMAnimationCueProxy* proxy)
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
  vtkSMAnimationCueProxy* AnimationCueProxy;
};

//***************************************************************************

//----------------------------------------------------------------------------
vtkSMAnimationCueProxy::vtkSMAnimationCueProxy()
{
  this->Observer = vtkSMAnimationCueProxyObserver::New();
  this->Observer->SetAnimationCueProxy(this);

  this->AnimatedProxy = 0;
  this->AnimatedElement= 0;
  this->AnimatedPropertyName = 0;
  this->AnimatedDomainName = 0;

  this->Manipulator = 0;

  this->AnimationCue = 0;
}

//----------------------------------------------------------------------------
vtkSMAnimationCueProxy::~vtkSMAnimationCueProxy()
{
  this->Observer->Delete();
  this->SetAnimatedProxy(0);
  this->SetManipulator(0);
  if (this->AnimationCue)
    {
    this->AnimationCue->Delete();
    }
  this->SetAnimatedPropertyName(0);
  this->SetAnimatedDomainName(0);
}

//----------------------------------------------------------------------------
void vtkSMAnimationCueProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->AnimationCue = vtkAnimationCue::New();
  this->InitializeObservers(this->AnimationCue);
  this->ObjectsCreated = 1; //since we don't have any serverside-vtkobjects
    //we set the objects created flag so that vtkSMProxy does not
    //attempt to create objects.
  this->Superclass::CreateVTKObjects(numObjects);
}

//----------------------------------------------------------------------------
void vtkSMAnimationCueProxy::InitializeObservers(vtkAnimationCue* cue)
{
  if (cue)
    {
    cue->AddObserver(vtkCommand::StartAnimationCueEvent, this->Observer);
    cue->AddObserver(vtkCommand::EndAnimationCueEvent, this->Observer);
    cue->AddObserver(vtkCommand::AnimationCueTickEvent, this->Observer);
    }
}

//----------------------------------------------------------------------------
vtkSMProperty* vtkSMAnimationCueProxy::GetAnimatedProperty()
{
  if (!this->AnimatedPropertyName || !this->AnimatedProxy)
    {
    return NULL;
    }
  return this->AnimatedProxy->GetProperty(this->AnimatedPropertyName);
}

//----------------------------------------------------------------------------
vtkSMDomain* vtkSMAnimationCueProxy::GetAnimatedDomain()
{
  vtkSMProperty* property = this->GetAnimatedProperty();
  if (!property)
    {
    return NULL;
    }
  vtkSMDomain* domain = NULL;
  vtkSMDomainIterator* iter = property->NewDomainIterator();
  iter->Begin();
  if (!iter->IsAtEnd())
    {
    domain = iter->GetDomain();
    }
  iter->Delete();
  return domain;
}

//----------------------------------------------------------------------------
void vtkSMAnimationCueProxy::ExecuteEvent(vtkObject* obj, unsigned long event,
  void* calldata)
{
  vtkAnimationCue* cue = vtkAnimationCue::SafeDownCast(obj);
  if (!cue)
    {
    return;
    }
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

//----------------------------------------------------------------------------
void vtkSMAnimationCueProxy::StartCueInternal(
  void* info)
{
  if (this->Manipulator)
    {
    // let the manipulator know that the cue has been restarted.
    this->Manipulator->Initialize();
    }
  this->InvokeEvent(vtkCommand::StartAnimationCueEvent, info);
}

//----------------------------------------------------------------------------
void vtkSMAnimationCueProxy::EndCueInternal(
  void* info)
{
  this->InvokeEvent(vtkCommand::EndAnimationCueEvent, info);
}

//----------------------------------------------------------------------------
void vtkSMAnimationCueProxy::TickInternal(
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
    ctime = (cueInfo->CurrentTime - cueInfo->StartTime) /
      (cueInfo->EndTime - cueInfo->StartTime);
    }

  if (this->Manipulator)
    {
    this->Manipulator->UpdateValue(ctime, this);
    }
  this->InvokeEvent(vtkCommand::AnimationCueTickEvent, info);
}

//----------------------------------------------------------------------------
void vtkSMAnimationCueProxy::SaveInBatchScript(ofstream* file)
{
  *file << endl;
  vtkClientServerID id = this->SelfID;
  *file << "set pvTemp" << id
    << " [$proxyManager NewProxy animation "
    << this->GetXMLName() <<"]" << endl;
  *file << "  $proxyManager RegisterProxy animation pvTemp" << id
    << " $pvTemp" << id << endl;
  *file << "  $pvTemp" << id << " UnRegister {}" << endl;


  *file << " [$pvTemp" << id << " GetProperty TimeMode]"
    << " SetElements1 " << this->AnimationCue->GetTimeMode() << endl;

  *file << "  [$pvTemp" << id << " GetProperty StartTime]"
    << " SetElements1 " << this->AnimationCue->GetStartTime() << endl;

  *file << "  [$pvTemp" << id << " GetProperty EndTime]"
    << " SetElements1 " << this->AnimationCue->GetEndTime() << endl;

  // Set Animated proxy details.
  // NOTE: For this to work, it is required that the the AnimatedProxy
  // has been already saved in the batch script. We can ensure that by dumping
  // the animation batch out at the end of the batch script.
  if (this->AnimatedProxy && this->AnimatedProxy->GetNumberOfIDs() > 0)
    {
    *file << "  [$pvTemp" << id << " GetProperty AnimatedProxy]"
      << " RemoveAllProxies" << endl;
    *file << "  [$pvTemp" << id << " GetProperty AnimatedProxy]"
      << " AddProxy $pvTemp" << this->AnimatedProxy->GetID(0)
      << endl;
    }
 
  if (this->AnimatedPropertyName)
    {
    *file << "  [$pvTemp" << id << " GetProperty AnimatedPropertyName]"
      << " SetElement 0 " << this->AnimatedPropertyName << endl;
    }

  if (this->AnimatedDomainName)
    {
    *file << "  [$pvTemp" << id << " GetProperty AnimatedDomainName]"
      << " SetElement 0 " << this->AnimatedDomainName << endl;
    }

  *file << "  [$pvTemp" << id << " GetProperty AnimatedElement]"
    << " SetElements1 " << this->AnimatedElement << endl;

  // Save Manipulator in batch script.
  if (this->Manipulator)
    {
    this->Manipulator->SaveInBatchScript(file);
    *file << "  $pvTemp" << id << " SetManipulator $pvTemp"
      << this->Manipulator->GetID() << endl;
    }

  
  *file << "  $pvTemp" << id << " UpdateVTKObjects" << endl;
  *file << endl;
}

//----------------------------------------------------------------------------
void vtkSMAnimationCueProxy::SetTimeMode(int mode)
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Not created yet.")
    return ;
    }
  this->AnimationCue->SetTimeMode(mode);
}

//----------------------------------------------------------------------------
void vtkSMAnimationCueProxy::SetStartTime(double time)
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Not created yet.")
    return ;
    }
  this->AnimationCue->SetStartTime(time);
}

//----------------------------------------------------------------------------
void vtkSMAnimationCueProxy::SetEndTime(double time)
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Not created yet.")
    return ;
    }
  this->AnimationCue->SetEndTime(time);
}

//----------------------------------------------------------------------------
double vtkSMAnimationCueProxy::GetEndTime()
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Not created yet.")
    return 0;
    }
  return this->AnimationCue->GetEndTime();
}

//----------------------------------------------------------------------------
double vtkSMAnimationCueProxy::GetStartTime()
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Not created yet.")
    return 0;
    }
  return this->AnimationCue->GetStartTime();
}

//----------------------------------------------------------------------------
void vtkSMAnimationCueProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AnimatedProxy: " << this->AnimatedProxy << endl;
  os << indent << "AnimatedElement: " << this->AnimatedElement << endl;
  os << indent << "AnimatedPropertyName: " << 
    ((this->AnimatedPropertyName)? this->AnimatedPropertyName : "NULL") << endl;
  os << indent << "AnimatedDomainName: " <<
    ((this->AnimatedDomainName)? this->AnimatedDomainName : "NULL") << endl;
  os << indent << "AnimationCue: " << this->AnimationCue << endl;
  os << indent << "Manipulator: " << this->Manipulator << endl;
}
