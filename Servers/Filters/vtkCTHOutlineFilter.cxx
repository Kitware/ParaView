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



vtkCxxRevisionMacro(vtkCTHOutlineFilter, "1.6");
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
  int *dimensions;
  int ghostLevels;
  
  // Hack
  dimensions = input->GetBlockPointDimensions(0);
  ghostLevels = input->GetNumberOfGhostLevels();

  append->AddInput(outlineSource->GetOutput());
  append->AddInput(tmp);

  numBlocks = input->GetNumberOfBlocks();
  for (blockId = 0; blockId < numBlocks; ++blockId)
    {
    this->UpdateProgress(static_cast<double>(blockId)/static_cast<double>(numBlocks));
    origin = input->GetBlockOrigin(blockId);
    spacing = input->GetBlockSpacing(blockId);
    bounds[0] = origin[0] + spacing[0]*ghostLevels;
    bounds[2] = origin[1] + spacing[1]*ghostLevels;
    bounds[4] = origin[2] + spacing[2]*ghostLevels;

    bounds[1] = origin[0] + spacing[0]*(dimensions[0]-1-ghostLevels);
    bounds[3] = origin[1] + spacing[1]*(dimensions[1]-1-ghostLevels);
    bounds[5] = origin[2] + spacing[2]*(dimensions[2]-1-ghostLevels);
  
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

