/*=========================================================================

  Program:   ParaView
  Module:    vtkDataObjectTreeToPointSetFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkDataObjectTreeToPointSetFilter
 * @brief Filter that merges blocks from a data object tree to a vtkPolyData of vtkUnstructuredGrid
 *
 * This filter takes a vtkDataObjectTree as input and merges the blocks in the tree into a single
 * output vtkUnstructuredGrid if the ExtractSurfaces option is off. If the ExtractSurfaces option
 * is on, surfaces of non-vtkPolyData datasets in the input tree will be extracted and merged to
 * the output with vtkPolyData datasets.
 */

#ifndef vtkDataObjectTreeToPointSetFilter_h
#define vtkDataObjectTreeToPointSetFilter_h

#include "vtkPVVTKExtensionsRenderingModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

class vtkAppendDataSets;
class vtkDataObjectTree;
class vtkDataSet;
class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkDataObjectTreeToPointSetFilter
  : public vtkPointSetAlgorithm
{
public:
  static vtkDataObjectTreeToPointSetFilter* New();
  vtkTypeMacro(vtkDataObjectTreeToPointSetFilter, vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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

  //@{
  /**
   * Get/Set the tolerance to use to find coincident points when `MergePoints`
   * is `true`. Default is 0.0.
   *
   * This is simply passed on to the internal vtkAppendFilter::vtkLocator used to merge points.
   * @sa `vtkLocator::SetTolerance`.
   */
  vtkSetClampMacro(Tolerance, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(Tolerance, double);
  //@}

  //@{
  /**
   * Get/Set whether Tolerance is treated as an absolute or relative tolerance.
   * The default is to treat it as an absolute tolerance.
   */
  vtkSetMacro(ToleranceIsAbsolute, bool);
  vtkGetMacro(ToleranceIsAbsolute, bool);
  vtkBooleanMacro(ToleranceIsAbsolute, bool);
  //@}

  //@{
  /**
   * Get/Set the output type produced by this filter. Only blocks compatible with the output type
   * will be merged in the output. For example, if the output type is vtkPolyData, then
   * blocks of type vtkImageData, vtkStructuredGrid, etc. will not be merged - only vtkPolyData
   * can be merged into a vtkPolyData. On the other hand, if the output type is
   * vtkUnstructuredGrid, then blocks of almost any type will be merged in the output.
   * Valid values are VTK_POLY_DATA and VTK_UNSTRUCTURED_GRID defined in vtkType.h.
   * Defaults to VTK_UNSTRUCTURED_GRID.
   */
  //@}
  vtkSetMacro(OutputDataSetType, int);
  vtkGetMacro(OutputDataSetType, int);

protected:
  vtkDataObjectTreeToPointSetFilter();
  ~vtkDataObjectTreeToPointSetFilter() override;

  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Remove point/cell arrays not present on all processes.
   */
  void RemovePartialArrays(vtkDataSet* data);

  void ExecuteSubTree(vtkDataObjectTree* dot, vtkAppendDataSets* appender);

  unsigned int SubTreeCompositeIndex;
  bool MergePoints;
  double Tolerance;
  bool ToleranceIsAbsolute;
  int OutputDataSetType;

private:
  vtkDataObjectTreeToPointSetFilter(const vtkDataObjectTreeToPointSetFilter&) = delete;
  void operator=(const vtkDataObjectTreeToPointSetFilter&) = delete;
};

#endif
