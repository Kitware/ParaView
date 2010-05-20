/*=========================================================================

  Program:   ParaView
  Module:    vtkCPMultiBlockGridBuilder.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCPMultiBlockGridBuilder - Class for creating multiblock grids.
// .SECTION Description
// Class for creating vtkMultiBlockDataSet grids for a test driver.

#ifndef __vtkCPMultiBlockGridBuilder_h
#define __vtkCPMultiBlockGridBuilder_h

#include "vtkCPBaseGridBuilder.h"

class vtkCPGridBuilder;
class vtkDataObject;
class vtkMultiBlockDataSet;
struct vtkCPMultiBlockGridBuilderInternals;

class VTK_EXPORT vtkCPMultiBlockGridBuilder : public vtkCPBaseGridBuilder
{
public:
  static vtkCPMultiBlockGridBuilder* New();
  vtkTypeMacro(vtkCPMultiBlockGridBuilder, vtkCPBaseGridBuilder);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return a grid.  BuiltNewGrid is 0 if the grid is the same
  // as the last time step.
  virtual vtkDataObject* GetGrid(unsigned long timeStep, double time,
                                 int & builtNewGrid);

  // Description:
  // Get the Grid.
  vtkMultiBlockDataSet* GetGrid();

  // Description:
  // Add a vtkCPGridBuilder.
  void AddGridBuilder(vtkCPGridBuilder* gridBuilder);

  // Description:
  // Remove a vtkCPGridBuilder.
  void RemoveGridBuilder(vtkCPGridBuilder* gridBuilder);
  
  // Description:
  // Clear out all of the current vtkCPGridBuilders.
  void RemoveAllGridBuilders();

  // Description:
  // Get the number of vtkCPGridBuilders.
  unsigned int GetNumberOfGridBuilders();

  // Description:
  // Get a specific vtkCPGridBuilder.
  vtkCPGridBuilder* GetGridBuilder(unsigned int which);

protected:
  vtkCPMultiBlockGridBuilder();
  ~vtkCPMultiBlockGridBuilder();

  // Description:
  // Set the Grid.
  void SetGrid(vtkMultiBlockDataSet* multiBlock);

private:
  vtkCPMultiBlockGridBuilder(const vtkCPMultiBlockGridBuilder&); // Not implemented
  void operator=(const vtkCPMultiBlockGridBuilder&); // Not implemented

  // Description:
  // The grid that is returned.
  vtkMultiBlockDataSet* Grid;

  // Description:
  // Internals used for storing the vtkCPGridBuilders.
  vtkCPMultiBlockGridBuilderInternals* Internal;
};

#endif
