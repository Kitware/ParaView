/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShearedCubeSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkShearedCubeSource.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkPVInformationKeys.h"

#include <vtkNew.h>

vtkStandardNewMacro(vtkShearedCubeSource);

//----------------------------------------------------------------------------
vtkShearedCubeSource::vtkShearedCubeSource()
{
  this->EnableCustomBase       = 0;
  this->EnableCustomTitle      = 0;
  this->EnableTimeLabel        = 0;
  this->EnableCustomLabelRange = 0;

  for(int i=0; i < 3; i++)
    {
    this->BaseU[i] = this->BaseV[i] = this->BaseW[i] = 0;
    this->OrientedBoundingBox[i*2] = -0.5;
    this->OrientedBoundingBox[i*2 + 1] = +0.5;
    }
  this->BaseU[0] = this->BaseV[1] = this->BaseW[2] = 1.0;
  this->AxisUTitle = this->AxisVTitle = this->AxisWTitle = NULL;
  this->TimeLabel = NULL;

  this->LabelRangeU[0] = this->LabelRangeV[0] = this->LabelRangeW[0] = 0.0;
  this->LabelRangeU[1] = this->LabelRangeV[1] = this->LabelRangeW[1] = 1.0;

  this->LinearTransformU[1] = LinearTransformV[1] = LinearTransformW[1] = 1.0;
  this->LinearTransformU[0] = LinearTransformV[0] = LinearTransformW[0] = 0.0;
}

//----------------------------------------------------------------------------
vtkShearedCubeSource::~vtkShearedCubeSource()
{
  this->SetAxisUTitle(NULL);
  this->SetAxisVTitle(NULL);
  this->SetAxisWTitle(NULL);
  this->SetTimeLabel(NULL);
}

//----------------------------------------------------------------------------
void vtkShearedCubeSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
//----------------------------------------------------------------------------
int vtkShearedCubeSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkMath::Normalize(this->BaseU);
  vtkMath::Normalize(this->BaseV);
  vtkMath::Normalize(this->BaseW);

  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  double x[3], n[3];
  int numPolys=6, numPts=8;
  int i, j, k, coord;
  vtkIdType pts[4];
  vtkPoints *newPoints;
  vtkFloatArray *newNormals;
  vtkCellArray *newPolys;

  //
  // Set things up; allocate memory
  //
  newPoints = vtkPoints::New();
  newPoints->Allocate(numPts);
  newNormals = vtkFloatArray::New();
  newNormals->SetNumberOfComponents(3);
  newNormals->Allocate(numPolys);
  newNormals->SetName("Normals");

  newPolys = vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(numPolys,4));

  //
  // Generate points and normals
  //

  // Do planes normal to U
  for(k = 0; k < 2; k++)
    {
    for(j = 0; j < 2; j++)
      {
     for(i = 0; i < 2; i++)
        {
        for(coord = 0; coord < 3; coord++)
          {
          x[coord] = this->BaseU[coord]*this->OrientedBoundingBox[i] +
                     this->BaseV[coord]*this->OrientedBoundingBox[2+j] +
                     this->BaseW[coord]*this->OrientedBoundingBox[4+k];
          }
        newPoints->InsertNextPoint(x);
        }
      }
    }

  pts[0] = 0; pts[1] = 1; pts[2] = 3; pts[3] = 2;
  n[0] = -this->BaseW[0]; n[1] = -this->BaseW[1]; n[2] = -this->BaseW[2];
  newPolys->InsertNextCell(4,pts);
  newNormals->InsertNextTuple(n);

  pts[0] = 4; pts[1] = 5; pts[2] = 7; pts[3] = 6;
  n[0] = this->BaseW[0]; n[1] = this->BaseW[1]; n[2] = this->BaseW[2];
  newPolys->InsertNextCell(4,pts);
  newNormals->InsertNextTuple(n);

  pts[0] = 0; pts[1] = 4; pts[2] = 6; pts[3] = 2;
  n[0] = -this->BaseU[0]; n[1] = -this->BaseU[1]; n[2] = -this->BaseU[2];
  newPolys->InsertNextCell(4,pts);
  newNormals->InsertNextTuple(n);

  pts[0] = 1; pts[1] = 3; pts[2] = 7; pts[3] = 5;
  n[0] = this->BaseU[0]; n[1] = this->BaseU[1]; n[2] = this->BaseU[2];
  newPolys->InsertNextCell(4,pts);
  newNormals->InsertNextTuple(n);

  pts[0] = 0; pts[1] = 1; pts[2] = 5; pts[3] = 4;
  n[0] = -this->BaseV[0]; n[1] = -this->BaseV[1]; n[2] = -this->BaseV[2];
  newPolys->InsertNextCell(4,pts);
  newNormals->InsertNextTuple(n);

  pts[0] = 2; pts[1] = 6; pts[2] = 7; pts[3] = 3;
  n[0] = this->BaseV[0]; n[1] = this->BaseV[1]; n[2] = this->BaseV[2];
  newPolys->InsertNextCell(4,pts);
  newNormals->InsertNextTuple(n);

  //
  // Update ourselves and release memory
  //
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->GetCellData()->SetNormals(newNormals);
  newNormals->Delete();

  newPolys->Squeeze(); // since we've estimated size; reclaim some space
  output->SetPolys(newPolys);
  newPolys->Delete();

  // Attach meta-data to output
  this->UpdateMetaData(output);

  return 1;
}
//----------------------------------------------------------------------------
void vtkShearedCubeSource::UpdateMetaData(vtkDataSet* ds)
{
  // Metadata container
  vtkFieldData* fieldData = ds->GetFieldData();

  // Clear any previous meta-data
  fieldData->RemoveArray("AxisBaseForX");
  fieldData->RemoveArray("AxisBaseForY");
  fieldData->RemoveArray("AxisBaseForZ");
  fieldData->RemoveArray("LinearTransformForX");
  fieldData->RemoveArray("LinearTransformForY");
  fieldData->RemoveArray("LinearTransformForZ");
  fieldData->RemoveArray("OrientedBoundingBox");
  fieldData->RemoveArray("AxisTitleForX");
  fieldData->RemoveArray("AxisTitleForY");
  fieldData->RemoveArray("AxisTitleForZ");
  fieldData->RemoveArray("LabelRangeForX");
  fieldData->RemoveArray("LabelRangeForY");
  fieldData->RemoveArray("LabelRangeForZ");
  fieldData->RemoveArray("LabelRangeActiveFlag");

  // New base meta-data
  if(this->EnableCustomBase != 0)
    {
    vtkNew<vtkFloatArray> uBase;
    uBase->SetNumberOfComponents(3);
    uBase->SetNumberOfTuples(1);
    uBase->SetName("AxisBaseForX");
    uBase->SetTuple(0, this->BaseU);
    fieldData->AddArray(uBase.GetPointer());

    vtkNew<vtkFloatArray> vBase;
    vBase->SetNumberOfComponents(3);
    vBase->SetNumberOfTuples(1);
    vBase->SetName("AxisBaseForY");
    vBase->SetTuple(0, this->BaseV);
    fieldData->AddArray(vBase.GetPointer());

    vtkNew<vtkFloatArray> wBase;
    wBase->SetNumberOfComponents(3);
    wBase->SetNumberOfTuples(1);
    wBase->SetName("AxisBaseForZ");
    wBase->SetTuple(0, this->BaseW);
    fieldData->AddArray(wBase.GetPointer());

    // New oriented bounding box
    vtkNew<vtkFloatArray> orientedBoundingBox;
    orientedBoundingBox->SetNumberOfComponents(6);
    orientedBoundingBox->SetNumberOfTuples(1);
    orientedBoundingBox->SetName("OrientedBoundingBox");
    orientedBoundingBox->SetTuple(0, this->OrientedBoundingBox);
    fieldData->AddArray(orientedBoundingBox.GetPointer());
    }

  if(this->EnableCustomTitle)
    {
    // Axis titles
    if(this->AxisUTitle)
      {
      vtkNew<vtkStringArray> uAxisTitle;
      uAxisTitle->SetName("AxisTitleForX");
      uAxisTitle->SetNumberOfComponents(1);
      uAxisTitle->SetNumberOfTuples(1);
      uAxisTitle->SetValue(0, this->AxisUTitle);
      fieldData->AddArray(uAxisTitle.GetPointer());
      }

    if(this->AxisVTitle)
      {
      vtkNew<vtkStringArray> vAxisTitle;
      vAxisTitle->SetName("AxisTitleForY");
      vAxisTitle->SetNumberOfComponents(1);
      vAxisTitle->SetNumberOfTuples(1);
      vAxisTitle->SetValue(0, this->AxisVTitle);
      fieldData->AddArray(vAxisTitle.GetPointer());
      }

    if(this->AxisWTitle)
      {
      vtkNew<vtkStringArray> wAxisTitle;
      wAxisTitle->SetName("AxisTitleForZ");
      wAxisTitle->SetNumberOfComponents(1);
      wAxisTitle->SetNumberOfTuples(1);
      wAxisTitle->SetValue(0, this->AxisWTitle);
      fieldData->AddArray(wAxisTitle.GetPointer());
      }
    }

  vtkNew<vtkUnsignedCharArray> activeLabelRange;
  activeLabelRange->SetNumberOfComponents(1);
  activeLabelRange->SetNumberOfTuples(3);
  activeLabelRange->SetName("LabelRangeActiveFlag");
  fieldData->AddArray(activeLabelRange.GetPointer());

  if(this->EnableCustomLabelRange)
    {
    vtkNew<vtkFloatArray> uLabelRange;
    uLabelRange->SetNumberOfComponents(2);
    uLabelRange->SetNumberOfTuples(1);
    uLabelRange->SetName("LabelRangeForX");
    uLabelRange->SetTuple(0, this->LabelRangeU);
    fieldData->AddArray(uLabelRange.GetPointer());

    vtkNew<vtkFloatArray> vLabelRange;
    vLabelRange->SetNumberOfComponents(2);
    vLabelRange->SetNumberOfTuples(1);
    vLabelRange->SetName("LabelRangeForY");
    vLabelRange->SetTuple(0, this->LabelRangeV);
    fieldData->AddArray(vLabelRange.GetPointer());

    vtkNew<vtkFloatArray> wLabelRange;
    wLabelRange->SetNumberOfComponents(2);
    wLabelRange->SetNumberOfTuples(1);
    wLabelRange->SetName("LabelRangeForZ");
    wLabelRange->SetTuple(0, this->LabelRangeW);

    activeLabelRange->SetValue(0, 1);
    activeLabelRange->SetValue(1, 1);
    activeLabelRange->SetValue(2, 1);
    fieldData->AddArray(wLabelRange.GetPointer());

    // Create custom Linear Affine transform
    this->LinearTransformU[0] = (this->LabelRangeU[1] - this->LabelRangeU[0]) / (this->OrientedBoundingBox[1] - this->OrientedBoundingBox[0]);
    this->LinearTransformU[1] = this->LabelRangeU[0] - (this->LinearTransformU[0] * this->OrientedBoundingBox[0]);

    this->LinearTransformV[0] = (this->LabelRangeV[1] - this->LabelRangeV[0]) / (this->OrientedBoundingBox[3] - this->OrientedBoundingBox[2]);
    this->LinearTransformV[1] = this->LabelRangeV[0] - (this->LinearTransformV[0] * this->OrientedBoundingBox[2]);

    this->LinearTransformW[0] = (this->LabelRangeW[1] - this->LabelRangeW[0]) / (this->OrientedBoundingBox[5] - this->OrientedBoundingBox[4]);
    this->LinearTransformW[1] = this->LabelRangeW[0] - (this->LinearTransformW[0] * this->OrientedBoundingBox[4]);

    vtkNew<vtkFloatArray> uLinearTransform;
    uLinearTransform->SetNumberOfComponents(2);
    uLinearTransform->SetNumberOfTuples(1);
    uLinearTransform->SetName("LinearTransformForX");
    uLinearTransform->SetTuple(0, this->LinearTransformU);
    fieldData->AddArray(uLinearTransform.GetPointer());

    vtkNew<vtkFloatArray> vLinearTransform;
    vLinearTransform->SetNumberOfComponents(2);
    vLinearTransform->SetNumberOfTuples(1);
    vLinearTransform->SetName("LinearTransformForY");
    vLinearTransform->SetTuple(0, this->LinearTransformV);
    fieldData->AddArray(vLinearTransform.GetPointer());

    vtkNew<vtkFloatArray> wLinearTransform;
    wLinearTransform->SetNumberOfComponents(2);
    wLinearTransform->SetNumberOfTuples(1);
    wLinearTransform->SetName("LinearTransformForZ");
    wLinearTransform->SetTuple(0, this->LinearTransformW);
    fieldData->AddArray(wLinearTransform.GetPointer());
    }
  else
    {
    activeLabelRange->SetValue(0, 0);
    activeLabelRange->SetValue(1, 0);
    activeLabelRange->SetValue(2, 0);
    }
}

//----------------------------------------------------------------------------
int vtkShearedCubeSource::RequestInformation(
    vtkInformation * info, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector)
{
  // Add new time label information
   vtkInformation *infoOutput = outputVector->GetInformationObject(0);
  if(this->EnableTimeLabel != 0 && this->TimeLabel)
    {
    // Get information object to fill
    infoOutput->Set(vtkPVInformationKeys::TIME_LABEL_ANNOTATION(), this->TimeLabel);

    // Also pretend that we have some time dependant data
    double timeRange[2] = {0,1};
    double timesteps[5] = {0, 0.25, 0.5, 0.75, 1};
    infoOutput->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), timesteps, 5);
    infoOutput->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
    }
  else
    {
    infoOutput->Remove(vtkPVInformationKeys::TIME_LABEL_ANNOTATION());
    infoOutput->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    infoOutput->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    }

  return this->Superclass::RequestInformation(info, inputVector, outputVector);;
}
