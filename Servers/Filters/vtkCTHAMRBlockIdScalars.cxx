/*=========================================================================

  Program:   ParaView
  Module:    vtkCTHAMRBlockIdScalars.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCTHAMRBlockIdScalars.h"
#include "vtkCTHData.h"
#include "vtkObjectFactory.h"

#include "vtkToolkits.h"
#include "vtkIntArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"


vtkCxxRevisionMacro(vtkCTHAMRBlockIdScalars, "1.1");
vtkStandardNewMacro(vtkCTHAMRBlockIdScalars);

//----------------------------------------------------------------------------
vtkCTHAMRBlockIdScalars::vtkCTHAMRBlockIdScalars()
{
}

//----------------------------------------------------------------------------
vtkCTHAMRBlockIdScalars::~vtkCTHAMRBlockIdScalars()
{
}

//----------------------------------------------------------------------------
void vtkCTHAMRBlockIdScalars::Execute()
{
  int numBlocks, blockId;
  vtkIdType numCellsPerBlock, idx;
  int dims[3];
  vtkCTHData* input = this->GetInput();
  vtkCTHData* output = this->GetOutput();
  vtkIntArray* blockIdArray = vtkIntArray::New();
  
  blockIdArray->Allocate(input->GetNumberOfCells()); 
  numBlocks = input->GetNumberOfBlocks();
  for (blockId = 0; blockId < numBlocks; ++blockId)
    {
    input->GetBlockCellDimensions(blockId, dims);
    numCellsPerBlock = dims[0] * dims[1] * dims[2];
    for (idx = 0; idx < numCellsPerBlock; ++idx)
      {
      blockIdArray->InsertNextValue(blockId);
      }
    }


  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  blockIdArray->SetName("BlockId");
  output->GetCellData()->AddArray(blockIdArray);
  output->GetCellData()->SetActiveScalars("BlockId");
  blockIdArray->Delete();
}


//------------------------------------------------------------------------------
void vtkCTHAMRBlockIdScalars::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}




