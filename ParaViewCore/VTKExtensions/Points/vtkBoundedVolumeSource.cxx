/*=========================================================================

  Program:   ParaView
  Module:    vtkBoundedVolumeSource.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBoundedVolumeSource.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkBoundedVolumeSource);
//----------------------------------------------------------------------------
vtkBoundedVolumeSource::vtkBoundedVolumeSource()
{
  this->SetNumberOfInputPorts(0);
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
  this->Scale[0] = this->Scale[1] = this->Scale[2] = 1.0;
  this->Resolution[0] = this->Resolution[1] = this->Resolution[2] = 64;
}

//----------------------------------------------------------------------------
vtkBoundedVolumeSource::~vtkBoundedVolumeSource()
{
}

//----------------------------------------------------------------------------
int vtkBoundedVolumeSource::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkDataObject::ORIGIN(), this->Origin, 3);

  int ext[6] = { 0, this->Resolution[0], 0, this->Resolution[1], 0, this->Resolution[2] };
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext, 6);

  double spacing[3] = { this->Scale[0] / this->Resolution[0], this->Scale[1] / this->Resolution[1],
    this->Scale[2] / this->Resolution[2] };
  outInfo->Set(vtkDataObject::SPACING(), spacing, 3);
  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_FLOAT, 1);
  outInfo->Set(CAN_PRODUCE_SUB_EXTENT(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkBoundedVolumeSource::ExecuteDataWithInformation(
  vtkDataObject* vtkNotUsed(odata), vtkInformation* outInfo)
{
  vtkImageData* data = vtkImageData::GetData(outInfo);
  int* execExt = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());

  double spacing[3] = { this->Scale[0] / this->Resolution[0], this->Scale[1] / this->Resolution[1],
    this->Scale[2] / this->Resolution[2] };
  data->SetSpacing(spacing);
  data->SetOrigin(this->Origin);
  data->SetExtent(execExt);
}

//----------------------------------------------------------------------------
void vtkBoundedVolumeSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Origin: " << this->Origin[0] << ", " << this->Origin[1] << ", "
     << this->Origin[2] << endl;
  os << indent << "Scale: " << this->Scale[0] << ", " << this->Scale[1] << ", " << this->Scale[2]
     << endl;
  os << indent << "Resolution: " << this->Resolution[0] << ", " << this->Resolution[1] << ", "
     << this->Resolution[2] << endl;
}
