/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCleanUnstructuredGrid.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

=========================================================================*/

// .NAME vtkCleanUnstructuredGrid

#ifndef __vtkCleanUnstructuredGrid_h
#define __vtkCleanUnstructuredGrid_h

#include <vtkDataSetToUnstructuredGridFilter.h>
#include <vtkVersion.h>

class vtkUnstructuredGrid;
class vtkPointLocator;

class VTK_EXPORT vtkCleanUnstructuredGrid: public vtkDataSetToUnstructuredGridFilter
{
public:
  static vtkCleanUnstructuredGrid *New();

  vtkTypeRevisionMacro(vtkCleanUnstructuredGrid, 
    vtkDataSetToUnstructuredGridFilter);

  void PrintSelf(ostream& os, vtkIndent indent);

  void Execute();

protected:

  vtkCleanUnstructuredGrid();
  ~vtkCleanUnstructuredGrid();

  vtkPointLocator *Locator;

private:

  vtkCleanUnstructuredGrid(const vtkCleanUnstructuredGrid&); // Not implemented
  void operator=(const vtkCleanUnstructuredGrid&); // Not implemented
};
#endif
