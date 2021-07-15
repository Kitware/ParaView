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

#include "vtkBoundingBox.h"
#include "vtkCompositeDataSet.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkVectorOperators.h"

vtkStandardNewMacro(vtkBoundedVolumeSource);
vtkCxxSetObjectMacro(vtkBoundedVolumeSource, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkBoundedVolumeSource::vtkBoundedVolumeSource()
{
  this->SetNumberOfInputPorts(1);
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
  this->Scale[0] = this->Scale[1] = this->Scale[2] = 1.0;
  this->Resolution[0] = this->Resolution[1] = this->Resolution[2] = 64;
  this->RefinementMode = USE_RESOLUTION;
  this->CellSize = 1.0;
  this->Padding = 0;
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->UseOriginScale = false;
}

//----------------------------------------------------------------------------
vtkBoundedVolumeSource::~vtkBoundedVolumeSource()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
int vtkBoundedVolumeSource::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkBoundedVolumeSource::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkBoundingBox bbox(this->Origin[0], this->Origin[0] + this->Scale[0], this->Origin[1],
    this->Origin[1] + this->Scale[1], this->Origin[2], this->Origin[2] + this->Scale[2]);
  bbox.Inflate(this->Padding);

  vtkNew<vtkImageData> tmp;
  if (this->RefinementMode == USE_RESOLUTION)
  {
    if (!vtkBoundedVolumeSource::SetImageParameters(tmp, bbox, vtkVector3i(this->Resolution)))
    {
      return 0;
    }
  }
  else
  {
    if (!vtkBoundedVolumeSource::SetImageParameters(tmp, bbox, this->CellSize))
    {
      return 0;
    }
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkDataObject::ORIGIN(), tmp->GetOrigin(), 3);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), tmp->GetExtent(), 6);
  outInfo->Set(vtkDataObject::SPACING(), tmp->GetSpacing(), 3);
  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_FLOAT, 1);
  outInfo->Set(CAN_PRODUCE_SUB_EXTENT(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkBoundedVolumeSource::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkBoundingBox bbox;
  if (this->UseOriginScale)
  {
    bbox.SetBounds(this->Origin[0], this->Origin[0] + this->Scale[0], this->Origin[1],
      this->Origin[1] + this->Scale[1], this->Origin[2], this->Origin[2] + this->Scale[2]);
  }
  else if (this->UseInputBounds)
  {
    double bds[6];
    auto inputDO = vtkDataObject::GetData(inputVector[0], 0);
    if (auto ds = vtkDataSet::SafeDownCast(inputDO))
    {
      ds->GetBounds(bds);
      bbox.SetBounds(bds);
    }
    else if (auto cd = vtkCompositeDataSet::SafeDownCast(inputDO))
    {
      cd->GetBounds(bds);
      bbox.SetBounds(bds);
    }
    if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
    {
      vtkBoundingBox result;
      this->Controller->AllReduce(bbox, result);
      bbox = result;
    }
  }
  else
  {
    bbox.SetBounds(this->BoundingBox);
  }

  bbox.Inflate(this->Padding);

  vtkImageData* data = vtkImageData::GetData(outputVector, 0);
  if (this->RefinementMode == USE_RESOLUTION)
  {
    if (!vtkBoundedVolumeSource::SetImageParameters(data, bbox, vtkVector3i(this->Resolution)))
    {
      vtkErrorMacro("Failed to determine image parameters.");
      return 0;
    }
  }
  else
  {
    if (!vtkBoundedVolumeSource::SetImageParameters(data, bbox, this->CellSize))
    {
      vtkErrorMacro("Failed to determine image parameters.");
      return 0;
    }
  }

  // limit extent to update extent.
  auto outInfo = outputVector->GetInformationObject(0);
  int* updateExt = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  data->SetExtent(updateExt);
  return 1;
}

//----------------------------------------------------------------------------
bool vtkBoundedVolumeSource::SetImageParameters(
  vtkImageData* image, const vtkBoundingBox& bbox, const vtkVector3i& resolution)
{
  if (image == nullptr || !bbox.IsValid() || resolution[0] < 1 || resolution[1] < 1 ||
    resolution[2] < 1)
  {
    return false;
  }

  image->SetExtent(0, resolution[0], 0, resolution[1], 0, resolution[2]);

  vtkVector3d lengths;
  bbox.GetLengths(lengths.GetData());

  vtkVector3d origin;
  bbox.GetMinPoint(origin[0], origin[1], origin[2]);
  image->SetOrigin(origin.GetData());

  const vtkVector3d resolutionD(static_cast<double>(resolution[0]),
    static_cast<double>(resolution[1]), static_cast<double>(resolution[2]));
  vtkVector3d spacing = lengths / resolutionD;
  image->SetSpacing(spacing.GetData());
  return true;
}

//----------------------------------------------------------------------------
bool vtkBoundedVolumeSource::SetImageParameters(
  vtkImageData* image, const vtkBoundingBox& bbox, const double cellSize)
{
  if (image == nullptr || !bbox.IsValid() || cellSize <= 0)
  {
    return false;
  }

  vtkVector3d lengths;
  bbox.GetLengths(lengths.GetData());

  vtkVector3i resolution;
  resolution[0] = static_cast<int>(std::ceil(lengths[0] / cellSize));
  resolution[1] = static_cast<int>(std::ceil(lengths[1] / cellSize));
  resolution[2] = static_cast<int>(std::ceil(lengths[2] / cellSize));
  assert(resolution[0] > 0 && resolution[1] > 0 && resolution[2] > 0);

  image->SetExtent(0, resolution[0], 0, resolution[1], 0, resolution[2]);

  // since old bounds may not exactly match, we compute new bounds keeping the
  // center same.
  vtkVector3d center;
  bbox.GetCenter(center.GetData());

  lengths[0] = resolution[0] * cellSize;
  lengths[1] = resolution[1] * cellSize;
  lengths[2] = resolution[2] * cellSize;
  vtkVector3d origin = center - (lengths / vtkVector3d(2.0));
  image->SetOrigin(origin.GetData());
  image->SetSpacing(cellSize, cellSize, cellSize);
  return true;
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
  os << indent << "UseOriginScale: " << this->UseOriginScale << endl;
  os << indent << "UseInputBounds: " << this->UseInputBounds << endl;
  os << indent << "BoundingBox: " << this->BoundingBox[0] << this->BoundingBox[1]
     << this->BoundingBox[2] << this->BoundingBox[3] << this->BoundingBox[4] << this->BoundingBox[5]
     << endl;
}
