/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSimpleSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageSimpleSource.h"

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <array>
#include <cmath>
#include <string>

namespace
{
const std::string SIMPLE_FIELD_X = "X";
const std::string SIMPLE_FIELD_DISTANCESQUARED = "DistanceSquared";
const std::string SIMPLE_FIELD_SWIRL = "Swirl";
}

vtkStandardNewMacro(vtkImageSimpleSource);

//----------------------------------------------------------------------------
vtkImageSimpleSource::vtkImageSimpleSource()
{
  this->WholeExtent[0] = -10;
  this->WholeExtent[1] = 10;
  this->WholeExtent[2] = -10;
  this->WholeExtent[3] = 10;
  this->WholeExtent[4] = -10;
  this->WholeExtent[5] = 10;

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
void vtkImageSimpleSource::SetWholeExtent(
  int xMin, int xMax, int yMin, int yMax, int zMin, int zMax)
{
  int modified = 0;

  if (this->WholeExtent[0] != xMin)
  {
    modified = 1;
    this->WholeExtent[0] = xMin;
  }
  if (this->WholeExtent[1] != xMax)
  {
    modified = 1;
    this->WholeExtent[1] = xMax;
  }
  if (this->WholeExtent[2] != yMin)
  {
    modified = 1;
    this->WholeExtent[2] = yMin;
  }
  if (this->WholeExtent[3] != yMax)
  {
    modified = 1;
    this->WholeExtent[3] = yMax;
  }
  if (this->WholeExtent[4] != zMin)
  {
    modified = 1;
    this->WholeExtent[4] = zMin;
  }
  if (this->WholeExtent[5] != zMax)
  {
    modified = 1;
    this->WholeExtent[5] = zMax;
  }
  if (modified)
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
namespace
{
class ValueFunctor
{
public:
  ValueFunctor()
    : Image(nullptr)
  {
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    if (!this->Image)
    {
      return;
    }

    int extent[6];
    this->Image->GetExtent(extent);
    vtkIdType outIncX = 1;
    vtkIdType outIncY = extent[1] - extent[0] + 1;
    vtkIdType outIncZ = outIncY * (extent[3] - extent[2] + 1);

    // Get first index
    vtkIdType firstIndex[3];
    firstIndex[2] = begin / outIncZ + extent[4];
    firstIndex[1] = begin / outIncZ % outIncY + extent[2];
    firstIndex[0] = begin % outIncY + extent[0];
    // std::cout << "firstIndex: " << firstIndex[0] << ", " << firstIndex[1] << ", " <<
    // firstIndex[2] << std::endl;

    // Get second index
    vtkIdType secondIndex[3];
    secondIndex[2] = end / outIncZ + extent[4];
    secondIndex[1] = end / outIncZ % outIncY + extent[2];
    secondIndex[0] = end % outIncY + extent[0];
    // std::cout << "secondIndex: " << secondIndex[0] << ", " << secondIndex[1] << ", " <<
    // secondIndex[2] << std::endl;

    // Build point data by traversing the image coordinates
    vtkPointData* pointData = this->Image->GetPointData();
    vtkDataArray* xArray = pointData->GetArray(SIMPLE_FIELD_X.c_str());
    vtkDataArray* distArray = pointData->GetArray(SIMPLE_FIELD_DISTANCESQUARED.c_str());
    vtkDataArray* swirlArray = pointData->GetArray(SIMPLE_FIELD_SWIRL.c_str());

    // Get starting pointer to each array
    double* xPtr = static_cast<double*>(xArray->GetVoidPointer(begin));
    double* distPtr = static_cast<double*>(distArray->GetVoidPointer(begin));
    double* swirlPtr = static_cast<double*>(swirlArray->GetVoidPointer(begin));

    // Loop over voxels
    // unsigned long target = static_cast<unsigned long>((dims[2])*(dims[1])/50.0) + 1;
    unsigned long count = 0;
    for (int idxZ = firstIndex[2]; idxZ <= secondIndex[2]; idxZ++)
    {
      for (int idxY = firstIndex[1]; idxY <= secondIndex[1]; idxY++)
      {
        for (int idxX = firstIndex[0]; idxX <= secondIndex[0]; idxX++)
        {
          // std::cout << "ijk: " << idxX << ", " << idxY << ", " << idxZ << std::endl;
          *xPtr = static_cast<double>(idxX);
          xPtr++;

          *distPtr = static_cast<double>(idxX * idxX + idxY * idxY + idxZ * idxZ);
          distPtr++;

          *swirlPtr = idxY;
          swirlPtr++;
          *swirlPtr = idxZ;
          swirlPtr++;
          *swirlPtr = idxX;
          swirlPtr++;
        } // for (idxX)
      }   // for (idxY)
    }     // for (idxZ)
  }

  vtkImageData* Image;
};
} // end anonymous namespace

//----------------------------------------------------------------------------
int vtkImageSimpleSource::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataObject* dataObj = outInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkImageData* output = vtkImageData::SafeDownCast(dataObj);

  int* extent = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  output->SetExtent(extent);

  // Get image size for allocating point data arrays
  vtkIdType dims[3];
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;
  vtkIdType imageSize = dims[0] * dims[1] * dims[2];

  // Update each of the data arrays
  vtkInformationVector* pointDataVector = outInfo->Get(vtkDataObject::POINT_DATA_VECTOR());
  vtkPointData* pointData = output->GetPointData();
  for (int i = 0; i < pointDataVector->GetNumberOfInformationObjects(); ++i)
  {
    vtkInformation* info = pointDataVector->GetInformationObject(i);
    std::string name = info->Get(vtkDataObject::FIELD_NAME());
    int numComponents = info->Get(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS());

    int index = -1;
    vtkDataArray* array = pointData->GetArray(name.c_str(), index);
    if (array)
    {
      array->SetNumberOfComponents(numComponents);
      array->SetNumberOfTuples(imageSize);
      array->Modified();
      continue;
    }

    // (else) allocate and add array
    int dataType = info->Get(vtkDataObject::FIELD_ATTRIBUTE_TYPE());
    array = vtkDataArray::CreateDataArray(dataType);
    array->SetNumberOfComponents(numComponents);
    array->SetNumberOfTuples(imageSize);
    array->SetName(name.c_str());

    pointData->AddArray(array);
    array->Delete();
  } // for(i)

  // Make X the active scalars, Swirl the active vectors
  pointData->SetActiveScalars(SIMPLE_FIELD_X.c_str());
  pointData->SetActiveVectors(SIMPLE_FIELD_SWIRL.c_str());

  // Test the functor conversion from linear index to extents here
  ValueFunctor functor;
  functor.Image = output;
  vtkSMPTools::For(0, imageSize - 1, functor);
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageSimpleSource::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // Set the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->Set(vtkDataObject::SPACING(), 1.0, 1.0, 1.0);
  outInfo->Set(vtkDataObject::ORIGIN(), 0.0, 0.0, 0.0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->WholeExtent, 6);

  // Create info objects for the 3 point data arrays
  vtkInformationVector* pointDataInfo = vtkInformationVector::New();

  // Point data "X" (active)
  vtkInformation* xInfo = vtkInformation::New();
  xInfo->Set(vtkDataObject::FIELD_ACTIVE_ATTRIBUTE(), VTK_DOUBLE);
  xInfo->Set(vtkDataObject::FIELD_NAME(), SIMPLE_FIELD_X);
  xInfo->Set(vtkDataObject::FIELD_ATTRIBUTE_TYPE(), VTK_DOUBLE);
  xInfo->Set(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS(), 1);
  xInfo->Set(vtkDataObject::FIELD_ASSOCIATION(), vtkDataObject::FIELD_ASSOCIATION_POINTS);
  pointDataInfo->Append(xInfo);
  xInfo->FastDelete();

  // Point data "DistanceSquared"
  vtkInformation* dsInfo = vtkInformation::New();
  dsInfo->Set(vtkDataObject::FIELD_ASSOCIATION(), vtkDataObject::FIELD_ASSOCIATION_POINTS);
  dsInfo->Set(vtkDataObject::FIELD_NAME(), SIMPLE_FIELD_DISTANCESQUARED);
  dsInfo->Set(vtkDataObject::FIELD_ATTRIBUTE_TYPE(), VTK_DOUBLE);
  dsInfo->Set(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS(), 1);
  pointDataInfo->Append(dsInfo);
  dsInfo->FastDelete();

  // Point data "Swirl" (vector)
  vtkInformation* swInfo = vtkInformation::New();
  swInfo->Set(vtkDataObject::FIELD_ASSOCIATION(), vtkDataObject::FIELD_ASSOCIATION_POINTS);
  swInfo->Set(vtkDataObject::FIELD_NAME(), SIMPLE_FIELD_SWIRL);
  swInfo->Set(vtkDataObject::FIELD_ATTRIBUTE_TYPE(), VTK_DOUBLE);
  swInfo->Set(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS(), 3);
  pointDataInfo->Append(swInfo);
  swInfo->FastDelete();

  outInfo->Set(vtkDataObject::POINT_DATA_VECTOR(), pointDataInfo);
  pointDataInfo->FastDelete();

  // Also support sub-extents
  outInfo->Set(CAN_PRODUCE_SUB_EXTENT(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkImageSimpleSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
