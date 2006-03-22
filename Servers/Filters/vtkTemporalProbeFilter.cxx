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
#include "vtkPolyData.h"
#include "vtkMultiProcessController.h"
#include "vtkVertex.h"

vtkCxxRevisionMacro(vtkTemporalProbeFilter, "1.6");
vtkStandardNewMacro(vtkTemporalProbeFilter);

vtkCxxSetObjectMacro(vtkTemporalProbeFilter, Controller, vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkTemporalProbeFilter::vtkTemporalProbeFilter()
{
  this->History = NULL;
  this->Empty = true;
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkTemporalProbeFilter::~vtkTemporalProbeFilter()
{
  if (this->History) {
    this->History->Delete();
    this->History = NULL;
  }
  this->SetController(0);
}

//----------------------------------------------------------------------------
void vtkTemporalProbeFilter::AnimateInit()
{
  this->Empty = true;

  if (this->History) {
    this->History->Delete();
    this->History = NULL;
  }

  vtkDataSet *input = vtkDataSet::SafeDownCast(this->GetInput());
  if (!input) 
    {
    return;
    }

  vtkPointData *ipd = NULL;
  ipd = input->GetPointData();
  if (!ipd) 
    {
    return;
    }

  this->History = vtkPolyData::New();

  //In Parallel AppendPolydata will be called in MPIMoveData later on.
  //APD requires at least one cell or it may output nothing.
  vtkVertex *dummy = vtkVertex::New();
  dummy->GetPointIds()->SetId(0, 0);
  this->History->Allocate(1,1);
  this->History->InsertNextCell(dummy->GetCellType(), dummy->GetPointIds());
  dummy->Delete();

  //Create an array for the time recordings.
  vtkPoints *opts = vtkPoints::New();
  this->History->SetPoints(opts);
  //Make a dummy first time point in case RequestData is called before 
  //AnimateTick puts any samples in.
  opts->InsertNextPoint(0.0,0.0,0.0);
  opts->Delete();

  //Copy the format of the input's pointdata arrays.
  vtkPointData *opd = this->History->GetPointData();
  int numArrs = ipd->GetNumberOfArrays();
  int idx;
  for (idx = 0; idx < numArrs; idx++) 
    {
    vtkDataArray *ida = ipd->GetArray(idx);
    vtkDataArray *idacp = ida->NewInstance();
    idacp->SetName(ida->GetName());
    opd->AddArray(idacp);
    idacp->Delete();
    }

  //Add initial attributes to go along with the first time point.
  for (idx = 0; idx < numArrs; idx++) 
    {
    vtkDataArray *ida = ipd->GetArray(idx);
    vtkDataArray *oda = opd->GetArray(idx);
    oda->InsertNextTuple(ida->GetTuple(0));
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

  vtkPointData *ipd = NULL;
  ipd = input->GetPointData();
  if (!ipd) 
    {
    return;
    }

  //Make a new point corresponding to this time.
  vtkPoints *opts = this->History->GetPoints();
  if (this->Empty)
    {
    //Overwrite initial value from AnimateInit.
    opts->InsertPoint(0, TheTime,0.0,0.0);
    }
  else
    {
    //Add this value to the History.
    opts->InsertNextPoint(TheTime,0.0,0.0);
    }

  //Assign the input's first point's attribute data to the new point.
  vtkPointData *opd = this->History->GetPointData();

  int numArrs = ipd->GetNumberOfArrays();
  for (int i = 0; i < numArrs; i++) 
    {
    vtkDataArray *ida = ipd->GetArray(i);
    vtkDataArray *oda = opd->GetArray(i);

    if (this->Empty)
      {
      oda->InsertTuple(0, ida->GetTuple(0));
      }
    else
      {
      oda->InsertNextTuple(ida->GetTuple(0));
      }
    }

  this->Empty = false;
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkTemporalProbeFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut from the info
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!this->History)
    {
    //make sure we have something to pass along
    this->AnimateInit();
    }

  output->DeepCopy(this->History);

  //The PProbe input to this only has data on root node.
  //Satellites do not have valid data anyway then, and if they 
  //release their outputs MPIMoveData will work properly.
  int procId = 0;
  int numProcs = 1;
  if ( this->Controller )
    {
    procId = this->Controller->GetLocalProcessId();
    numProcs = this->Controller->GetNumberOfProcesses();
    }  
  if ( procId )
    { 
    output->ReleaseData();
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkTemporalProbeFilter::FillOutputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return 1;
}

//----------------------------------------------------------------------------
void vtkTemporalProbeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Controller " << this->Controller << endl;
}

