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

#include "vtkObjectFactory.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSourceProxy.h"

vtkCxxRevisionMacro(vtkSMPipelineRepresentationProxy, "1.2");
vtkCxxSetObjectMacro(vtkSMPipelineRepresentationProxy, InputProxy, vtkSMSourceProxy);
//----------------------------------------------------------------------------
vtkSMPipelineRepresentationProxy::vtkSMPipelineRepresentationProxy()
{
  this->InputProxy = 0;
  this->Strategy = 0;

  this->UpdateTime = 0.0;
  this->UpdateTimeInitialized = false;
}

//----------------------------------------------------------------------------
vtkSMPipelineRepresentationProxy::~vtkSMPipelineRepresentationProxy()
{
  this->SetInputProxy(0);
  this->SetStrategy(0);
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
  return this->InitializeStrategy(view);
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
    }
}

//----------------------------------------------------------------------------
void vtkSMPipelineRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


