/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalProbeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTemporalProbeFilter.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkTemporalProbeFilter, "1.4");
vtkStandardNewMacro(vtkTemporalProbeFilter);


//----------------------------------------------------------------------------
vtkTemporalProbeFilter::vtkTemporalProbeFilter()
{
  this->History = NULL;
  this->Empty = true;
  this->PointOrCell = 0; 
}

//----------------------------------------------------------------------------
vtkTemporalProbeFilter::~vtkTemporalProbeFilter()
{
  if (this->History) {
    this->History->Delete();
    this->History = NULL;
  }
}

//----------------------------------------------------------------------------
void vtkTemporalProbeFilter::AnimateInit()
{
  this->Empty = true;
  this->Modified();

  if (this->History) {
    this->History->Delete();
    this->History = NULL;
  }

  vtkDataSet *input = vtkDataSet::SafeDownCast(this->GetInput());
  if (!input) 
    {
    return;
    }

  vtkCellData *icd;
  vtkPointData *ipd;
  if (this->PointOrCell)
    {
    icd = input->GetCellData();
    if (!icd) 
      {
      return;
      }
    }
  else
    {
    ipd = input->GetPointData();
    if (!ipd) 
      {
      return;
      }
    }

  this->History = vtkUnstructuredGrid::New();

  vtkPoints *opts = vtkPoints::New();
  this->History->SetPoints(opts);
  opts->Delete();

  vtkPointData *opd = this->History->GetPointData();
  int numArrs = this->PointOrCell ? icd->GetNumberOfArrays() : ipd->GetNumberOfArrays();
  for (int i = 0; i < numArrs; i++) 
    {
    vtkDataArray *ida = this->PointOrCell ? icd->GetArray(i) : ipd->GetArray(i);
    opd->AddArray(ida->NewInstance());
    vtkDataArray *oda = opd->GetArray(i);
    oda->SetName(ida->GetName());
    }
}

//----------------------------------------------------------------------------
void vtkTemporalProbeFilter::AnimateTick(double TheTime)
{
  vtkDataSet *input = vtkDataSet::SafeDownCast(this->GetInput());
  if (!input) 
    {
    return;
    }

  vtkCellData *icd;
  vtkPointData *ipd;
  if (this->PointOrCell)
    {
    icd = input->GetCellData();
    if (!icd) 
      {
      return;
      }
    }
  else
    {
    ipd = input->GetPointData();
    if (!ipd) 
      {
      return;
      }
    }

  //Want to run silently even when input is the line probe, so do not warn.
  //int numPts = input->GetNumberOfPoints();
  //if (numPts != 1) 
  //{
  //vtkWarningMacro("Warning TemporalProbeFilter will only probe first point.");
  //}

  vtkPoints *opts = this->History->GetPoints();
  opts->InsertNextPoint(TheTime,0.0,0.0);

  vtkPointData *opd = this->History->GetPointData();

  int numArrs = (this->PointOrCell) ? icd->GetNumberOfArrays() : ipd->GetNumberOfArrays();
  for (int i = 0; i < numArrs; i++) 
    {
    vtkDataArray *ida = (this->PointOrCell) ? icd->GetArray(i) : ipd->GetArray(i);
    vtkDataArray *oda = opd->GetArray(i);
    int numComp = ida->GetNumberOfComponents();
    double *x = new double[numComp];
    ida->GetTuple(0,x);
    oda->InsertNextTuple(x);
    delete(x);
    }  

  this->Empty = false;
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkTemporalProbeFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut from the info
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (this->Empty) 
    {
    output->DeepCopy(input);
    } 
  else 
    {
    output->DeepCopy(History);
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkTemporalProbeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "PointOrCell: " << this->PointOrCell << endl;

}

