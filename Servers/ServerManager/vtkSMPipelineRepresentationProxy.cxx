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
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSourceProxy.h"

vtkCxxRevisionMacro(vtkSMPipelineRepresentationProxy, "1.1");
vtkCxxSetObjectMacro(vtkSMPipelineRepresentationProxy, InputProxy, vtkSMSourceProxy);
vtkCxxSetObjectMacro(vtkSMPipelineRepresentationProxy, Strategy, 
  vtkSMRepresentationStrategy);
//----------------------------------------------------------------------------
vtkSMPipelineRepresentationProxy::vtkSMPipelineRepresentationProxy()
{
  this->InputProxy = 0;
  this->Strategy = 0;
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
    vtkErrorMacro("CreateVTKObjects() must be called before AddToView.");
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
void vtkSMPipelineRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


