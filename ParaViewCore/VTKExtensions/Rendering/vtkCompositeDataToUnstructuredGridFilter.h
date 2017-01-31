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
/**
 * @class   vtkCompositeDataToUnstructuredGridFilter
 * @brief    appends all
 * vtkUnstructuredGrid leaves of the input composite dataset to a single
 * vtkUnstructuredGrid.
 *
 * vtkCompositeDataToUnstructuredGridFilter appends all vtkUnstructuredGrid
 * leaves of the input composite dataset to a single unstructure grid. The
 * subtree to be combined can be chosen using the SubTreeCompositeIndex. If
 * the SubTreeCompositeIndex is a leaf node, then no appending is required.
*/

#ifndef vtkCompositeDataToUnstructuredGridFilter_h
#define vtkCompositeDataToUnstructuredGridFilter_h

#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro
#include "vtkUnstructuredGridAlgorithm.h"

class vtkCompositeDataSet;
class vtkAppendFilter;

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkCompositeDataToUnstructuredGridFilter
  : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkCompositeDataToUnstructuredGridFilter* New();
  vtkTypeMacro(vtkCompositeDataToUnstructuredGridFilter, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get/Set the composite index of the subtree to be merged. By default set to
   * 0 i.e. root, hence entire input composite dataset is merged.
   */
  vtkSetMacro(SubTreeCompositeIndex, unsigned int);
  vtkGetMacro(SubTreeCompositeIndex, unsigned int);
  //@}

  //@{
  /**
   * Turn on/off merging of coincidental points.  Frontend to
   * vtkAppendFilter::MergePoints. Default is on.
   */
  vtkSetMacro(MergePoints, bool);
  vtkGetMacro(MergePoints, bool);
  vtkBooleanMacro(MergePoints, bool);
  //@}

protected:
  vtkCompositeDataToUnstructuredGridFilter();
  ~vtkCompositeDataToUnstructuredGridFilter();

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;

  virtual int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  /**
   * Remove point/cell arrays not present on all processes.
   */
  void RemovePartialArrays(vtkUnstructuredGrid* data);

  void AddDataSet(vtkDataSet* ds, vtkAppendFilter* appender);

  void ExecuteSubTree(vtkCompositeDataSet* cd, vtkAppendFilter* output);
  unsigned int SubTreeCompositeIndex;
  bool MergePoints;

private:
  vtkCompositeDataToUnstructuredGridFilter(
    const vtkCompositeDataToUnstructuredGridFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCompositeDataToUnstructuredGridFilter&) VTK_DELETE_FUNCTION;
};

#endif
