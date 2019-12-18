/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVMetaClipDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVMetaClipDataSet
 * Meta class for clip filter that will allow the user to switch between
 * a regular clip filter or an extract cell by region filter.
*/

#ifndef vtkPVMetaClipDataSet_h
#define vtkPVMetaClipDataSet_h

#include "vtkPVDataSetAlgorithmSelectorFilter.h"
#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports

class vtkImplicitFunction;

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkPVMetaClipDataSet
  : public vtkPVDataSetAlgorithmSelectorFilter
{
public:
  vtkTypeMacro(vtkPVMetaClipDataSet, vtkPVDataSetAlgorithmSelectorFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPVMetaClipDataSet* New();

  static const unsigned METACLIP_DATASET = 0;
  static const unsigned METACLIP_HYPERTREEGRID = 1;

  /**
   * Enable or disable the Extract Cells By Regions.
   */
  void PreserveInputCells(int keepCellAsIs);

  void SetImplicitFunction(vtkImplicitFunction* func);

  void SetInsideOut(int insideOut);

  // Only available for cut -------------

  /**
   * Expose method from vtkCutter
   */
  void SetClipFunction(vtkImplicitFunction* func) { this->SetImplicitFunction(func); };

  /**
   * Sets the clip function for vtkDataSet inputs
   */
  void SetDataSetClipFunction(vtkImplicitFunction* func);

  /**
   * Sets the clip function for vtkHyperTreeGrid inputs
   */
  void SetHyperTreeGridClipFunction(vtkImplicitFunction* func);

  /**
   * Expose method from vtkClip
   */
  void SetValue(double value);

  void SetInputArrayToProcess(
    int idx, int port, int connection, int fieldAssociation, const char* name) override;
  void SetInputArrayToProcess(
    int idx, int port, int connection, int fieldAssociation, int fieldAttributeType) override;
  void SetInputArrayToProcess(int idx, vtkInformation* info) override;

  void SetInputArrayToProcess(
    int idx, int port, int connection, const char* fieldName, const char* fieldType) override;

  //@{
  /**
   * For clipping a box we will only get an approximate box from the vtkPVBox implicit function
   * which can give undesired results. In order to get the exact box geometry output we need
   * to perform 6 plane clips which is very expensive. The default is to not use the exact option.
   * Additionally, the exact box clip must have inside out enabled.
   */
  vtkSetMacro(ExactBoxClip, bool);
  vtkGetMacro(ExactBoxClip, bool);
  vtkBooleanMacro(ExactBoxClip, bool);
  //@}

  /**
   * Expose method from vtkClip
   */
  void SetUseValueAsOffset(int);

  /**
   * Add validation for active filter so that the vtkExtractGeometry
   * won't be used without ImplicifFuntion being set.
   */
  int ProcessRequest(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo) override;

  // Add validation for active filter so that the vtkExtractGeometry
  // won't be used without ImplicifFuntion being set.
  int ProcessRequest(
    vtkInformation* request, vtkCollection* inInfo, vtkInformationVector* outInfo) override;

  virtual int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

protected:
  vtkPVMetaClipDataSet();
  ~vtkPVMetaClipDataSet() override;

  // Check to see if this filter can do crinkle, return true if
  // we need to switch active filter, so that we can switch back after.
  bool SwitchFilterForCrinkle();

  vtkImplicitFunction* ImplicitFunctions[2];

private:
  vtkPVMetaClipDataSet(const vtkPVMetaClipDataSet&) = delete;
  void operator=(const vtkPVMetaClipDataSet&) = delete;

  bool ExactBoxClip;

  class vtkInternals;
  vtkInternals* Internal;
};

#endif
