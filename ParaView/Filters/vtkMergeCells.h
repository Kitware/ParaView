// -*- c++ -*-

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeCells.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

=========================================================================*/

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

//#include "vtksnlGraphicsWin32Header.h"

#include <vtkObject.h>
#include <vtkUnstructuredGrid.h>
#include <vtkDataSetAttributes.h>
#include <map>

class vtkIdList;
class vtkDataSet;
class vtkCollection;

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

  vtkSetMacro(TotalCells, int);
  vtkGetMacro(TotalCells, int);

  // Description:
  //    Specify the total number of points in the final vtkUnstructuredGrid
  //    Make this call before any call to MergeDataSet().  This is an
  //    upper bound, since some points may be duplicates.

  vtkSetMacro(TotalPoints, int);
  vtkGetMacro(TotalPoints, int);

  // Description:
  //    You can specify the name of a point array that contains a global
  //    Id for every point.  Then, when DataSets are merged in, duplicate
  //    points will be detected and not included in the final merged 
  //    UnstructuredGrid.

  vtkSetStringMacro(GlobalIdArrayName);
  vtkGetStringMacro(GlobalIdArrayName);

  // Description:
  //    Provide a DataSet to be merged in to the final UnstructuredGrid.
  //    This call returns after the merge has completed.  Be sure to call
  //    SetTotalCells and SetTotalPoints before making any of these calls.

  void MergeDataSet(vtkDataSet *set);

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
//BTX
  void StartUGrid(vtkDataSetAttributes::FieldList *ptList,
                  vtkDataSetAttributes::FieldList *cellList);

  void FinishInput(vtkDataSet *set, int inputId,
                   vtkDataSetAttributes::FieldList *ptList,
                   vtkDataSetAttributes::FieldList *cellList);
//ETX

  int *MapPointsToIds(vtkDataSet *set);

  int TotalCells;
  int TotalPoints;

  int NumberOfCells;
  int NumberOfPoints;

  char *GlobalIdArrayName;

//BTX
  vtkstd::map<int, int> GlobalIdMap;
//ETX

  // We need to have all the inputs before we can
  // use copy allocate.  This is slightly less efficient because
  // inputs cannot be freed as they are used.
  vtkCollection* Inputs;

  vtkUnstructuredGrid *UnstructuredGrid;

};
