/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAlgorithmPortsInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVAlgorithmPortsInformation.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerStream.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkObjectFactory.h"
#include "vtkSource.h"
#include "vtkInformation.h"

vtkStandardNewMacro(vtkPVAlgorithmPortsInformation);

//----------------------------------------------------------------------------
vtkPVAlgorithmPortsInformation::vtkPVAlgorithmPortsInformation()
{
  this->RootOnly = 1;
  this->NumberOfOutputs = 0;
  this->NumberOfRequiredInputs = 0;
}

//----------------------------------------------------------------------------
vtkPVAlgorithmPortsInformation::~vtkPVAlgorithmPortsInformation()
{
}

//----------------------------------------------------------------------------
void vtkPVAlgorithmPortsInformation::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfOutputs: " << this->NumberOfOutputs << "\n";
  os << indent << "NumberOfRequiredInputs: " 
    << this->NumberOfRequiredInputs << "\n";
}

//----------------------------------------------------------------------------
void vtkPVAlgorithmPortsInformation::CopyFromObject(vtkObject* obj)
{
  this->NumberOfOutputs = 0;
  this->NumberOfRequiredInputs = 0;

  vtkAlgorithm* algorithm = vtkAlgorithm::SafeDownCast(obj);
  if(!algorithm)
    {
    vtkErrorMacro("Could not downcast vtkAlgorithm.");
    return;
    }
  vtkDemandDrivenPipeline* pipeline = 
    vtkDemandDrivenPipeline::SafeDownCast(algorithm->GetExecutive());
  if (pipeline)
    {
    //pipeline->UpdateDataObject();
    }
  vtkSource* source = vtkSource::SafeDownCast(obj);
  if (source)
    {
    this->NumberOfOutputs = source->GetNumberOfOutputs();
    }
  else
    {
    this->NumberOfOutputs = algorithm->GetNumberOfOutputPorts();
    }

  int numInputs = algorithm->GetNumberOfInputPorts();
  for (int cc=0; cc < numInputs; cc++)
    {
    vtkInformation* info = algorithm->GetInputPortInformation(cc);
    if (info && !info->Has(vtkAlgorithm::INPUT_IS_OPTIONAL()))
      {
      this->NumberOfRequiredInputs++;
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVAlgorithmPortsInformation::AddInformation(vtkPVInformation* info)
{
  if (vtkPVAlgorithmPortsInformation::SafeDownCast(info))
    {
    this->NumberOfOutputs = vtkPVAlgorithmPortsInformation::SafeDownCast(info)
      ->GetNumberOfOutputs();
    this->NumberOfRequiredInputs = vtkPVAlgorithmPortsInformation::SafeDownCast(info)
      ->GetNumberOfRequiredInputs();
    }
}

//----------------------------------------------------------------------------
void
vtkPVAlgorithmPortsInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply 
       << this->NumberOfOutputs
       << this->NumberOfRequiredInputs
       << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void
vtkPVAlgorithmPortsInformation
::CopyFromStream(const vtkClientServerStream* css)
{
  css->GetArgument(0, 0, &this->NumberOfOutputs);
  css->GetArgument(0, 1, &this->NumberOfRequiredInputs);
}
