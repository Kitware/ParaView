/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWExtractGeometryByScalar.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkKWExtractGeometryByScalar.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkKWExtractGeometryByScalar, "1.1");
vtkStandardNewMacro(vtkKWExtractGeometryByScalar);

vtkKWExtractGeometryByScalar::vtkKWExtractGeometryByScalar()
{
  this->ScalarValues = vtkIdList::New();
}

vtkKWExtractGeometryByScalar::~vtkKWExtractGeometryByScalar()
{
  this->ScalarValues->Delete();
}

void vtkKWExtractGeometryByScalar::Execute()
{
  vtkUnstructuredGrid *input = this->GetInput();
  vtkUnstructuredGrid *output = this->GetOutput();
  vtkPointData *inputPD = input->GetPointData();
  vtkCellData *inputCD = input->GetCellData();
  vtkCellData *outputCD = output->GetCellData();
  
  inputCD->CopyAllOn();
  inputPD->CopyAllOn();

  vtkIdType numCells = input->GetNumberOfCells();
  vtkIdType i, pointId, numOutputCells = 0;
  vtkGenericCell *cell = vtkGenericCell::New();
  int scalar, *cellTypes;
  
  cellTypes = new int[numCells];

  vtkCellArray *outputCells = vtkCellArray::New();
  outputCells->Allocate(numCells);
  
  for (i = 0; i < numCells; i++)
    {
    input->GetCell(i, cell);
    pointId = cell->GetPointId(0);
    scalar = (int)inputPD->GetScalars()->GetTuple1(pointId);
    if (this->ScalarValues->IsId(scalar) > -1)
      {
      outputCells->InsertNextCell(cell);
      outputCD->CopyData(inputCD, i, numOutputCells);
      cellTypes[numOutputCells++] = cell->GetCellType();
      }
    }
  
  output->SetPoints(input->GetPoints());
  output->SetCells(cellTypes, outputCells);
  output->GetPointData()->PassData(inputPD);
  
  delete [] cellTypes;
  outputCells->Delete();
  cell->Delete();
}

void vtkKWExtractGeometryByScalar::SetValue(int i, int value)
{
  this->ScalarValues->InsertId(i, value);
  this->Modified();
}

int vtkKWExtractGeometryByScalar::GetValue(int i)
{
  return this->ScalarValues->GetId(i);
}

void vtkKWExtractGeometryByScalar::RemoveAllValues()
{
  this->ScalarValues->Reset();
  this->ScalarValues->Squeeze();
  this->Modified();
}

void vtkKWExtractGeometryByScalar::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
