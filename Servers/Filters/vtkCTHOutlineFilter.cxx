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
#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkOutlineSource.h"
#include "vtkAppendPolyData.h"
#include "vtkPolyData.h"



vtkCxxRevisionMacro(vtkCTHOutlineFilter, "1.7");
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
  vtkOutlineSource* outlineSource;
  outlineSource = vtkOutlineSource::New();
  vtkPolyData* tmp = vtkPolyData::New();
  vtkAppendPolyData *append = vtkAppendPolyData::New();  
  double bounds[6];
  double *origin;
  double *spacing;
  int ext[6];
  int ghostLevels;
  
  origin = input->GetTopLevelOrigin();
  ghostLevels = input->GetNumberOfGhostLevels();

  append->AddInput(outlineSource->GetOutput());
  append->AddInput(tmp);

  numBlocks = input->GetNumberOfBlocks();
  for (blockId = 0; blockId < numBlocks; ++blockId)
    {
    this->UpdateProgress(static_cast<double>(blockId)/static_cast<double>(numBlocks));
    spacing = input->GetBlockSpacing(blockId);    
    input->GetBlockPointExtent(blockId, ext);
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
  
    outlineSource->SetBounds(bounds);
    append->Update();

    // Copy output to input to append next block.
    tmp->ShallowCopy(append->GetOutput());
    }
  
  this->GetOutput()->ShallowCopy(append->GetOutput());

  append->Delete();
  outlineSource->Delete();
  tmp->Delete();
}


//----------------------------------------------------------------------------
void vtkCTHOutlineFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

