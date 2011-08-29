/*=========================================================================

  Program:   ParaView
  Module:    vtkCompositeDataToUnstructuredGridFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCompositeDataToUnstructuredGridFilter -  appends all
// vtkUnstructuredGrid leaves of the input composite dataset to a single
// vtkUnstructuredGrid.
// .SECTION Description
// vtkCompositeDataToUnstructuredGridFilter appends all vtkUnstructuredGrid
// leaves of the input composite dataset to a single unstructure grid. The
// subtree to be combined can be chosen using the SubTreeCompositeIndex. If 
// the SubTreeCompositeIndex is a leaf node, then no appending is required.

#ifndef __vtkCompositeDataToUnstructuredGridFilter_h
#define __vtkCompositeDataToUnstructuredGridFilter_h

#include "vtkUnstructuredGridAlgorithm.h"

class vtkCompositeDataSet;
class vtkAppendFilter;

class VTK_EXPORT vtkCompositeDataToUnstructuredGridFilter : 
  public vtkUnstructuredGridAlgorithm
{
public:
  static vtkCompositeDataToUnstructuredGridFilter* New();
  vtkTypeMacro(vtkCompositeDataToUnstructuredGridFilter,
    vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the composite index of the subtree to be merged. By default set to
  // 0 i.e. root, hence entire input composite dataset is merged.
  vtkSetMacro(SubTreeCompositeIndex, unsigned int);
  vtkGetMacro(SubTreeCompositeIndex, unsigned int);

  // Description:
  // Turn on/off merging of coincidental points.  Frontend to
  // vtkAppendFilter::MergePoints. Default is on.
  vtkSetMacro(MergePoints, bool);
  vtkGetMacro(MergePoints, bool);
  vtkBooleanMacro(MergePoints, bool);

//BTX
protected:
  vtkCompositeDataToUnstructuredGridFilter();
  ~vtkCompositeDataToUnstructuredGridFilter();

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // Remove point/cell arrays not present on all processes.
  void RemovePartialArrays(vtkUnstructuredGrid* data);

  void AddDataSet(vtkDataSet* ds, vtkAppendFilter* appender);

  void ExecuteSubTree(vtkCompositeDataSet* cd, vtkAppendFilter* output);
  unsigned int SubTreeCompositeIndex;
  bool MergePoints;
private:
  vtkCompositeDataToUnstructuredGridFilter(const vtkCompositeDataToUnstructuredGridFilter&); // Not implemented
  void operator=(const vtkCompositeDataToUnstructuredGridFilter&); // Not implemented
//ETX
};

#endif

