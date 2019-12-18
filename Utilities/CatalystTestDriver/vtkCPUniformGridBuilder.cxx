/*=========================================================================

  Program:   ParaView
  Module:    vtkCPUniformGridBuilder.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCPUniformGridBuilder.h"

#include "vtkCPBaseFieldBuilder.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkUniformGrid.h"

vtkStandardNewMacro(vtkCPUniformGridBuilder);
vtkCxxSetObjectMacro(vtkCPUniformGridBuilder, UniformGrid, vtkUniformGrid);

//----------------------------------------------------------------------------
vtkCPUniformGridBuilder::vtkCPUniformGridBuilder()
{
  for (int i = 0; i < 3; i++)
  {
    this->Dimensions[i] = 0;
    this->Spacing[i] = 0;
    this->Origin[i] = 0;
  }
  this->UniformGrid = 0;
}

//----------------------------------------------------------------------------
vtkCPUniformGridBuilder::~vtkCPUniformGridBuilder()
{
  this->SetUniformGrid(0);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkCPUniformGridBuilder::GetGrid(
  unsigned long timeStep, double time, int& builtNewGrid)
{
  vtkCPBaseFieldBuilder* fieldBuilder = this->GetFieldBuilder();
  if (!fieldBuilder)
  {
    vtkErrorMacro("FieldBuilder is not defined.");
    return 0;
  }

  builtNewGrid = this->CreateUniformGrid();

  fieldBuilder->BuildField(timeStep, time, this->UniformGrid);

  return this->UniformGrid;
}

//----------------------------------------------------------------------------
vtkUniformGrid* vtkCPUniformGridBuilder::GetUniformGrid()
{
  return this->UniformGrid;
}

//----------------------------------------------------------------------------
int* vtkCPUniformGridBuilder::GetDimensions()
{
  return this->Dimensions;
}

//----------------------------------------------------------------------------
double* vtkCPUniformGridBuilder::GetSpacing()
{
  return this->Spacing;
}

//----------------------------------------------------------------------------
double* vtkCPUniformGridBuilder::GetOrigin()
{
  return this->Origin;
}

//----------------------------------------------------------------------------
bool vtkCPUniformGridBuilder::CreateUniformGrid()
{
  bool builtNewGrid = 0;
  if (this->UniformGrid == 0)
  {
    builtNewGrid = 1;
  }
  else
  {
    for (int i = 0; i < 3; i++)
    {
      if (this->Dimensions[i] != this->UniformGrid->GetDimensions()[i] ||
        this->Spacing[i] != this->UniformGrid->GetSpacing()[i] ||
        this->Origin[i] != this->UniformGrid->GetOrigin()[i])
      {
        builtNewGrid = 1;
        break;
      }
    }
  }
  if (builtNewGrid)
  {
    vtkUniformGrid* newGrid = vtkUniformGrid::New();
    newGrid->SetSpacing(this->Spacing);
    newGrid->SetOrigin(this->Origin);

    vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
    int numberOfProcesses = controller->GetNumberOfProcesses();
    int processId = controller->GetLocalProcessId();
    // partition in the x-direction
    int extents[6] = { this->Dimensions[0] * processId / numberOfProcesses,
      this->Dimensions[0] * (processId + 1) / numberOfProcesses, 0, this->Dimensions[1], 0,
      this->Dimensions[2] };
    newGrid->SetExtent(extents);

    this->SetUniformGrid(newGrid);
    newGrid->Delete();
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkCPUniformGridBuilder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UniformGrid: " << this->UniformGrid << "\n";
  os << indent << "Dimensions: " << this->Dimensions[0] << " " << this->Dimensions[1] << " "
     << this->Dimensions[2] << "\n";
  os << indent << "Spacing: " << this->Spacing[0] << " " << this->Spacing[1] << " "
     << this->Spacing[2] << "\n";
  os << indent << "Origin: " << this->Origin[0] << " " << this->Origin[1] << " " << this->Origin[2]
     << "\n";
}
