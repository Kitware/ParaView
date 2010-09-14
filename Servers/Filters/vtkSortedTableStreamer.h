/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSortedTableStreamer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSortedTableStreamer - return a sorted subset of the original table
//
// .SECTION Description
// This filter is used quickly get a sorted subset of a given vtkTable.
// By sorted we mean a subset build from a global sort even if some optimisation
// allow us to skip a global table sorting.

#ifndef __vtkSortedTableStreamer_h
#define __vtkSortedTableStreamer_h

#include "vtkTableAlgorithm.h"
class vtkTable;
class vtkDataArray;
class vtkMultiProcessController;

class VTK_EXPORT vtkSortedTableStreamer : public vtkTableAlgorithm
{
private:
  class InternalsBase;
  template<class T> class Internals;
  InternalsBase* Internal;

public:
  static void PrintInfo(vtkTable* input);
  // Description:
  // Test the internal structure and make sure that they behave as expected.
  // Return true if everything is OK, false otherwise.
  static bool TestInternalClasses();
  static vtkSortedTableStreamer* New();
  vtkTypeMacro(vtkSortedTableStreamer, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Only one input which is the table to sort
  int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // Block index used to select a data range
  vtkGetMacro(Block, vtkIdType);
  vtkSetMacro(Block, vtkIdType);

  // Description:
  // Set the block size. Default value is 1024
  vtkGetMacro(BlockSize, vtkIdType);
  vtkSetMacro(BlockSize, vtkIdType);

  // Description:
  // Choose on which colum the sort operation should occurs
  vtkGetMacro(SelectedComponent,int);
  vtkSetMacro(SelectedComponent,int);

  // Description:
  // Get/Set the MPI controller used for gathering.
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  // Choose on which colum the sort operation should occurs
  const char* GetColumnNameToSort();

  // Update column to sort has well as invalidating the pre-processing
  void SetColumnNameToSort(const char* columnName);

  // Choose if the sorting order should be inverted or not
  void SetInvertOrder(int newValue);
  vtkGetMacro(InvertOrder, int);

protected:
  vtkSortedTableStreamer();
  ~vtkSortedTableStreamer();

  int RequestData( vtkInformation*,
                   vtkInformationVector**,
                   vtkInformationVector*);

  void CreateInternalIfNeeded(vtkTable* input, vtkDataArray* data);
  vtkDataArray* GetDataArrayToProcess(vtkTable* input);

  // Description:
  // Choose on which colum the sort operation should occurs
  vtkGetStringMacro(ColumnToSort);
  vtkSetStringMacro(ColumnToSort);


  vtkIdType Block;
  vtkIdType BlockSize;
  vtkMultiProcessController* Controller;

  char* ColumnToSort;
  int SelectedComponent;
  int InvertOrder;
private:
  vtkSortedTableStreamer(const vtkSortedTableStreamer&); // Not implemented
  void operator=(const vtkSortedTableStreamer&);   // Not implemented
};

#endif
