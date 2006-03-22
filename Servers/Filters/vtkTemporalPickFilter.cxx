/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalPickFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTemporalPickFilter.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"
#include "vtkMultiProcessController.h"
#include "vtkVertex.h"
#include "vtkAppendFilter.h"
#include "vtkProcessModule.h"

vtkCxxRevisionMacro(vtkTemporalPickFilter, "1.3");
vtkStandardNewMacro(vtkTemporalPickFilter);

vtkCxxSetObjectMacro(vtkTemporalPickFilter, Controller, vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkTemporalPickFilter::vtkTemporalPickFilter()
{
  this->History = NULL;
  this->Empty = true;
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->PointOrCell = 0; 
  this->HasAllData = 1;
}

//----------------------------------------------------------------------------
vtkTemporalPickFilter::~vtkTemporalPickFilter()
{
  if (this->History) {
    this->History->Delete();
    this->History = NULL;
  }
  this->SetController(0);
}

//----------------------------------------------------------------------------
void vtkTemporalPickFilter::AnimateInit()
{
  this->Empty = true;
  this->HasAllData = 1;

  if (this->History) {
    this->History->Delete();
    this->History = NULL;
  }

  vtkDataSet *input = vtkDataSet::SafeDownCast(this->GetInput());
  if (!input) 
    {
    return;
    }

  vtkCellData *icd = NULL;
  vtkPointData *ipd = NULL;
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
 
  //In Parallel AppendPolydata will be called in MPIMoveData later on.
  //APD requires at least one cell or it will output nothing.
  vtkVertex *dummy = vtkVertex::New();
  dummy->GetPointIds()->SetId(0, 0);
  this->History->Allocate(1,1);
  this->History->InsertNextCell(dummy->GetCellType(), dummy->GetPointIds());
  dummy->Delete();

  vtkPoints *opts = vtkPoints::New();
  this->History->SetPoints(opts);
  opts->InsertNextPoint(0.0,0.0,0.0);
  opts->Delete();

  vtkPointData *opd = this->History->GetPointData();
  int numArrs = this->PointOrCell ? icd->GetNumberOfArrays() : ipd->GetNumberOfArrays();
  int idx;
  for (idx = 0; idx < numArrs; idx++) 
    {
    vtkDataArray *ida = this->PointOrCell ? icd->GetArray(idx) : ipd->GetArray(idx);
    vtkDataArray *idacp = ida->NewInstance();
    idacp->SetName(ida->GetName());
    opd->AddArray(idacp);
    idacp->Delete();
    }
  for (idx = 0; idx < numArrs; idx++) 
    {
    vtkDataArray *ida = (this->PointOrCell) ? icd->GetArray(idx) : ipd->GetArray(idx);
    vtkDataArray *oda = opd->GetArray(idx);

    oda->InsertNextTuple(ida->GetTuple(0));
    }

}

//----------------------------------------------------------------------------
void vtkTemporalPickFilter::AnimateTick(double TheTime)
{
  vtkDataSet *input = vtkDataSet::SafeDownCast(this->GetInput());
  if (!input) 
    {
    return;
    }

  vtkCellData *icd = NULL;
  vtkPointData *ipd = NULL;
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

  vtkPoints *opts = this->History->GetPoints();
  if (this->Empty)
    {
    opts->InsertPoint(0, TheTime,0.0,0.0);
    }
  else
    {
    opts->InsertNextPoint(TheTime,0.0,0.0);
    }

  vtkPointData *opd = this->History->GetPointData();

  int numArrs = (this->PointOrCell) ? icd->GetNumberOfArrays() : ipd->GetNumberOfArrays();
  for (int i = 0; i < numArrs; i++) 
    {
    vtkDataArray *ida = (this->PointOrCell) ? icd->GetArray(i) : ipd->GetArray(i);
    vtkDataArray *oda = opd->GetArray(i);

    if (oda)
      {
      if (this->Empty)
        {
        oda->InsertTuple(0, ida->GetTuple(0));
        }
      else
        {
        oda->InsertNextTuple(ida->GetTuple(0));
        }
      }
    else
      {
      //numArrs increased between AnimateInit and now.
      //This can happen during parallel runs for example.
      //When it does, flag this node's contribution as invalid.
      this->HasAllData = 0;
      break;
      }
    }
  
  this->Empty = false;
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkTemporalPickFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!this->History)
    {
    this->AnimateInit();
    }

  //The vtkPickFilter input to this may have inputs on any or all nodes.
  //This gathers the valid inputs to the root to be plotted.
  if (this->Controller->GetLocalProcessId() > 0)
    {
    this->Controller->Send(&this->HasAllData, 1, 0, vtkProcessModule::TemporalPickHasData);
    if (this->HasAllData)
      {
      this->Controller->Send(this->History, 0, vtkProcessModule::TemporalPicksData);
      }
    output->ReleaseData();
    }
  else
    {
    vtkAppendFilter* append = vtkAppendFilter::New();
    int numArrs = this->History->GetPointData()->GetNumberOfArrays();
    if (numArrs > 0 && this->HasAllData)
      {
      append->AddInput(this->History);
      }
    int numProcs = this->Controller->GetNumberOfProcesses();
    for (int idx = 1; idx < numProcs; ++idx)
      {
      int TheyHaveAll = 0;
      this->Controller->Receive(&TheyHaveAll, 1, idx, vtkProcessModule::TemporalPickHasData);
      if (TheyHaveAll)
        {
        vtkUnstructuredGrid* tmp = vtkUnstructuredGrid::New();
        this->Controller->Receive(tmp, idx, vtkProcessModule::TemporalPicksData);
        numArrs = tmp->GetPointData()->GetNumberOfArrays();
        if (numArrs > 0)
          {
          append->AddInput(tmp);
          }
        tmp->Delete();
        }
      }
    append->Update();
    output->CopyStructure(append->GetOutput());
    output->GetPointData()->PassData(append->GetOutput()->GetPointData());
    output->GetCellData()->PassData(append->GetOutput()->GetCellData());
    output->GetFieldData()->PassData(append->GetOutput()->GetFieldData());
    append->Delete();
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkTemporalPickFilter::FillOutputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
void vtkTemporalPickFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "PointOrCell: " << this->PointOrCell << endl;
  os << indent << "Controller " << this->Controller << endl;
}
