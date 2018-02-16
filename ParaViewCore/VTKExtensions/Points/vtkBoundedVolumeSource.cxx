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
#include "vtkVector.h"
#include "vtkVectorOperators.h"

vtkStandardNewMacro(vtkBoundedVolumeSource);
//----------------------------------------------------------------------------
vtkBoundedVolumeSource::vtkBoundedVolumeSource()
{
  this->SetNumberOfInputPorts(0);
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
  this->Scale[0] = this->Scale[1] = this->Scale[2] = 1.0;
  this->Resolution[0] = this->Resolution[1] = this->Resolution[2] = 64;
  this->RefinementMode = USE_RESOLUTION;
  this->CellSize = 1.0;
}

//----------------------------------------------------------------------------
vtkBoundedVolumeSource::~vtkBoundedVolumeSource()
{
}

//----------------------------------------------------------------------------
namespace
{
void vtkGetVolume(vtkBoundedVolumeSource* self, vtkVector3d& origin, vtkVector3d& spacing,
  vtkVector<int, 6>& extents)
{
  origin = vtkVector3d(self->GetOrigin());

  if (self->GetRefinementMode() == vtkBoundedVolumeSource::USE_RESOLUTION)
  {
    const vtkVector3d scale(self->GetScale());
    const vtkVector3i resolution(self->GetResolution());

    spacing =
      vtkVector3d(scale[0] / resolution[0], scale[1] / resolution[1], scale[2] / resolution[2]);
    extents[0] = extents[2] = extents[4] = 0;
    extents[1] = resolution[0];
    extents[3] = resolution[1];
    extents[5] = resolution[2];
  }
  else
  {
    vtkVector3d lengths(self->GetScale());

    vtkVector3i resolution;
    resolution[0] = static_cast<int>(std::ceil(lengths[0] / self->GetCellSize()));
    resolution[1] = static_cast<int>(std::ceil(lengths[1] / self->GetCellSize()));
    resolution[2] = static_cast<int>(std::ceil(lengths[2] / self->GetCellSize()));
    assert(resolution[0] > 0 && resolution[1] > 0 && resolution[2] > 0);
    extents[0] = extents[2] = extents[4] = 0;
    extents[1] = resolution[0];
    extents[3] = resolution[1];
    extents[5] = resolution[2];

    spacing = vtkVector3d(self->GetCellSize());
  }
}
}

//----------------------------------------------------------------------------
int vtkBoundedVolumeSource::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkVector3d origin, spacing;
  vtkVector<int, 6> extents;
  vtkGetVolume(this, origin, spacing, extents);

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkDataObject::ORIGIN(), origin.GetData(), 3);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extents.GetData(), 6);
  outInfo->Set(vtkDataObject::SPACING(), spacing.GetData(), 3);
  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_FLOAT, 1);
  outInfo->Set(CAN_PRODUCE_SUB_EXTENT(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkBoundedVolumeSource::ExecuteDataWithInformation(
  vtkDataObject* vtkNotUsed(odata), vtkInformation* outInfo)
{
  vtkVector3d origin, spacing;
  vtkVector<int, 6> whole_extents;
  vtkGetVolume(this, origin, spacing, whole_extents);

  int* execExt = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());

  vtkImageData* data = vtkImageData::GetData(outInfo);
  data->SetSpacing(spacing.GetData());
  data->SetOrigin(origin.GetData());
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
  os << indent << "RefinementMode: " << this->RefinementMode << endl;
  os << indent << "Resolution: " << this->Resolution[0] << ", " << this->Resolution[1] << ", "
     << this->Resolution[2] << endl;
  os << indent << "CellSize: " << this->CellSize << endl;
}
