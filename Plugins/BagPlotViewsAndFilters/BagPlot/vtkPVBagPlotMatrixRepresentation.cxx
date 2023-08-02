// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVBagPlotMatrixRepresentation.h"

#include "vtkDoubleArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkTable.h"

vtkStandardNewMacro(vtkPVBagPlotMatrixRepresentation);

//----------------------------------------------------------------------------
void vtkPVBagPlotMatrixRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkPVBagPlotMatrixRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  int ret = this->Superclass::RequestData(request, inputVector, outputVector);

  vtkSmartPointer<vtkMultiBlockDataSet> data = vtkMultiBlockDataSet::GetData(inputVector[0], 0);
  if (data && data->GetNumberOfBlocks() == 4)
  {
    vtkTable* thresholdTable = vtkTable::SafeDownCast(data->GetBlock(3));
    if (thresholdTable->GetNumberOfColumns() == 1)
    {
      vtkDoubleArray* array = vtkDoubleArray::SafeDownCast(thresholdTable->GetColumn(0));
      if (array)
      {
        if (array->GetNumberOfTuples() >= 5)
        {
          this->ExtractedExplainedVariance = array->GetValue(4);
        }
        else
        {
          vtkWarningMacro("Unexpected number of tuples in threshold array,"
                          " explained Variance not extracted");
        }
      }
      else
      {
        vtkWarningMacro("Threshold array is not of expected type,"
                        " explained Variance not extracted");
      }
    }
    else
    {
      vtkWarningMacro("Threshold table does not have the expected number of columns,"
                      " explained Variance not extracted");
    }
  }
  else
  {
    vtkWarningMacro("Threshold table could not be recovered from the data,"
                    " explained Variance not extracted");
  }
  return ret;
}
