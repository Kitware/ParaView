/*=========================================================================

   Program: ParaView
   Module:    vtkSMClientDeliverySource.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkSMClientDeliverySource.h"

#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSMClientDeliverySource);

// Construct programmable filter with empty execute method.
vtkSMClientDeliverySource::vtkSMClientDeliverySource() :
  DeliveryProxy(0)
{
  this->SetNumberOfInputPorts(0);
}

vtkSMClientDeliverySource::~vtkSMClientDeliverySource()
{
  this->SetDeliveryProxy(0);
}

int vtkSMClientDeliverySource::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Just update the delivery proxy and copy it to the output.
  this->DeliveryProxy->Update();
  vtkDataObject* input = this->DeliveryProxy->GetOutput();
  vtkDataObject* output = outputVector->GetInformationObject(0)->
    Get(vtkDataObject::DATA_OBJECT());
  output->ShallowCopy(input);
  return 1;
}

int vtkSMClientDeliverySource::RequestDataObject(
  vtkInformation*,
  vtkInformationVector**,
  vtkInformationVector* outputVector)
{
  if (!this->DeliveryProxy)
    {
    vtkErrorMacro("Delivery proxy must be set before update.");
    }

  // Make the algorithm's output type match the proxy's output.
  this->DeliveryProxy->Update();
  vtkDataObject* input = this->DeliveryProxy->GetOutput();
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());
  if (!output || output->GetDataObjectType() != input->GetDataObjectType())
    {
    output = input->NewInstance();
    output->SetPipelineInformation(info);
    output->Delete();
    this->GetOutputPortInformation(0)->Set(
      vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
    }

  return 1;
}

void vtkSMClientDeliverySource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

