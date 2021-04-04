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
  if (!this->Superclass::RequestData(request, inputVector, outputVector))
  {
    return 0;
  }

  bool isRoot = !this->Controller || (this->Controller->GetLocalProcessId() == 0);

  vtkTable* output = vtkTable::GetData(outputVector, 0);

  // Handle > 1 ranks
  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    vtkSmartPointer<vtkDataArray> oldExtents = output->GetRowData()->GetArray("bin_extents");
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
      // We save the old bin_extents and then revert to be restored later since
      // the reduction reduces the bin_extents as well.
      output->ShallowCopy(reduceFilter->GetOutput());
      if (output->GetRowData()->GetNumberOfArrays() == 0)
      {
        vtkErrorMacro(<< "Reduced data has 0 arrays");
        return 0;
      }
      output->GetRowData()->GetArray("bin_extents")->DeepCopy(oldExtents);
      if (this->CalculateAverages)
      {
        vtkDataArray* bin_values = output->GetRowData()->GetArray("bin_values");
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

  if (this->Normalize && isRoot)
  {
    auto rowData = output->GetRowData();
    vtkDataArray* bin_values = rowData->GetArray("bin_values");
    vtkNew<vtkDoubleArray> normalized_values;
    normalized_values->SetName("bin_values");
    normalized_values->SetNumberOfComponents(bin_values->GetNumberOfComponents());
    normalized_values->SetNumberOfTuples(bin_values->GetNumberOfTuples());

    auto bin_range = vtk::DataArrayValueRange<1>(bin_values);
    int sum = 0;
    for (auto bin_val : bin_range)
    {
      sum += bin_val;
    }

    auto normalized_range = vtk::DataArrayValueRange<1>(normalized_values);
    auto bin_iter = bin_range.begin();
    auto normalized_iter = normalized_range.begin();
    for (; bin_iter != bin_range.end(); ++bin_iter, ++normalized_iter)
    {
      *normalized_iter = static_cast<double>(bin_iter[0]) / sum;
    }

    // Replace the previous bin_values array with the normalized version.
    rowData->AddArray(normalized_values);
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkPExtractHistogram::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}
