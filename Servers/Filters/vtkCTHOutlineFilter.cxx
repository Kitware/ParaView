/*=========================================================================

  Program:   ParaView
  Module:    vtkCTHOutlineFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCTHOutlineFilter.h"
#include "vtkCTHData.h"
#include "vtkObjectFactory.h"

#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkCellArray.h"
#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkPolyData.h"

vtkCxxRevisionMacro(vtkCTHOutlineFilter, "1.9");
vtkStandardNewMacro(vtkCTHOutlineFilter);

//----------------------------------------------------------------------------
vtkCTHOutlineFilter::vtkCTHOutlineFilter()
{
}

//----------------------------------------------------------------------------
vtkCTHOutlineFilter::~vtkCTHOutlineFilter()
{
}

//----------------------------------------------------------------------------
void vtkCTHOutlineFilter::Execute()
{
  int blockId, numBlocks;
  vtkCTHData* input = this->GetInput();
  vtkPolyData* output = this->GetOutput();
  vtkPoints* newPts = vtkPoints::New();
  vtkCellArray* newLines = vtkCellArray::New();
  vtkIntArray* levelArray = vtkIntArray::New();
  vtkIdType ptId;
  vtkIdType edge[2];
  int level;
  double bounds[6];
  double *origin;
  double *spacing;
  int ext[6];
  int ghostLevels;
  
  origin = input->GetTopLevelOrigin();
  ghostLevels = input->GetNumberOfGhostLevels();

  numBlocks = input->GetNumberOfBlocks();
  newLines->Allocate(3*numBlocks*12);
  newPts->Allocate(numBlocks*8);
  levelArray->Allocate(numBlocks*8);
  levelArray->SetName("Level");

  for (blockId = 0; blockId < numBlocks; ++blockId)
    {
    this->UpdateProgress(static_cast<double>(blockId)/static_cast<double>(numBlocks));
    spacing = input->GetBlockSpacing(blockId);    
    input->GetBlockPointExtent(blockId, ext);
    level = input->GetBlockLevel(blockId);
    // Make sure a dimension is not collapsed before removing ghost levels.
    if (ext[0] < ext[1])
      {
      ext[0] += ghostLevels;
      ext[1] -= ghostLevels;
      }
    if (ext[2] < ext[3])
      {
      ext[2] += ghostLevels;
      ext[3] -= ghostLevels;
      }
    if (ext[4] < ext[5])
      {
      ext[4] += ghostLevels;
      ext[5] -= ghostLevels;
      }
               
    bounds[0] = origin[0] + spacing[0]*ext[0];
    bounds[1] = origin[0] + spacing[0]*ext[1];
    bounds[2] = origin[1] + spacing[1]*ext[2];
    bounds[3] = origin[1] + spacing[1]*ext[3];
    bounds[4] = origin[2] + spacing[2]*ext[4];
    bounds[5] = origin[2] + spacing[2]*ext[5];

    // Insert points:
    ptId = newPts->InsertNextPoint(bounds[0], bounds[2], bounds[4]);
    newPts->InsertNextPoint(bounds[1], bounds[2], bounds[4]);
    newPts->InsertNextPoint(bounds[0], bounds[3], bounds[4]);
    newPts->InsertNextPoint(bounds[1], bounds[3], bounds[4]);
    // generate level array:
    levelArray->InsertNextValue(level);
    levelArray->InsertNextValue(level);
    levelArray->InsertNextValue(level);
    levelArray->InsertNextValue(level);
    // Add lines:
    edge[0] = ptId+0; edge[1] = ptId+1;
    newLines->InsertNextCell(2, edge);
    edge[0] = ptId+1; edge[1] = ptId+3;
    newLines->InsertNextCell(2, edge);
    edge[0] = ptId+3; edge[1] = ptId+2;
    newLines->InsertNextCell(2, edge);
    edge[0] = ptId+2; edge[1] = ptId+0;
    newLines->InsertNextCell(2, edge);

    // If 3D ...
    if (bounds[5] > bounds[4])
      {
      newPts->InsertNextPoint(bounds[0], bounds[2], bounds[5]);
      newPts->InsertNextPoint(bounds[1], bounds[2], bounds[5]);
      newPts->InsertNextPoint(bounds[0], bounds[3], bounds[5]);
      newPts->InsertNextPoint(bounds[1], bounds[3], bounds[5]);
      // generate level array:
      levelArray->InsertNextValue(level);
      levelArray->InsertNextValue(level);
      levelArray->InsertNextValue(level);
      levelArray->InsertNextValue(level);
 
      // Add lines:
      edge[0] = ptId+4; edge[1] = ptId+5;
      newLines->InsertNextCell(2, edge);
      edge[0] = ptId+5; edge[1] = ptId+7;
      newLines->InsertNextCell(2, edge);
      edge[0] = ptId+7; edge[1] = ptId+6;
      newLines->InsertNextCell(2, edge);
      edge[0] = ptId+6; edge[1] = ptId+4;

      newLines->InsertNextCell(2, edge);
      edge[0] = ptId+0; edge[1] = ptId+4;
      newLines->InsertNextCell(2, edge);
      edge[0] = ptId+1; edge[1] = ptId+5;
      newLines->InsertNextCell(2, edge);
      edge[0] = ptId+3; edge[1] = ptId+7;
      newLines->InsertNextCell(2, edge);
      edge[0] = ptId+2; edge[1] = ptId+6;
      newLines->InsertNextCell(2, edge);
      }
    }

  output->SetPoints(newPts);
  newPts->Delete();
  newPts = 0;
  output->SetLines(newLines);
  newLines->Delete();
  newLines = 0;
  output->GetPointData()->AddArray(levelArray);
  levelArray->Delete();
  levelArray = 0;
}


//----------------------------------------------------------------------------
void vtkCTHOutlineFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

