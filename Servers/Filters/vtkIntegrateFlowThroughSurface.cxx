/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIntegrateFlowThroughSurface.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkIntegrateFlowThroughSurface.h"
#include "vtkIntegrateAttributes.h"
#include "vtkSurfaceVectors.h"
#include "vtkObjectFactory.h"
#include "vtkDataSet.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"

vtkCxxRevisionMacro(vtkIntegrateFlowThroughSurface, "1.1");
vtkStandardNewMacro(vtkIntegrateFlowThroughSurface);

//-----------------------------------------------------------------------------
vtkIntegrateFlowThroughSurface::vtkIntegrateFlowThroughSurface()
{
  this->InputVectorsSelection = 0;
}

//-----------------------------------------------------------------------------
vtkIntegrateFlowThroughSurface::~vtkIntegrateFlowThroughSurface()
{
  this->SetInputVectorsSelection(0);
}

//-----------------------------------------------------------------------------
void vtkIntegrateFlowThroughSurface::ComputeInputUpdateExtent()
{
  vtkDataSet* input = this->GetInput();
  vtkUnstructuredGrid* output = this->GetOutput();

  input->SetUpdateExtent(output->GetUpdatePiece(), 
                         output->GetUpdateNumberOfPieces(),
                         output->GetUpdateGhostLevel()+1);

}

//-----------------------------------------------------------------------------
void vtkIntegrateFlowThroughSurface::Execute()
{
  vtkDataSet* input = this->GetInput();
  vtkUnstructuredGrid* output = this->GetOutput();
  vtkDataSet* inputCopy;

  inputCopy = input->NewInstance();
  inputCopy->CopyStructure(input);
  vtkDataArray* vectors;
  vectors = input->GetPointData()->GetVectors(this->InputVectorsSelection);
  if (vectors == 0)
    {
    vtkErrorMacro("Missing Vectors.");
    inputCopy->Delete();
    return;
    }
  inputCopy->GetPointData()->SetVectors(vectors);
  inputCopy->GetCellData()->AddArray(
                               input->GetCellData()->GetArray("vtkGhostLevels"));

  vtkSurfaceVectors* dot = vtkSurfaceVectors::New();
  dot->SetInput(inputCopy);
  dot->SetConstraintModeToPerpendicularScale();
  vtkIntegrateAttributes* integrate = vtkIntegrateAttributes::New();
  integrate->SetInput(dot->GetOutput());
  vtkUnstructuredGrid* integrateOutput = integrate->GetOutput();
  integrateOutput->Update();

  output->CopyStructure(integrateOutput);
  output->GetPointData()->PassData(integrateOutput->GetPointData());
  vtkDataArray* flow = output->GetPointData()->GetArray("Perpendicular Scale");
  if (flow)
    {
    flow->SetName("Surface Flow");
    }  
  
  dot->Delete();
  dot = 0;
  integrate->Delete();
  integrate = 0;
  inputCopy->Delete();
  inputCopy = 0;
}        

//-----------------------------------------------------------------------------
void vtkIntegrateFlowThroughSurface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->InputVectorsSelection)
    {
    os << indent <<  "InputVectorsSelection: " 
       <<   this->InputVectorsSelection << endl;
    }
  else
    {
    os << indent <<  "InputVectorsSelection: NULL\n"; 
    }
}

