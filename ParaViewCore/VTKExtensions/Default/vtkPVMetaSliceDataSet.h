/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVMetaSliceDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVMetaSliceDataSet
 * Meta class for slice filter that will allow the user to switch between
 * a regular cutter filter or an extract cell by region filter.
*/

#ifndef vtkPVMetaSliceDataSet_h
#define vtkPVMetaSliceDataSet_h

#include "vtkPVDataSetAlgorithmSelectorFilter.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

class vtkImplicitFunction;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPVMetaSliceDataSet
  : public vtkPVDataSetAlgorithmSelectorFilter
{
public:
  vtkTypeMacro(vtkPVMetaSliceDataSet, vtkPVDataSetAlgorithmSelectorFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPVMetaSliceDataSet* New();

  /**
   * Enable or disable the Extract Cells By Regions.
   */
  void PreserveInputCells(int keepCellAsIs);

  /**
   * Override it so we can change the output type of the filter
   */
  vtkAlgorithm* SetActiveFilter(int index) override;

  void SetImplicitFunction(vtkImplicitFunction* func);

  // Only available for cut -------------

  /**
   * Expose method from vtkCutter
   */
  void SetCutFunction(vtkImplicitFunction* func) { this->SetImplicitFunction(func); };

  /**
   * Expose method from vtkCutter
   */
  void SetNumberOfContours(int nbContours);

  /**
   * Expose method from vtkCutter
   */
  void SetValue(int index, double value);

  /**
   * Expose method from vtkCutter
   */
  void SetGenerateTriangles(int status);

  /**
   * Expose method from vtkCutter
   */
  void SetMergePoints(bool status);

protected:
  vtkPVMetaSliceDataSet();
  ~vtkPVMetaSliceDataSet() override;

private:
  vtkPVMetaSliceDataSet(const vtkPVMetaSliceDataSet&) = delete;
  void operator=(const vtkPVMetaSliceDataSet&) = delete;

  class vtkInternals;
  vtkInternals* Internal;
};

#endif
