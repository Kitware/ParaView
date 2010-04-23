/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDataRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDataRepresentationProxy.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMRepresentationStrategyVector.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"

class vtkSMDataRepresentationProxyObserver : public vtkCommand
{
public:
  static vtkSMDataRepresentationProxyObserver* New()
    { return new vtkSMDataRepresentationProxyObserver; }

  void SetTarget(vtkSMDataRepresentationProxy* t)
    {
    this->Target = t;
    }

  virtual void Execute(vtkObject *vtkNotUsed(caller), unsigned long eventId,
                       void *callData)
    {
    if (this->Target)
      {
      this->Target->InvokeEvent(eventId, callData);
      }
    }

protected:
  vtkSMDataRepresentationProxy* Target;
  vtkSMDataRepresentationProxyObserver()
    {
    this->Target = 0;
    }
};


vtkCxxSetObjectMacro(vtkSMDataRepresentationProxy, InputProxy, vtkSMSourceProxy);
//----------------------------------------------------------------------------
vtkSMDataRepresentationProxy::vtkSMDataRepresentationProxy()
{
  this->InputProxy = 0;
  this->RepresentationStrategies = new vtkSMRepresentationStrategyVector();

  this->UpdateTime = 0.0;
  this->UpdateTimeInitialized = false;

  this->UseViewUpdateTime = true;

  this->Observer = vtkSMDataRepresentationProxyObserver::New();
  this->Observer->SetTarget(this);

  this->OutputPort = 0;
}

//----------------------------------------------------------------------------
vtkSMDataRepresentationProxy::~vtkSMDataRepresentationProxy()
{
  this->SetInputProxy(0);

  delete this->RepresentationStrategies;
  this->RepresentationStrategies = 0;

  this->Observer->SetTarget(0);
  this->Observer->Delete();
}

//----------------------------------------------------------------------------
vtkCommand* vtkSMDataRepresentationProxy::GetObserver()
{
  return this->Observer;
}

//----------------------------------------------------------------------------
void vtkSMDataRepresentationProxy::GetActiveStrategies(
  vtkSMRepresentationStrategyVector& activeStrategies)
{
  vtkSMRepresentationStrategyVector::iterator iter;

  if (this->GetVisibility())
    {
    for (iter = this->RepresentationStrategies->begin(); 
      iter != this->RepresentationStrategies->end(); ++iter)
      {
      activeStrategies.push_back(iter->GetPointer());
      }
    }
}

//----------------------------------------------------------------------------
bool vtkSMDataRepresentationProxy::AddToView(vtkSMViewProxy* view)
{
  // Since adding representation to a view setups the view pipeline,
  // it is essential that the view proxy has been created.
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("CreateVTKObjects() must be called before AddToView."
      << "This typically implies that the input to the "
      << "representation was not set before adding it to the view.");
    return false;
    }

  // If representation needs compositing strategy from the view,
  // give it a chance to set it up.
  if (!this->InitializeStrategy(view))
    {
    return false;
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMDataRepresentationProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }

  // We can create the display proxy only if the input has been set.
  return (this->InputProxy != 0);
}

//----------------------------------------------------------------------------
bool vtkSMDataRepresentationProxy::EndCreateVTKObjects()
{
  return this->Superclass::EndCreateVTKObjects();
}

//----------------------------------------------------------------------------
 void vtkSMDataRepresentationProxy::AddInput(unsigned int,
                                             vtkSMSourceProxy* input,
                                             unsigned int outputPort,
                                             const char*)
{
  if (!input)
    {
    vtkErrorMacro("Representation cannot have NULL input.");
    return;
    }

  input->CreateOutputPorts();
  int numParts = input->GetNumberOfOutputPorts();

  if (numParts == 0)
    {
    vtkErrorMacro("Input has no output. Cannot create the representation.");
    return;
    }

  this->SetInputProxy(input);
  this->OutputPort = outputPort;
  this->CreateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMDataRepresentationProxy::Update(vtkSMViewProxy* view)
{
  vtkSMRepresentationStrategyVector activeStrategies;
  this->GetActiveStrategies(activeStrategies);

  vtkSMRepresentationStrategyVector::iterator iter;
  for (iter = activeStrategies.begin(); iter != activeStrategies.end(); ++iter)
    {
    iter->GetPointer()->Update();
    }

  this->Superclass::Update(view);
}

//----------------------------------------------------------------------------
bool vtkSMDataRepresentationProxy::UpdateRequired()
{
  bool update_required = false;

  vtkSMRepresentationStrategyVector activeStrategies;
  this->GetActiveStrategies(activeStrategies);

  vtkSMRepresentationStrategyVector::iterator iter;
  for (iter = activeStrategies.begin(); 
    !update_required && iter != activeStrategies.end(); ++iter)
    {
    update_required |= iter->GetPointer()->UpdateRequired();
    }

  return (update_required? true: this->Superclass::UpdateRequired());
}

//----------------------------------------------------------------------------
void vtkSMDataRepresentationProxy::SetUseViewUpdateTime(bool val)
{
  if (val == this->UseViewUpdateTime)
    {
    return;
    }

  this->UseViewUpdateTime = val;

  if (this->ViewUpdateTimeInitialized)
    {
    this->SetUpdateTimeInternal(this->ViewUpdateTime);
    }
}

//----------------------------------------------------------------------------
void vtkSMDataRepresentationProxy::SetViewUpdateTime(double time)
{
  this->Superclass::SetViewUpdateTime(time);
  if (this->UseViewUpdateTime)
    {
    this->SetUpdateTimeInternal(this->ViewUpdateTime);
    }
}

//----------------------------------------------------------------------------
void vtkSMDataRepresentationProxy::SetUpdateTime(double time)
{
  this->UpdateTimeInitialized = true;
  this->UpdateTime = time;
  if (!this->UseViewUpdateTime)
    {
    this->SetUpdateTimeInternal(time);
    }
}

//----------------------------------------------------------------------------
void vtkSMDataRepresentationProxy::SetUpdateTimeInternal(double time)
{
  vtkSMRepresentationStrategyVector::iterator iter;
  for (iter = this->RepresentationStrategies->begin(); 
    iter != this->RepresentationStrategies->end(); ++iter)
    {
    vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      iter->GetPointer()->GetProperty("UpdateTime"));
    if (dvp)
      {
      dvp->SetElement(0, time);
      iter->GetPointer()->UpdateProperty("UpdateTime");
      }
    }

  this->MarkUpstreamModified();
}

//-----------------------------------------------------------------------------
void vtkSMDataRepresentationProxy::MarkUpstreamModified()
{
  vtkSMProxy* current = this;
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    current->GetProperty("Input"));
  while (current && pp && pp->GetNumberOfProxies() > 0)
    {
    current = pp->GetProxy(0);
    pp = vtkSMProxyProperty::SafeDownCast(current->GetProperty("Input"));
    }

  if (current)
    {
    current->MarkModified(current);
    }
}

//----------------------------------------------------------------------------
void vtkSMDataRepresentationProxy::AddStrategy(
  vtkSMRepresentationStrategy* strategy)
{
  this->RepresentationStrategies->push_back(strategy);

  strategy->AddObserver(vtkCommand::StartEvent, this->Observer);
  strategy->AddObserver(vtkCommand::EndEvent, this->Observer);

  if (this->UpdateTimeInitialized)
    {
    // This will propagate the update time to the newly added strategy.
    this->SetUpdateTime(this->UpdateTime);
    }
  if (this->ViewUpdateTimeInitialized)
    {
    // This will propagate the update time to the newly added strategy.
    this->SetViewUpdateTime(this->ViewUpdateTime);
    }
}

//----------------------------------------------------------------------------
void vtkSMDataRepresentationProxy::MarkDirty(vtkSMProxy* modifiedProxy)
{
  // If some changes to the representation proxy invalidate the data
  // then we must expilictly call Strategy->MarkDirty();
  if (modifiedProxy != this)
    {
    vtkSMRepresentationStrategyVector::iterator iter;
    for (iter = this->RepresentationStrategies->begin(); 
      iter != this->RepresentationStrategies->end(); ++iter)
      {
      iter->GetPointer()->MarkDirty(modifiedProxy);
      }
    }

  this->Superclass::MarkDirty(modifiedProxy);
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMDataRepresentationProxy::GetRepresentedDataInformation(
  bool update/*=true*/)
{
  if (!this->GetInputProxy())
    {
    vtkErrorMacro("Input not set, cannot gather information.");
    return 0;
    }

  // Don't call update, UpdateDataInformation() will update the sub-pipeline
  // until the place from which the data information is obtained if needed.
  //if (update)
  //  {
  //  this->Update();
  //  }

  // We don't use active stratgies since active strategies are returned only
  // when visibile.
  vtkSMRepresentationStrategyVector::iterator iter;
  for (iter = this->RepresentationStrategies->begin(); 
    iter != this->RepresentationStrategies->end(); ++iter)
    {
    if (update)
      {
      iter->GetPointer()->UpdateDataInformation();
      }
    return iter->GetPointer()->GetRepresentedDataInformation();
    }

  return 0;
}

//----------------------------------------------------------------------------
unsigned long vtkSMDataRepresentationProxy::GetFullResMemorySize()
{
  unsigned long size = 0;

  vtkSMRepresentationStrategyVector activeStrategies;
  this->GetActiveStrategies(activeStrategies);

  vtkSMRepresentationStrategyVector::iterator iter;
  for (iter = activeStrategies.begin(); iter != activeStrategies.end(); ++iter)
    {
    // update part of pipeline to obtain correct data size information.
    iter->GetPointer()->UpdateDataInformation();
    size += iter->GetPointer()->GetFullResMemorySize();
    }

  return size;
}

//----------------------------------------------------------------------------
unsigned long vtkSMDataRepresentationProxy::GetDisplayedMemorySize()
{
  unsigned long size = 0;

  vtkSMRepresentationStrategyVector activeStrategies;
  this->GetActiveStrategies(activeStrategies);

  vtkSMRepresentationStrategyVector::iterator iter;
  for (iter = activeStrategies.begin(); iter != activeStrategies.end(); ++iter)
    {
    // update part of pipeline to obtain correct data size information.
    iter->GetPointer()->UpdateDataInformation();
    size += iter->GetPointer()->GetDisplayedMemorySize();
    }

  return size;
}

//----------------------------------------------------------------------------
void vtkSMDataRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseViewUpdateTime: " << this->UseViewUpdateTime
    << endl;
}

