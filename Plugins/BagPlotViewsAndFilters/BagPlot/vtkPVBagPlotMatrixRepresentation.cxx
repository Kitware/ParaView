/*=========================================================================

   Program: ParaView
   Module: vtkPVBagPlotMatrixRepresentation.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
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
