/*=========================================================================

  Program:   ParaView
  Module:    vtkCleanUnstructuredGridCells.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Programmed 2010 by Dominik Szczerba <dominik@itis.ethz.ch>
//
// .NAME vtkCleanUnstructuredGridCells - remove duplicate/degenerate cells
//
// .SECTION Description
// Merges degenerate cells. Assumes the input grid does not contain duplicate
// points. You may want to run vtkCleanUnstructuredGrid first to assert it. If
// duplicated cells are found they are removed in the output. The filter also
// handles the case, where a cell may contain degenerate nodes (i.e. one and
// the same node is referenced by a cell more than once).
//
// .SECTION See Also
// vtkCleanPolyData

#ifndef __vtkCleanUnstructuredGridCells_h
#define __vtkCleanUnstructuredGridCells_h

#include "vtkUnstructuredGridAlgorithm.h"

class VTK_EXPORT vtkCleanUnstructuredGridCells: public vtkUnstructuredGridAlgorithm
{
public:
  static vtkCleanUnstructuredGridCells *New();

  vtkTypeMacro(vtkCleanUnstructuredGridCells, vtkUnstructuredGridAlgorithm);

  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkCleanUnstructuredGridCells();
  ~vtkCleanUnstructuredGridCells();

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkCleanUnstructuredGridCells(const vtkCleanUnstructuredGridCells&); // Not implemented
  void operator=(const vtkCleanUnstructuredGridCells&); // Not implemented
};

#endif
