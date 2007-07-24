/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMergeTables.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVMergeTables.h"

#include "vtkObjectFactory.h"
#include "vtkTable.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkVariant.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkPVMergeTables);
vtkCxxRevisionMacro(vtkPVMergeTables, "1.1");
//----------------------------------------------------------------------------
vtkPVMergeTables::vtkPVMergeTables()
{
}

//----------------------------------------------------------------------------
vtkPVMergeTables::~vtkPVMergeTables()
{
}

//----------------------------------------------------------------------------
int vtkPVMergeTables::FillInputPortInformation(int port, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return this->Superclass::FillInputPortInformation(port, info);
}


//----------------------------------------------------------------------------
int vtkPVMergeTables::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  // Get output table
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkTable* output = vtkTable::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int num_connections = this->GetNumberOfInputConnections(0);
  for (int idx = 0; idx < num_connections; ++idx)
    {
    vtkInformation* info = inputVector[0]->GetInformationObject(idx);
    vtkTable* curTable= vtkTable::SafeDownCast(
      info->Get(vtkDataObject::DATA_OBJECT()));

    if (output->GetNumberOfRows() == 0)
      {
      // Copy output structure from the first non-empty input.
      output->DeepCopy(curTable);
      continue;
      }
    
    vtkIdType numRows = curTable->GetNumberOfRows();
    vtkIdType numCols = curTable->GetNumberOfColumns();
    for (vtkIdType i = 0; i < numRows; i++)
      {
      vtkIdType curRow = output->InsertNextBlankRow();
      for (vtkIdType j = 0; j < numCols; j++)
        {
        output->SetValue(curRow, j, curTable->GetValue(i, j));
        }
      }
    }
  cout << "Num rows: " << output->GetNumberOfRows() << endl;

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVMergeTables::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


