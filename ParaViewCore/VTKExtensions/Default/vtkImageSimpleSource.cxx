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
void vtkImageSimpleSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Whole Extent: (" << this->WholeExtent[0] << "," << this->WholeExtent[1] << ", "
     << this->WholeExtent[2] << ", " << this->WholeExtent[3] << ", " << this->WholeExtent[4] << ", "
     << this->WholeExtent[5] << ")\n";
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
void vtkImageSimpleSource::PrepareImageData(vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector, vtkImageData*** vtkNotUsed(inDataObjects),
  vtkImageData** outDataObjects)
{
  std::cout << "PrepareImageData()" << std::endl;
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkImageData* outData = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Get image size for allocating point data arrays
  int extent[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent);
  outData->SetExtent(extent);
  if (outDataObjects)
  {
    outDataObjects[0] = outData;
  }

  vtkIdType dims[3];
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;
  vtkIdType imageSize = dims[0] * dims[1] * dims[2];

  // Allocate each of the data arrays
  vtkPointData* pointData = outData->GetPointData();
  vtkInformationVector* pointDataVector = outInfo->Get(vtkDataObject::POINT_DATA_VECTOR());
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
void vtkImageSimpleSource::ThreadedRequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector,
  vtkImageData*** vtkNotUsed(inData), vtkImageData** outData, int extent[6], int threadId)
{
  // std::cout << threadId << "-ThreadedReqData" << std::endl;
  vtkImageData* imageData = outData[0];

  // Get data arrays
  vtkPointData* pointData = imageData->GetPointData();
  vtkDataArray* xArray = pointData->GetArray(SIMPLE_FIELD_X.c_str());
  vtkDataArray* distArray = pointData->GetArray(SIMPLE_FIELD_DISTANCESQUARED.c_str());
  vtkDataArray* swirlArray = pointData->GetArray(SIMPLE_FIELD_SWIRL.c_str());

  // Compute continuous increments for this extent
  int dims[3];
  imageData->GetDimensions(dims);

  vtkIdType inc[3];
  inc[0] = 1;
  inc[1] = dims[0];
  inc[2] = dims[0] * dims[1];
  vtkIdType outIncY = inc[1] - (extent[1] - extent[0] + 1) * inc[0];
  vtkIdType outIncZ = inc[2] - (extent[3] - extent[2] + 1) * inc[1];

  // Compute first index for this extent
  int structuredBegin[3];
  structuredBegin[0] = extent[0] - this->WholeExtent[0];
  structuredBegin[1] = extent[2] - this->WholeExtent[2];
  structuredBegin[2] = extent[4] - this->WholeExtent[4];
  vtkIdType begin = structuredBegin[0] + inc[1] * structuredBegin[1] + inc[2] * structuredBegin[2];

  // Get starting pointer for each array
  double* xPtr = static_cast<double*>(xArray->GetVoidPointer(0)) + begin;
  double* distPtr = static_cast<double*>(distArray->GetVoidPointer(0)) + begin;
  double* swirlPtr = static_cast<double*>(swirlArray->GetVoidPointer(0)) + 3 * begin;
  // double* swirlPtrStart = static_cast<double*>(swirlArray->GetVoidPointer(0));

  // Loop over voxels
  // unsigned long target = static_cast<unsigned long>((dims[2])*(dims[1])/50.0) + 1;
  unsigned long count = 0;
  for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
  {
    for (int idxY = extent[2]; !this->AbortExecute && idxY <= extent[3]; idxY++)
    {
      for (int idxX = extent[0]; idxX <= extent[1]; idxX++)
      {
        // std::cout << "ijk: " << idxX << ", " << idxY << ", " << idxZ
        //           << " -- dswirl " << (swirlPtr - swirlPtrStart) << std::endl;
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
      xPtr += outIncY;
      distPtr += outIncY;
      swirlPtr += 3 * outIncY;
    } // for (idxY)
    xPtr += outIncZ;
    distPtr += outIncZ;
    swirlPtr += 3 * outIncZ;
  } // for (idxZ)
}
