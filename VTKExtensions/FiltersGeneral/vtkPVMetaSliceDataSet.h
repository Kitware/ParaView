// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVMetaSliceDataSet
 * Meta class for slice filter that will allow the user to switch between
 * a regular cutter filter or an extract cell by region filter.
 */

#ifndef vtkPVMetaSliceDataSet_h
#define vtkPVMetaSliceDataSet_h

#include "vtkPVDataSetAlgorithmSelectorFilter.h"
#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports
#include "vtkSmartPointer.h"

class vtkImplicitFunction;
class vtkInformation;
class vtkInformationVector;
class vtkIncrementalPointLocator;

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkPVMetaSliceDataSet
  : public vtkPVDataSetAlgorithmSelectorFilter
{
public:
  vtkTypeMacro(vtkPVMetaSliceDataSet, vtkPVDataSetAlgorithmSelectorFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPVMetaSliceDataSet* New();

  static const unsigned METASLICE_DATASET = 0;
  static const unsigned METASLICE_HYPERTREEGRID = 1;

  /**
   * Enable or disable the Extract Cells By Regions.
   */
  void PreserveInputCells(int keepCellAsIs);

  /**
   * Override it so we can change the output type of the filter
   */
  vtkAlgorithm* SetActiveFilter(int index) override;

  void SetImplicitFunction(vtkImplicitFunction* func);

  /**
   * Sets the cut function for vtkDataSet inputs
   */
  void SetDataSetCutFunction(vtkImplicitFunction* func);

  /**
   * Sets the cut function for vtkHyperTreeGrid inputs
   */
  void SetHyperTreeGridCutFunction(vtkImplicitFunction* func);

  // Only available for cut -------------

  /**
   * Expose method from vtkPVCutter
   */
  void SetCutFunction(vtkImplicitFunction* func) { this->SetImplicitFunction(func); };

  /**
   * Expose method from vtkPVCutter
   */
  void SetNumberOfContours(int nbContours);

  /**
   * Expose method from vtkPVCutter
   */
  void SetValue(int index, double value);

  /**
   * Expose method from vtkPVCutter
   */
  void SetGenerateTriangles(int status);

  /**
   * Method used for vtkHyperTreeGrid
   */
  void SetDual(bool dual);

  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  ///@{
  /**
   * Set / get a spatial locator.
   */
  void SetLocator(vtkIncrementalPointLocator* locator);
  vtkIncrementalPointLocator* GetLocator();
  ///@}

protected:
  vtkPVMetaSliceDataSet();
  ~vtkPVMetaSliceDataSet() override;

  vtkImplicitFunction* ImplicitFunctions[2];

private:
  vtkPVMetaSliceDataSet(const vtkPVMetaSliceDataSet&) = delete;
  void operator=(const vtkPVMetaSliceDataSet&) = delete;

  class vtkInternals;
  vtkInternals* Internal;
  vtkSmartPointer<vtkIncrementalPointLocator> Locator;
};

#endif
