/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPExtractHistogram.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPExtractHistogram.h"

#include "vtkAttributeDataReductionFilter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkReductionFilter.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkPExtractHistogram);
vtkCxxRevisionMacro(vtkPExtractHistogram, "1.1");
vtkCxxSetObjectMacro(vtkPExtractHistogram, Controller, vtkMultiProcessController);
//-----------------------------------------------------------------------------
vtkPExtractHistogram::vtkPExtractHistogram()
{
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//-----------------------------------------------------------------------------
vtkPExtractHistogram::~vtkPExtractHistogram()
{
  this->SetController(0);
}

//-----------------------------------------------------------------------------
int vtkPExtractHistogram::RequestData(vtkInformation *request,
  vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  // All processes generate the histogram.
  if (!this->Superclass::RequestData(request, inputVector, outputVector))
    {
    return 0;
    }

  if (!this->Controller || this->Controller->GetNumberOfProcesses() <= 1)
    {
    // Nothing to do for single process.
    return 1;
    }

  // Now we need to collect and reduce data from all nodes on the root.
  vtkSmartPointer<vtkReductionFilter> reduceFilter = 
    vtkSmartPointer<vtkReductionFilter>::New();
  reduceFilter->SetController(this->Controller);

  bool isRoot = (this->Controller->GetLocalProcessId() ==0);
  if (isRoot)
    {
    // PostGatherHelper needs to be set only on the root node.
    vtkSmartPointer<vtkAttributeDataReductionFilter> rf = 
      vtkSmartPointer<vtkAttributeDataReductionFilter>::New();
    rf->SetAttributeType(vtkAttributeDataReductionFilter::CELL_DATA|
      vtkAttributeDataReductionFilter::FIELD_DATA);
    rf->SetReductionType(vtkAttributeDataReductionFilter::ADD);
    reduceFilter->SetPostGatherHelper(rf);
    }

  vtkDataSet* input = 0;
  if (inputVector[0]->GetNumberOfInformationObjects() > 0)
    {
    input = vtkDataSet::SafeDownCast(
      inputVector[0]->GetInformationObject(0)->Get(
        vtkDataObject::DATA_OBJECT()));
    }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataSet* output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkSmartPointer<vtkDataObject> copy;
  copy.TakeReference(output->NewInstance());
  copy->ShallowCopy(output);
  reduceFilter->SetInput(copy);
  reduceFilter->Update();
  if (isRoot)
    {
    output->ShallowCopy(reduceFilter->GetOutput());
    }
  return 1;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void vtkPExtractHistogram::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}
