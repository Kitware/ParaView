/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAppendRectilinearGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAppendRectilinearGrid - appends rectliner grids together.
// .SECTION Description
// vtkAppendRectilinearGrid appends rectilinear grids to produce a
// single combined rectilinear grid. Inputs are appends based on
// their extents.

#ifndef __vtkAppendRectilinearGrid_h
#define __vtkAppendRectilinearGrid_h

#include "vtkRectilinearGridAlgorithm.h"

class VTK_EXPORT vtkAppendRectilinearGrid : public vtkRectilinearGridAlgorithm
{
public:
  static vtkAppendRectilinearGrid* New();
  vtkTypeMacro(vtkAppendRectilinearGrid, vtkRectilinearGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkAppendRectilinearGrid();
  ~vtkAppendRectilinearGrid();

  // Propagate UPDATE_EXTENT up to the inputs.
  virtual int RequestUpdateExtent(vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *);

  // Tell the output information about the data this filter will produce.
  virtual int RequestInformation (vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *);

  // Perform actual execution.
  virtual int RequestData(vtkInformation *, 
                          vtkInformationVector **, vtkInformationVector *);

  virtual int FillInputPortInformation(int port, vtkInformation *info);

  void CopyArray(vtkAbstractArray* outArray, const int* outExt,
    vtkAbstractArray* inArray, const int* inExt);
private:
  vtkAppendRectilinearGrid(const vtkAppendRectilinearGrid&); // Not implemented.
  void operator=(const vtkAppendRectilinearGrid&); // Not implemented.
};

#endif

