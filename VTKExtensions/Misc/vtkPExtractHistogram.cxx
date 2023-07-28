// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPExtractHistogram.h"

#include "vtkAttributeDataReductionFilter.h"
#include "vtkCellData.h"
#include "vtkCommunicator.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkReductionFilter.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

#include <string>
#include <vtksys/RegularExpression.hxx>

vtkStandardNewMacro(vtkPExtractHistogram);
vtkCxxSetObjectMacro(vtkPExtractHistogram, Controller, vtkMultiProcessController);
//-----------------------------------------------------------------------------
vtkPExtractHistogram::vtkPExtractHistogram()
{
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//-----------------------------------------------------------------------------
vtkPExtractHistogram::~vtkPExtractHistogram()
{
  this->SetController(nullptr);
}

//-----------------------------------------------------------------------------
bool vtkPExtractHistogram::GetInputArrayRange(vtkInformationVector** inputVector, double range[2])
{
  if (!this->Controller || this->Controller->GetNumberOfProcesses() <= 1)
  {
    return this->Superclass::GetInputArrayRange(inputVector, range);
  }

  double local_range[2] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };

  // it's okay if we fail to determine the range locally, hence we ignore the
  // return value in this call.
  this->Superclass::GetInputArrayRange(inputVector, local_range);

  if (!this->Controller->AllReduce(&local_range[0], &range[0], 1, vtkCommunicator::MIN_OP) ||
    !this->Controller->AllReduce(&local_range[1], &range[1], 1, vtkCommunicator::MAX_OP))
  {
    vtkErrorMacro("Parallel communication error. Could not reduce ranges.");
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
int vtkPExtractHistogram::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // All processes generate the histogram.
  // However we want to avoid the super class to normalize/accumulate the results, hence temporarily
  // disable these functionalities.
  bool tempNormalize = this->Normalize;
  this->Normalize = false;
  bool tempAccumulation = this->Accumulation;
  this->Accumulation = false;

  int superRequestData = this->Superclass::RequestData(request, inputVector, outputVector);

  this->Normalize = tempNormalize;
  this->Accumulation = tempAccumulation;
  if (superRequestData == 0)
  {
    return 0;
  }

  bool isRoot = !this->Controller || (this->Controller->GetLocalProcessId() == 0);

  vtkTable* output = vtkTable::GetData(outputVector, 0);

  // Handle > 1 ranks
  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    vtkSmartPointer<vtkDataArray> oldExtents =
      output->GetRowData()->GetArray(this->BinExtentsArrayName);
    if (oldExtents == nullptr)
    {
      // Nothing to do if there is no data
      return 1;
    }
    // Now we need to collect and reduce data from all nodes on the root.
    vtkSmartPointer<vtkReductionFilter> reduceFilter = vtkSmartPointer<vtkReductionFilter>::New();
    reduceFilter->SetController(this->Controller);

    if (isRoot)
    {
      // PostGatherHelper needs to be set only on the root node.
      vtkSmartPointer<vtkAttributeDataReductionFilter> rf =
        vtkSmartPointer<vtkAttributeDataReductionFilter>::New();
      rf->SetAttributeType(vtkAttributeDataReductionFilter::ROW_DATA);
      rf->SetReductionType(vtkAttributeDataReductionFilter::ADD);
      reduceFilter->SetPostGatherHelper(rf);
    }

    vtkSmartPointer<vtkTable> copy = vtkSmartPointer<vtkTable>::New();
    copy->ShallowCopy(output);
    reduceFilter->SetInputData(copy);
    reduceFilter->Update();
    if (isRoot)
    {
      // We save the old bin extents and then revert to be restored later since
      // the reduction reduces the bin extents as well.
      output->ShallowCopy(reduceFilter->GetOutput());
      if (output->GetRowData()->GetNumberOfArrays() == 0)
      {
        vtkErrorMacro(<< "Reduced data has 0 arrays");
        return 0;
      }
      output->GetRowData()->GetArray(this->BinExtentsArrayName)->DeepCopy(oldExtents);
      if (this->CalculateAverages)
      {
        vtkDataArray* bin_values = output->GetRowData()->GetArray(this->BinValuesArrayName);
        vtksys::RegularExpression reg_ex("^(.*)_average$");
        int numArrays = output->GetRowData()->GetNumberOfArrays();
        for (int i = 0; i < numArrays; i++)
        {
          vtkDataArray* array = output->GetRowData()->GetArray(i);
          if (array && reg_ex.find(array->GetName()))
          {
            int numComps = array->GetNumberOfComponents();
            std::string name = reg_ex.match(1) + "_total";
            vtkDataArray* tarray = output->GetRowData()->GetArray(name.c_str());
            for (vtkIdType idx = 0; idx < this->BinCount; idx++)
            {
              for (int j = 0; j < numComps; j++)
              {
                array->SetComponent(
                  idx, j, tarray->GetComponent(idx, j) / bin_values->GetTuple1(idx));
              }
            }
          }
        }
      }
    }
    else
    {
      output->Initialize();
    }
  }

  if (isRoot)
  {
    if (this->Normalize)
    {
      this->Superclass::NormalizeBins(output);
    }
    if (this->Accumulation)
    {
      this->Superclass::AccumulateBins(output);
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkPExtractHistogram::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}
