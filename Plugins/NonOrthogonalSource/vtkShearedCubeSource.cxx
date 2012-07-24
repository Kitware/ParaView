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
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkFieldData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkMath.h"
#include "vtkDemandDrivenPipeline.h"

#include <vtkNew.h>

vtkStandardNewMacro(vtkShearedCubeSource);

//----------------------------------------------------------------------------
vtkShearedCubeSource::vtkShearedCubeSource()
{
  this->EnableCustomBase = 0;
  this->EnableCustomBounds = 0;
  this->EnableCustomTitle = 0;
  this->EnableCustomOrigin = 0;
  this->EnableTimeLabel = 0;

  for(int i=0; i < 3; i++)
    {
    this->BaseU[i] = this->BaseV[i] = this->BaseW[i] = this->AxisOrigin[i] = 0;
    this->OrientedBoundingBox[i*2] = -0.5;
    this->OrientedBoundingBox[i*2 + 1] = +0.5;
    }
  this->BaseU[0] = this->BaseV[1] = this->BaseW[2] = 1.0;
  this->AxisUTitle = this->AxisVTitle = this->AxisWTitle = NULL;
  this->TimeLabel = NULL;
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
  int i, j, k;
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
  for(int k=0; k < 2; k++)
    {
    for(int j=0; j < 2; j++)
      {
     for(int i=0; i < 2; i++)
        {
        for(int coord=0; coord < 3; coord++)
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
  fieldData->RemoveArray("OrientedBoundingBox");
  fieldData->RemoveArray("AxisOrigin");
  fieldData->RemoveArray("AxisTitleForX");
  fieldData->RemoveArray("AxisTitleForY");
  fieldData->RemoveArray("AxisTitleForZ");

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
    }

  if(this->EnableCustomBounds)
    {
    // New oriented bounding box
    vtkNew<vtkFloatArray> orientedBoundingBox;
    orientedBoundingBox->SetNumberOfComponents(6);
    orientedBoundingBox->SetNumberOfTuples(1);
    orientedBoundingBox->SetName("OrientedBoundingBox");
    orientedBoundingBox->SetTuple(0, this->OrientedBoundingBox);
    fieldData->AddArray(orientedBoundingBox.GetPointer());
    }

  if(this->EnableCustomOrigin)
    {
    // Axis meta-data
    vtkNew<vtkFloatArray> axisOrigin;
    axisOrigin->SetNumberOfComponents(3);
    axisOrigin->SetNumberOfTuples(1);
    axisOrigin->SetName("AxisOrigin");
    axisOrigin->SetTuple(0, this->AxisOrigin);
    fieldData->AddArray(axisOrigin.GetPointer());
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
    infoOutput->Set(vtkStreamingDemandDrivenPipeline::TIME_LABEL_ANNOTATION(), this->TimeLabel);

    // Also pretend that we have some time dependant data
    double timeRange[2] = {0,1};
    double timesteps[5] = {0, 0.25, 0.5, 0.75, 1};
    infoOutput->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), timesteps, 5);
    infoOutput->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
    }
  else
    {
    infoOutput->Remove(vtkStreamingDemandDrivenPipeline::TIME_LABEL_ANNOTATION());
    infoOutput->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    infoOutput->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    }

  return this->Superclass::RequestInformation(info, inputVector, outputVector);;
}
