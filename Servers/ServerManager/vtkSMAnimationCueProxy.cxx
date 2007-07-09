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
#include "vtkSMProxyProperty.h"
#include "vtkSMAnimationCueManipulatorProxy.h"
#include "vtkSMDomainIterator.h"
#include "vtkClientServerID.h"

vtkCxxRevisionMacro(vtkSMAnimationCueProxy, "1.22");
vtkStandardNewMacro(vtkSMAnimationCueProxy);

vtkCxxSetObjectMacro(vtkSMAnimationCueProxy, AnimatedProxy, vtkSMProxy);
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
  vtkSMAnimationCueProxyObserver* obs = vtkSMAnimationCueProxyObserver::New();
  obs->SetAnimationCueProxy(this);
  this->Observer = obs; 

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
void vtkSMAnimationCueProxy::RemoveAnimatedProxy()
{
  this->SetAnimatedProxy(0);
}

//----------------------------------------------------------------------------
void vtkSMAnimationCueProxy::CreateVTKObjects()
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
  this->Superclass::CreateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMAnimationCueProxy::SetManipulator(
  vtkSMAnimationCueManipulatorProxy* manipulator)
{
  if (manipulator == this->Manipulator)
    {
    return;
    }
  if (this->Manipulator)
    {
    this->Manipulator->RemoveObserver(this->Observer);
    this->Manipulator->UnRegister(this);
    this->Manipulator = 0;
    }
  this->Manipulator = manipulator;
  if (this->Manipulator)
    {
    this->Manipulator->AddObserver(
      vtkSMAnimationCueManipulatorProxy::StateModifiedEvent,
      this->Observer);
    this->Manipulator->Register(this);
    }
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
}

//----------------------------------------------------------------------------
void vtkSMAnimationCueProxy::StartCueInternal(
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
void vtkSMAnimationCueProxy::EndCueInternal(
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
double vtkSMAnimationCueProxy::GetAnimationTime()
{
  return (this->AnimationCue)? this->AnimationCue->GetAnimationTime() : 0.0;
}

//----------------------------------------------------------------------------
double vtkSMAnimationCueProxy::GetDeltaTime()
{
  return (this->AnimationCue)? this->AnimationCue->GetDeltaTime() : 0.0;
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
void vtkSMAnimationCueProxy::CloneCopy(vtkSMAnimationCueProxy* src)
{
  if (!src || src == this)
    {
    return;
    }

  // Copy all properties except proxyproperties.
  this->Copy(src, "vtkSMProxyProperty", 
    vtkSMProxy::COPY_PROXY_PROPERTY_VALUES_BY_REFERENCE);
  
  vtkSMProxyProperty* source = vtkSMProxyProperty::SafeDownCast(
    src->GetProperty("AnimatedProxy"));
  vtkSMProxyProperty* dest = vtkSMProxyProperty::SafeDownCast(
    this->GetProperty("AnimatedProxy"));
  
  if (source && dest)
    {
    dest->Copy(source);//we ShallowCopy AnimatedProxy.
    }

  source = vtkSMProxyProperty::SafeDownCast(src->GetProperty("Manipulator"));
  dest = vtkSMProxyProperty::SafeDownCast(this->GetProperty("Manipulator"));

  if (source && dest)
    {
    dest->DeepCopy(source, 0, 
      vtkSMProxy::COPY_PROXY_PROPERTY_VALUES_BY_CLONING);
    }
  this->MarkAllPropertiesAsModified();
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
