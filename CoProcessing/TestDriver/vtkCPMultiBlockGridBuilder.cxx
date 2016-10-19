/*=========================================================================

  Program:   ParaView
  Module:    vtkCPMultiBlockGridBuilder.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCPMultiBlockGridBuilder.h"

#include "vtkCPGridBuilder.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <algorithm> // for std::find
#include <vector>

vtkStandardNewMacro(vtkCPMultiBlockGridBuilder);
vtkCxxSetObjectMacro(vtkCPMultiBlockGridBuilder, Grid, vtkMultiBlockDataSet);

struct vtkCPMultiBlockGridBuilderInternals
{
  typedef std::vector<vtkSmartPointer<vtkCPGridBuilder> > GridBuilderContainer;
  typedef GridBuilderContainer::iterator GridBuilderIterator;
  GridBuilderContainer GridBuilders;
};

//----------------------------------------------------------------------------
vtkCPMultiBlockGridBuilder::vtkCPMultiBlockGridBuilder()
{
  this->Grid = 0;
  this->Internal = new vtkCPMultiBlockGridBuilderInternals;
}

//----------------------------------------------------------------------------
vtkCPMultiBlockGridBuilder::~vtkCPMultiBlockGridBuilder()
{
  this->SetGrid(0);
  delete this->Internal;
  this->Internal = 0;
}

//----------------------------------------------------------------------------
vtkMultiBlockDataSet* vtkCPMultiBlockGridBuilder::GetGrid()
{
  return this->Grid;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkCPMultiBlockGridBuilder::GetGrid(
  unsigned long timeStep, double time, int& builtNewGrid)
{
  builtNewGrid = 0;
  if (!this->Grid)
  {
    builtNewGrid = 1;
    vtkMultiBlockDataSet* multiBlock = vtkMultiBlockDataSet::New();
    this->SetGrid(multiBlock);
    multiBlock->Delete();
  }
  unsigned int numberOfBlocks = static_cast<unsigned int>(this->Internal->GridBuilders.size());
  if (this->Grid->GetNumberOfBlocks() != numberOfBlocks)
  {
    builtNewGrid = 1;
    this->Grid->SetNumberOfBlocks(numberOfBlocks);
  }
  for (unsigned int ui = 0; ui < numberOfBlocks; ui++)
  {
    int builtNewDataObject = 0;
    this->Grid->SetBlock(
      ui, this->Internal->GridBuilders[ui]->GetGrid(timeStep, time, builtNewDataObject));
    if (builtNewDataObject)
    {
      builtNewGrid = 1;
    }
  }
  return this->Grid;
}

//----------------------------------------------------------------------------
void vtkCPMultiBlockGridBuilder::AddGridBuilder(vtkCPGridBuilder* gridBuilder)
{
  this->Internal->GridBuilders.push_back(gridBuilder);
}

//----------------------------------------------------------------------------
void vtkCPMultiBlockGridBuilder::RemoveGridBuilder(vtkCPGridBuilder* gridBuilder)
{
  vtkCPMultiBlockGridBuilderInternals::GridBuilderIterator it = std::find(
    this->Internal->GridBuilders.begin(), this->Internal->GridBuilders.end(), gridBuilder);
  if (it != this->Internal->GridBuilders.end())
  {
    this->Internal->GridBuilders.erase(it);
  }
}

//----------------------------------------------------------------------------
void vtkCPMultiBlockGridBuilder::RemoveAllGridBuilders()
{
  this->Internal->GridBuilders.clear();
}

//----------------------------------------------------------------------------
unsigned int vtkCPMultiBlockGridBuilder::GetNumberOfGridBuilders()
{
  return static_cast<unsigned int>(this->Internal->GridBuilders.size());
}

//----------------------------------------------------------------------------
vtkCPGridBuilder* vtkCPMultiBlockGridBuilder::GetGridBuilder(unsigned int which)
{
  if (which >= this->GetNumberOfGridBuilders())
  {
    vtkWarningMacro("Bad input.");
    return 0;
  }
  return this->Internal->GridBuilders[which];
}

//----------------------------------------------------------------------------
void vtkCPMultiBlockGridBuilder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Grid: " << this->Grid << endl;
}
