// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPTemporalRanges.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkPTemporalRanges.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkReductionFilter.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

//=============================================================================
//=============================================================================
class vtkPTemporalRanges::vtkRangeTableReduction : public vtkTableAlgorithm
{
public:
  vtkTypeMacro(vtkPTemporalRanges::vtkRangeTableReduction, vtkTableAlgorithm);
  static vtkRangeTableReduction* New()
  {
    vtkRangeTableReduction* ret = new vtkRangeTableReduction;
    ret->InitializeObjectBase();
    return ret;
  }

  vtkGetObjectMacro(Parent, vtkPTemporalRanges);
  vtkSetObjectMacro(Parent, vtkPTemporalRanges);

protected:
  vtkRangeTableReduction() { this->Parent = NULL; }
  ~vtkRangeTableReduction() { this->SetParent(NULL); }

  virtual int FillInputPortInformation(int port, vtkInformation* info) override
  {
    info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
    return this->Superclass::FillInputPortInformation(port, info);
  }

  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkPTemporalRanges* Parent;
};

//-----------------------------------------------------------------------------
int vtkPTemporalRanges::vtkRangeTableReduction::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  vtkTable* output = vtkTable::GetData(outputVector);

  this->Parent->InitializeTable(output);

  for (int i = 0; i < numInputs; i++)
  {
    vtkTable* input = vtkTable::GetData(inputVector[0], i);
    this->Parent->AccumulateTable(input, output);
  }

  return 1;
}

//=============================================================================
//=============================================================================
vtkStandardNewMacro(vtkPTemporalRanges);

vtkCxxSetObjectMacro(vtkPTemporalRanges, Controller, vtkMultiProcessController);

//-----------------------------------------------------------------------------
vtkPTemporalRanges::vtkPTemporalRanges()
{
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

vtkPTemporalRanges::~vtkPTemporalRanges()
{
  this->SetController(NULL);
}

void vtkPTemporalRanges::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Controller: " << this->Controller << endl;
}

//-----------------------------------------------------------------------------
int vtkPTemporalRanges::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestData(request, inputVector, outputVector))
  {
    return 0;
  }

  if (!request->Has(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING()))
  {
    // Finished last execution.  Reduce tables.
    this->Reduce(vtkTable::GetData(outputVector));
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkPTemporalRanges::Reduce(vtkTable* table)
{
  if (!this->Controller || (this->Controller->GetNumberOfProcesses() <= 1))
  {
    return;
  }

  VTK_CREATE(vtkReductionFilter, reduceFilter);
  reduceFilter->SetController(this->Controller);

  VTK_CREATE(vtkPTemporalRanges::vtkRangeTableReduction, reduceOperation);
  reduceOperation->SetParent(this);
  reduceFilter->SetPostGatherHelper(reduceOperation);

  VTK_CREATE(vtkTable, copy);
  copy->ShallowCopy(table);
  reduceFilter->SetInputData(copy);
  reduceFilter->Update();

  if (this->Controller->GetLocalProcessId() == 0)
  {
    table->ShallowCopy(reduceFilter->GetOutput());
  }
  else
  {
    table->Initialize();
  }
}
