/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSelectionDeliveryRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSelectionDeliveryRepresentationProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMSelectionDeliveryRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMSelectionDeliveryRepresentationProxy::vtkSMSelectionDeliveryRepresentationProxy()
{
  this->SelectionRepresentation = 0;
}

//----------------------------------------------------------------------------
vtkSMSelectionDeliveryRepresentationProxy::~vtkSMSelectionDeliveryRepresentationProxy()
{
}

//-----------------------------------------------------------------------------
bool vtkSMSelectionDeliveryRepresentationProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }

  this->SelectionRepresentation =
    vtkSMClientDeliveryRepresentationProxy::SafeDownCast(
      this->GetSubProxy("SelectionRepresentation"));
  if (!this->SelectionRepresentation)
    {
    vtkErrorMacro("SelectionRepresentation must be defined in the xml configuration.");
    return false;
    }
  return true;
}

//-----------------------------------------------------------------------------
void vtkSMSelectionDeliveryRepresentationProxy::CreatePipeline(
  vtkSMSourceProxy* input, 
  int outputport)
{
  this->Superclass::CreatePipeline(input, outputport);

  // Connect the selection output from the input to the SelectionRepresentation.

  // Ensure that the source proxy has created extract selection filters.
  input->CreateSelectionProxies();

  vtkSMSourceProxy* esProxy = input->GetSelectionOutput(outputport);
  if (!esProxy)
    {
    vtkErrorMacro("Input proxy does not support selection extraction.");
    return;
    }

  // esProxy port 2 is the input vtkSelection. That's the one we are
  // interested in.
  this->Connect(esProxy, this->SelectionRepresentation, "Input", 2);
}

//----------------------------------------------------------------------------
void vtkSMSelectionDeliveryRepresentationProxy::Update(vtkSMViewProxy* view)
{
  this->Superclass::Update(view);
  this->SelectionRepresentation->Update(view);
}

//----------------------------------------------------------------------------
void vtkSMSelectionDeliveryRepresentationProxy::PrintSelf(
  ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SelectionRepresentation: " 
    << this->SelectionRepresentation << endl;
}


