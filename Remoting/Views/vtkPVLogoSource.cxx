/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLogoSource.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVLogoSource.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTexture.h"

vtkStandardNewMacro(vtkPVLogoSource);
vtkCxxSetObjectMacro(vtkPVLogoSource, Texture, vtkTexture);

//----------------------------------------------------------------------------
vtkPVLogoSource::vtkPVLogoSource()
{
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
vtkPVLogoSource::~vtkPVLogoSource()
{
  this->SetTexture(nullptr);
}

//----------------------------------------------------------------------------
int vtkPVLogoSource::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (this->Texture)
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    this->Texture->Update();
    vtkImageData* image = this->Texture->GetInput();
    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), image->GetExtent(), 6);
    outInfo->Set(vtkDataObject::ORIGIN(), 0.0, 0.0, 0.0);
    outInfo->Set(vtkDataObject::SPACING(), 1, 1, 1);
  }
  return this->Superclass::RequestInformation(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void vtkPVLogoSource::ExecuteDataWithInformation(
  vtkDataObject* vtkNotUsed(data), vtkInformation* outInfo)
{
  if (this->Texture)
  {
    vtkImageData* output = this->GetOutput();
    this->AllocateOutputData(output, outInfo);
    this->Texture->Update();
    output->ShallowCopy(this->Texture->GetInput());
  }
}

//----------------------------------------------------------------------------
void vtkPVLogoSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkIndent nextIndent = indent.GetNextIndent();
  this->Superclass::PrintSelf(os, indent);
  if (this->Texture)
  {
    os << indent << "Texture: " << endl;
    this->Texture->PrintSelf(os, nextIndent);
  }
  else
  {
    os << indent << "Texture: (None)" << endl;
  }
}
