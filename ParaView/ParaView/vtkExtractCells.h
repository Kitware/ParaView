// -*- c++ -*-

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractCells.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

=========================================================================*/

// .NAME vtkExtractCells - subset a vtkDataSet to create a vtkUnstructuredGrid
//
// .SECTION Description
//    Given a vtkDataSet and a list of cell Ids, create a vtkUnstructuredGrid
//    composed of these cells.  If the cell list is empty when vtkExtractCells 
//    executes, it will set up the ugrid, point and cell arrays, with no points, 
//    cells or data.
//
// .SECTION See Also

//#include "vtksnlGraphicsWin32Header.h"

#include <vtkDataSetToUnstructuredGridFilter.h>
#include <set>

class vtkIdList;
class vtkUnstructuredGrid;

class VTK_EXPORT vtkExtractCells : 
      public vtkDataSetToUnstructuredGridFilter
{
public:
  vtkTypeRevisionMacro(vtkExtractCells, vtkDataSetToUnstructuredGridFilter);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  static vtkExtractCells *New();

  void FreeCellList(); 
  void SetCellList(vtkIdList *l); 
  void AddCellList(vtkIdList *l);
  void AddCellRange(int from, int to);

protected:

  virtual void Execute();

  vtkExtractCells(){};
  ~vtkExtractCells(){};

private:

  void Copy();
  static int findInSortedList(vtkIdList *idList, vtkIdType id);
  vtkIdList *reMapPointIds(vtkDataSet *grid);

//BTX
  vtkstd::set<int> CellList;
//ETX
};
