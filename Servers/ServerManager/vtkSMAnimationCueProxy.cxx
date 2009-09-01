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
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMAnimationCueManipulatorProxy.h"
#include "vtkSMDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyProperty.h"

vtkCxxRevisionMacro(vtkSMAnimationCueProxy, "1.27.2.1");
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
  this->Enabled = true;
}

//----------------------------------------------------------------------------
vtkSMAnimationCueProxy::~vtkSMAnimationCueProxy()
{
  this->AnimationCue = 0;
  this->Observer->Delete();
  this->SetAnimatedProxy(0);
  this->SetManipulator(0);
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

  // Ensure that all vtk objects are created only on the client side.
  this->SetServers(vtkProcessModule::CLIENT);

  this->Superclass::CreateVTKObjects();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  this->AnimationCue = vtkAnimationCue::SafeDownCast(
    pm->GetObjectFromID(this->GetID()));
  this->InitializeObservers(this->AnimationCue);

  // If Manipulator subproxy is defined, the manipulator is set the xml
  // configuration.
  vtkSMAnimationCueManipulatorProxy* manip = 
    vtkSMAnimationCueManipulatorProxy::SafeDownCast(
      this->GetSubProxy("Manipulator"));
  if (manip)
    {
    this->SetManipulator(manip);
    }
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
    // Listen to the manipulator's ModifiedEvent. The manipilator fires this
    // event when the manipulator changes, its keyframes change or the values of
    // those key frames change. We simply propagate that event out so
    // applications can only listen to vtkSMAnimationCueProxy for modification
    // of the entire track.
    this->Manipulator->AddObserver(
      vtkCommand::ModifiedEvent,
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

  vtkSMAnimationCueManipulatorProxy* manip =
    vtkSMAnimationCueManipulatorProxy::SafeDownCast(obj);
  if (manip && event == vtkCommand::ModifiedEvent)
    {
    this->Modified();
    this->MarkConsumersAsDirty(this);
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
double vtkSMAnimationCueProxy::GetClockTime()
{
  return (this->AnimationCue)? this->AnimationCue->GetClockTime() : 0.0;
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
void vtkSMAnimationCueProxy::MarkConsumersAsDirty(vtkSMProxy* modifiedProxy)
{
  this->Superclass::MarkConsumersAsDirty(modifiedProxy);
  if (this->AnimatedProxy)
    {
    this->AnimatedProxy->MarkDirty(modifiedProxy);
    }
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
  os << indent << "Enabled: " << this->Enabled << endl;
}
