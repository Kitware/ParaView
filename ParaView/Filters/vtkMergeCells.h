/*=========================================================================

  Program:   ParaView
  Module:    vtkMergeCells.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// .NAME vtkMergeCells - merges any number of vtkDataSets back into a single
//   vtkUnstructuredGrid
//
// .SECTION Description
//    Designed to work with distributed vtkDataSets, this class will take
//    vtkDataSets and merge them back into a single vtkUnstructuredGrid.
//    It is assumed the different DataSets have the same field arrays.  If
//    the name of a global point ID array is provided, this class will
//    refrain from including duplicate points in the merged Ugrid.  This
//    class differs from vtkAppendFilter in these ways: (1) it uses less
//    memory than that class (which uses memory equal to twice the size
//    of the final Ugrid) but requires that you know the size of the
//    final Ugrid in advance (2) this class assumes the individual DataSets have
//    the same field arrays, while vtkAppendFilter intersects the field
//    arrays (3) this class knows duplicate points may be appearing in
//    the DataSets and can filter those out, (4) this class is not a filter.

#ifndef __vtkMergeCells_h
#define __vtkMergeCells_h

#include "vtkObject.h"

#include "vtkDataSetAttributes.h" // Needed for FieldList
#include <map> // Needed for GlobalIdMap

class vtkDataSet;
class vtkUnstructuredGrid;
class vtkPointData;
class vtkCellData;

class VTK_EXPORT vtkMergeCells : public vtkObject
{ 
public:
  vtkTypeRevisionMacro(vtkMergeCells, vtkObject);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  static vtkMergeCells *New();

  // Description:
  //    Set the vtkUnstructuredGrid object that will become the
  //    union of the DataSets specified in MergeDataSet calls.

  virtual void SetUnstructuredGrid(vtkUnstructuredGrid*);
  vtkGetObjectMacro(UnstructuredGrid, vtkUnstructuredGrid);

  // Description:
  //    Specify the total number of cells in the final vtkUnstructuredGrid.
  //    Make this call before any call to MergeDataSet().

  vtkSetMacro(TotalCells, vtkIdType);
  vtkGetMacro(TotalCells, vtkIdType);

  // Description:
  //    Specify the total number of points in the final vtkUnstructuredGrid
  //    Make this call before any call to MergeDataSet().  This is an
  //    upper bound, since some points may be duplicates.

  vtkSetMacro(TotalPoints, vtkIdType);
  vtkGetMacro(TotalPoints, vtkIdType);

  // Description:
  //    You can specify the name of a point array that contains a global
  //    Id for every point.  Then, when DataSets are merged in, duplicate
  //    points will be detected and not included in the final merged 
  //    UnstructuredGrid.

  vtkSetStringMacro(GlobalIdArrayName);
  vtkGetStringMacro(GlobalIdArrayName);

  // Description:
  //    We need to know the number of different data sets that will
  //    be merged into one so we can pre-allocate some arrays.
  //    This can be an upper bound, not necessarily exact.

  vtkSetMacro(TotalNumberOfDataSets, int);
  vtkGetMacro(TotalNumberOfDataSets, int);

  // Description:
  //    Provide a DataSet to be merged in to the final UnstructuredGrid.
  //    This call returns after the merge has completed.  Be sure to call
  //    SetTotalCells, SetTotalPoints, and SetTotalNumberOfDataSets
  //    before making this call.  Return 0 if OK, -1 if error.

  int MergeDataSet(vtkDataSet *set);

  // Description:
  //    Call Finish() after merging last DataSet to free unneeded memory and to
  //    make sure the ugrid's GetNumberOfPoints() reflects the actual
  //    number of points set, not the number allocated.

  void Finish();

protected:

  vtkMergeCells();
  ~vtkMergeCells();

private:

  void FreeLists();
  void StartUGrid(vtkPointData *PD, vtkCellData *CD);
  vtkIdType *MapPointsToIds(vtkDataSet *set);
  vtkIdType AddNewCellsUnstructuredGrid(vtkDataSet *set, vtkIdType *idMap);
  vtkIdType AddNewCellsDataSet(vtkDataSet *set, vtkIdType *idMap);

  int TotalNumberOfDataSets;

  vtkIdType TotalCells;
  vtkIdType TotalPoints;

  vtkIdType NumberOfCells;     // so far
  vtkIdType NumberOfPoints;

  char *GlobalIdArrayName;

  char InputIsUGrid;

//BTX
  vtkstd::map<vtkIdType, vtkIdType> GlobalIdMap;

  vtkDataSetAttributes::FieldList *ptList;
  vtkDataSetAttributes::FieldList *cellList;
//ETX

  vtkUnstructuredGrid *UnstructuredGrid;

  int nextGrid;

  vtkMergeCells(const vtkMergeCells&); // Not implemented
  void operator=(const vtkMergeCells&); // Not implemented
};
#endif
