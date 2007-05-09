/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPipelineRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPipelineRepresentationProxy.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"

class vtkSMPipelineRepresentationProxyObserver : public vtkCommand
{
public:
  static vtkSMPipelineRepresentationProxyObserver* New()
    { return new vtkSMPipelineRepresentationProxyObserver; }

  void SetTarget(vtkSMPipelineRepresentationProxy* t)
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
  vtkSMPipelineRepresentationProxy* Target;
  vtkSMPipelineRepresentationProxyObserver()
    {
    this->Target = 0;
    }
};


vtkCxxRevisionMacro(vtkSMPipelineRepresentationProxy, "1.4");
vtkCxxSetObjectMacro(vtkSMPipelineRepresentationProxy, InputProxy, vtkSMSourceProxy);
//----------------------------------------------------------------------------
vtkSMPipelineRepresentationProxy::vtkSMPipelineRepresentationProxy()
{
  this->InputProxy = 0;
  this->Strategy = 0;

  this->UpdateTime = 0.0;
  this->UpdateTimeInitialized = false;

  this->UseViewTimeForUpdate = false;

  this->Observer = vtkSMPipelineRepresentationProxyObserver::New();
  this->Observer->SetTarget(this);

  this->ViewTimeLink = vtkSMPropertyLink::New();
}

//----------------------------------------------------------------------------
vtkSMPipelineRepresentationProxy::~vtkSMPipelineRepresentationProxy()
{
  this->ViewTimeLink->RemoveAllLinks();
  this->ViewTimeLink->Delete();

  this->SetInputProxy(0);
  this->SetStrategy(0);

  this->Observer->SetTarget(0);
  this->Observer->Delete();

}

//----------------------------------------------------------------------------
bool vtkSMPipelineRepresentationProxy::AddToView(vtkSMViewProxy* view)
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

  // Link with view time.
  if (vtkSMProperty* prop = view->GetProperty("ViewTime"))
    {
    this->ViewTimeLink->AddLinkedProperty(prop, vtkSMLink::INPUT);
    }
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMPipelineRepresentationProxy::BeginCreateVTKObjects(int numObjects)
{
  if (!this->Superclass::BeginCreateVTKObjects(numObjects))
    {
    return false;
    }

  // We can create the display proxy only if the input has been set.
  return (this->InputProxy != 0);
}

//----------------------------------------------------------------------------
bool vtkSMPipelineRepresentationProxy::EndCreateVTKObjects(int numObjects)
{
  if (vtkSMProperty* prop = this->GetProperty("UpdateTime"))
    {
    this->ViewTimeLink->AddLinkedProperty(prop, vtkSMLink::OUTPUT);
    }

  return this->Superclass::EndCreateVTKObjects(numObjects);
}

//----------------------------------------------------------------------------
void vtkSMPipelineRepresentationProxy::AddInput(vtkSMSourceProxy* input, 
  const char* vtkNotUsed(method), int vtkNotUsed(hasMultipleInputs))
{
  if (!input)
    {
    vtkErrorMacro("Representation cannot have NULL input.");
    return;
    }

  input->CreateParts();
  int numParts = input->GetNumberOfParts();

  if (numParts == 0)
    {
    vtkErrorMacro("Input has no output. Cannot create the representation.");
    return;
    }

  this->SetInputProxy(input);
  this->CreateVTKObjects(numParts);
}

//----------------------------------------------------------------------------
void vtkSMPipelineRepresentationProxy::Update(vtkSMViewProxy* view)
{
  if (this->Strategy)
    {
    this->Strategy->Update();
    }

  this->Superclass::Update(view);
}

//----------------------------------------------------------------------------
bool vtkSMPipelineRepresentationProxy::UpdateRequired()
{
  if (this->Strategy)
    {
    return this->Strategy->UpdateRequired();
    }

  return this->Superclass::UpdateRequired();
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMPipelineRepresentationProxy::GetDisplayedDataInformation()
{
  if (this->Strategy)
    {
    return this->Strategy->GetDisplayedDataInformation();
    }

  return this->Superclass::GetDisplayedDataInformation();
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMPipelineRepresentationProxy::GetFullResDataInformation()
{
  if (this->Strategy)
    {
    return this->Strategy->GetFullResDataInformation();
    }

  return this->Superclass::GetFullResDataInformation();
}

//----------------------------------------------------------------------------
void vtkSMPipelineRepresentationProxy::SetUseViewTimeForUpdate(bool val)
{
  if (val == this->UseViewTimeForUpdate)
    {
    return;
    }

  this->UseViewTimeForUpdate = val;
  this->ViewTimeLink->SetEnabled(val);
}

//----------------------------------------------------------------------------
void vtkSMPipelineRepresentationProxy::SetUpdateTime(double time)
{
  this->UpdateTimeInitialized = true;
  this->UpdateTime = time;

  if (this->Strategy)
    {
    vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->Strategy->GetProperty("UpdateTime"));
    if (dvp)
      {
      dvp->SetElement(0, time);
      this->Strategy->UpdateVTKObjects();
      }
    }

  this->MarkUpstreamModified();
}

//-----------------------------------------------------------------------------
void vtkSMPipelineRepresentationProxy::MarkUpstreamModified()
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
void vtkSMPipelineRepresentationProxy::SetStrategy(
  vtkSMRepresentationStrategy* strategy)
{
  if (this->Strategy)
    {
    this->Strategy->RemoveObserver(this->Observer);
    }

  vtkSetObjectBodyMacro(Strategy, vtkSMRepresentationStrategy, strategy);

  if (this->Strategy && this->UpdateTimeInitialized)
    {
    // Pass the update time to the strategy if it has been initialized.
    vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->Strategy->GetProperty("UpdateTime"));
    if (dvp)
      {
      dvp->SetElement(0, this->UpdateTime);
      this->Strategy->UpdateVTKObjects();
      }

    this->Strategy->AddObserver(vtkCommand::StartEvent, this->Observer);
    this->Strategy->AddObserver(vtkCommand::EndEvent, this->Observer);
    }
}

//----------------------------------------------------------------------------
void vtkSMPipelineRepresentationProxy::MarkModified(vtkSMProxy* modifiedProxy)
{
  // If some changes to the representation proxy invalidate the data
  // then we must expilictly call Strategy->MarkModified();
  if (modifiedProxy != this && this->Strategy)
    {
    this->Strategy->MarkModified(modifiedProxy); 
    }

  this->Superclass::MarkModified(modifiedProxy);
}

//----------------------------------------------------------------------------
void vtkSMPipelineRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


