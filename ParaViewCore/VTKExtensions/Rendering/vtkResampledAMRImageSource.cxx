/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkResampledAMRImageSource.h"

#include "vtkAMRBox.h"
#include "vtkAMRInformation.h"
#include "vtkBoundingBox.h"
#include "vtkCellData.h"
#include "vtkIdList.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkPVStreamingMacros.h"
#include "vtkPointData.h"
#include "vtkUniformGrid.h"
#include "vtkUniformGridAMRDataIterator.h"
#include "vtkVoxel.h"

#include <algorithm>
#include <assert.h>

namespace
{
static inline vtkIdType FindCell(vtkImageData* image, double point[3])
{
  double pcoords[3];
  int subid = 0;
  return image->vtkImageData::FindCell(point, NULL, -1, 0.1, subid, pcoords, NULL);
}
}

vtkStandardNewMacro(vtkResampledAMRImageSource);
//----------------------------------------------------------------------------
vtkResampledAMRImageSource::vtkResampledAMRImageSource()
{
  this->MaxDimensions[0] = this->MaxDimensions[1] = this->MaxDimensions[2] = 32;
  vtkMath::UninitializeBounds(this->SpatialBounds);
}

//----------------------------------------------------------------------------
vtkResampledAMRImageSource::~vtkResampledAMRImageSource()
{
}

//----------------------------------------------------------------------------
void vtkResampledAMRImageSource::Reset()
{
  vtkMath::UninitializeBounds(this->SpatialBounds);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkResampledAMRImageSource::UpdateResampledVolume(vtkOverlappingAMR* amr)
{
  assert(amr);
  if (this->NeedsInitialization())
  {
    if (!this->Initialize(amr))
    {
      return;
    }
  }

  // Now, fill in values from datasets in the amr.

  bool something_changed = false;
  const vtkAMRInformation* amrInfo = amr->GetAMRInfo();
  vtkSmartPointer<vtkUniformGridAMRDataIterator> iter;
  iter.TakeReference(vtkUniformGridAMRDataIterator::SafeDownCast(amr->NewIterator()));
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    // note: this iteration "naturally" goes from datasets at lower levels to
    // those at higher levels.
    vtkImageData* data = vtkImageData::SafeDownCast(iter->GetCurrentDataObject());
    assert(data != NULL);

    unsigned int level = iter->GetCurrentLevel();
    unsigned int index = iter->GetCurrentIndex();

    const vtkAMRBox& box = amrInfo->GetAMRBox(level, index);
    bool val = this->UpdateResampledVolume(level, index, box, data);
    something_changed |= val;
  }

  if (something_changed)
  {
    // mark data modified, otherwise mappers are confused.
    this->Output->Modified();

    // mark arrays modified too.
    for (int cc = 0; cc < this->ResampledAMR->GetCellData()->GetNumberOfArrays(); cc++)
    {
      this->ResampledAMR->GetCellData()->GetAbstractArray(cc)->Modified();
    }
    for (int cc = 0;
         this->ResampledAMRPointData && cc < this->ResampledAMRPointData->GetNumberOfArrays(); cc++)
    {
      this->ResampledAMRPointData->GetAbstractArray(cc)->Modified();
    }
  }
}

//----------------------------------------------------------------------------
bool vtkResampledAMRImageSource::Initialize(vtkOverlappingAMR* amr)
{
  if (amr->GetNumberOfLevels() < 1 || amr->GetNumberOfDataSets(0) < 1)
  {
    // this is an empty AMR. Nothing to do here.
    return false;
  }

  vtkNew<vtkImageData> output;

  double bounds[6];
  amr->GetBounds(bounds);

  vtkStreamingStatusMacro("Data bounds: " << bounds[0] << ", " << bounds[1] << ", " << bounds[2]
                                          << ", " << bounds[3] << ", " << bounds[4] << ", "
                                          << bounds[5]);

  vtkBoundingBox amrBBox(bounds);
  if (vtkMath::AreBoundsInitialized(this->SpatialBounds))
  {
    vtkStreamingStatusMacro(
      "Spatial bounds: " << this->SpatialBounds[0] << ", " << this->SpatialBounds[1] << ", "
                         << this->SpatialBounds[2] << ", " << this->SpatialBounds[3] << ", "
                         << this->SpatialBounds[4] << ", " << this->SpatialBounds[5]);

    if (amrBBox.IntersectBox(this->SpatialBounds))
    {
      amrBBox.GetBounds(bounds);
    }
  }

  vtkStreamingStatusMacro("Image bounds: " << bounds[0] << ", " << bounds[1] << ", " << bounds[2]
                                           << ", " << bounds[3] << ", " << bounds[4] << ", "
                                           << bounds[5]);

  //---------------------------------------------------------------------------
  // determine optimal dimensions for the image to cover the spatial bounds
  // specified. We want it to be clamped to the value user specified, but not
  // more than the refinement available in the data itself.

  // get the spacing at the maximum level. That will help us compute the maximum
  // refinement we can get from the data.
  assert(amr->GetAMRInfo()->HasSpacing(amr->GetNumberOfLevels() - 1));
  double data_spacing[3];
  amr->GetAMRInfo()->GetSpacing(amr->GetNumberOfLevels() - 1, data_spacing);

  // now for a box covering the bounds, with this data-spacing, we can get the
  // following resolution.
  int data_dimensions[3];
  data_dimensions[0] = static_cast<int>((bounds[1] - bounds[0]) / data_spacing[0]);
  data_dimensions[1] = static_cast<int>((bounds[3] - bounds[2]) / data_spacing[1]);
  data_dimensions[2] = static_cast<int>((bounds[5] - bounds[4]) / data_spacing[2]);

  int dimensions[3];
  dimensions[0] = std::min(data_dimensions[0], this->MaxDimensions[0]);
  dimensions[1] = std::min(data_dimensions[1], this->MaxDimensions[1]);
  dimensions[2] = std::min(data_dimensions[2], this->MaxDimensions[2]);

  vtkStreamingStatusMacro("resampled image resolution: " << dimensions[0] << ", " << dimensions[1]
                                                         << ", " << dimensions[2]);

  //---------------------------------------------------------------------------
  assert(dimensions[0] >= 2 && dimensions[1] >= 2 && dimensions[2] >= 2);

  double spacing[3] = { (bounds[1] - bounds[0]) / dimensions[0],
    (bounds[3] - bounds[2]) / dimensions[1], (bounds[5] - bounds[4]) / dimensions[2] };

  double origin[3];
  origin[0] = bounds[0];
  origin[1] = bounds[2];
  origin[2] = bounds[4];

  // convert cell-dims to point-dims
  dimensions[0] += 1;
  dimensions[1] += 1;
  dimensions[2] += 1;

  //---------------------------------------------------------------------------
  output->SetDimensions(dimensions);
  output->SetOrigin(origin);
  output->SetSpacing(spacing);
  vtkStreamingStatusMacro("Volume Origin: " << origin[0] << ", " << origin[1] << ", " << origin[2]);
  vtkStreamingStatusMacro(
    "Volume Spacing: " << spacing[0] << ", " << spacing[1] << ", " << spacing[2]);
  vtkStreamingStatusMacro(
    "Volume Dimensions: " << dimensions[0] << ", " << dimensions[1] << ", " << dimensions[2]);

  // locate first non-null uniform grid in the AMR. That's the one we use to
  // model the field arrays.
  vtkImageData* reference = NULL;
  for (unsigned int level = 0; reference == NULL && level < amr->GetNumberOfLevels(); level++)
  {
    for (unsigned int index = 0; reference == NULL && index < amr->GetNumberOfDataSets(level);
         index++)
    {
      reference = amr->GetDataSet(level, index);
    }
  }
  if (!reference)
  {
    // cannot initialize.
    vtkStreamingStatusMacro("Insufficient data. AMR is empty.");
    return false;
  }

  vtkIdType numCells = output->GetNumberOfCells();

  // Add point arrays in the output that correspond to the cell arrays in the
  // input.
  output->GetCellData()->CopyAllocate(reference->GetCellData(), numCells);

  if (reference->GetPointData()->GetNumberOfArrays() > 0)
  {
    // If reference grid has point data, we are going to pass that through as
    // well. However we cannot put those in the output->CellData since that will
    // confuse all CopyData() calls. So we just keep it separate and pass it to
    // the dualGrid directly.
    this->ResampledAMRPointData = vtkSmartPointer<vtkPointData>::New();
    this->ResampledAMRPointData->InterpolateAllocate(reference->GetPointData(), numCells);
  }
  else
  {
    this->ResampledAMRPointData = NULL;
  }

  // Generate a mask array that's used to keep track of which point comes from
  // which level. We use this array to avoid overwriting data from  higher
  // levels with that from lower levels.
  vtkNew<vtkIntArray> levelArray;
  levelArray->SetName("DonorLevel");
  levelArray->SetNumberOfComponents(1);
  levelArray->SetNumberOfTuples(numCells);
  for (vtkIdType cc = 0; cc < numCells; cc++)
  {
    *levelArray->GetPointer(cc) = -1;
  }
  this->DonorLevel = levelArray.GetPointer();
  this->ResampledAMR = output.GetPointer();

  // the output of this filter is the dual grid on the resample AMR since.
  vtkNew<vtkImageData> dualGrid;
  dualGrid->SetDimensions(
    output->GetDimensions()[0] - 1, output->GetDimensions()[1] - 1, output->GetDimensions()[2] - 1);
  dualGrid->SetOrigin(output->GetOrigin()[0] + output->GetSpacing()[0] / 2.0,
    output->GetOrigin()[1] + output->GetSpacing()[1] / 2.0,
    output->GetOrigin()[2] + output->GetSpacing()[2] / 2.0);
  dualGrid->SetSpacing(output->GetSpacing());
  dualGrid->GetPointData()->PassData(output->GetCellData());

  // Pass arrays from this->ResampledAMRPointData to  the dualGrid as well.
  for (int cc = 0;
       this->ResampledAMRPointData != NULL && cc < this->ResampledAMRPointData->GetNumberOfArrays();
       cc++)
  {
    vtkAbstractArray* curArray = this->ResampledAMRPointData->GetAbstractArray(cc);

    if (curArray && curArray->GetName() &&
      dualGrid->GetPointData()->GetAbstractArray(curArray->GetName()) == NULL)
    {
      dualGrid->GetPointData()->AddArray(curArray);
    }
  }

  this->SetOutput(dualGrid.GetPointer());
  this->InitializationTime.Modified();

  vtkStreamingStatusMacro("Resample volume has been initialized.");
  vtkStreamingStatusMacro("    number of cells :" << numCells);
  vtkStreamingStatusMacro(
    "    number of cell arrays  :" << output->GetCellData()->GetNumberOfArrays());
  vtkStreamingStatusMacro("    number of point arrays  :" << (this->ResampledAMRPointData
                              ? this->ResampledAMRPointData->GetNumberOfArrays()
                              : 0)) return true;
}

//----------------------------------------------------------------------------
bool vtkResampledAMRImageSource::UpdateResampledVolume(
  const unsigned int& level, const unsigned& index, const vtkAMRBox&, vtkImageData* donor)
{
  (void)index;
  vtkStreamingStatusMacro("Updating with block at " << level << "," << index);

  vtkBoundingBox donorBounds(donor->GetBounds());
  vtkBoundingBox receiverBounds(this->ResampledAMR->GetBounds());

  vtkBoundingBox updateBounds(receiverBounds);
  if (!updateBounds.IntersectBox(donorBounds))
  {
    // this block is skipped since it doesn't intersect our region on interest.
    return false;
  }

  bool something_changed = false;
  double receiver_spacing[3];
  this->ResampledAMR->GetSpacing(receiver_spacing);

  vtkNew<vtkIdList> cellPoints;

  for (double z = updateBounds.GetMinPoint()[2]; z <= updateBounds.GetMaxPoint()[2];
       z += receiver_spacing[2])
  {
    for (double y = updateBounds.GetMinPoint()[1]; y <= updateBounds.GetMaxPoint()[1];
         y += receiver_spacing[1])
    {
      for (double x = updateBounds.GetMinPoint()[0]; x < updateBounds.GetMaxPoint()[0];
           x += receiver_spacing[0])
      {
        double sample_point[3] = { x, y, z };
        vtkIdType receiver_cellid = FindCell(this->ResampledAMR, sample_point);
        if (receiver_cellid == -1 || receiver_cellid >= this->DonorLevel->GetNumberOfTuples() ||
          this->DonorLevel->GetValue(receiver_cellid) > static_cast<int>(level))
        {
          continue;
        }
        vtkIdType donor_cellId = FindCell(donor, sample_point);
        if (donor_cellId == -1)
        {
          // the sample_point is not in the donor grid. This cannot happen since
          // we have intersected the two boxes, but just in case.
          continue;
        }
        this->ResampledAMR->GetCellData()->CopyData(
          donor->GetCellData(), donor_cellId, receiver_cellid);
        if (this->ResampledAMRPointData)
        {
          donor->GetCellPoints(donor_cellId, cellPoints.GetPointer());
          vtkIdType numPoints = cellPoints->GetNumberOfIds();
          assert(numPoints <= 8);
          if (numPoints > 0)
          {
            double weight, weights[8];
            weight = 1.0 / numPoints;
            for (vtkIdType cc = 0; cc < numPoints; cc++)
            {
              weights[cc] = weight;
            }

            this->ResampledAMRPointData->InterpolatePoint(
              donor->GetPointData(), receiver_cellid, cellPoints.GetPointer(), weights);
          }
        }
        something_changed = true;
        this->DonorLevel->SetValue(receiver_cellid, static_cast<int>(level));
      }
    }
  }
  return something_changed;
}

//----------------------------------------------------------------------------
void vtkResampledAMRImageSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
