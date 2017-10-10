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
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

class vtkImplicitFunction;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPVMetaClipDataSet
  : public vtkPVDataSetAlgorithmSelectorFilter
{
public:
  vtkTypeMacro(vtkPVMetaClipDataSet, vtkPVDataSetAlgorithmSelectorFilter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  static vtkPVMetaClipDataSet* New();

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
   * Expose method from vtkClip
   */
  void SetValue(double value);

  void SetInputArrayToProcess(
    int idx, int port, int connection, int fieldAssociation, const char* name) VTK_OVERRIDE;
  void SetInputArrayToProcess(
    int idx, int port, int connection, int fieldAssociation, int fieldAttributeType) VTK_OVERRIDE;
  void SetInputArrayToProcess(int idx, vtkInformation* info) VTK_OVERRIDE;

  void SetInputArrayToProcess(
    int idx, int port, int connection, const char* fieldName, const char* fieldType) VTK_OVERRIDE;

  /**
   * Expose method from vtkClip
   */
  void SetUseValueAsOffset(int);

  /**
   * Add validation for active filter so that the vtkExtractGeometry
   * won't be used without ImplicifFuntion being set.
   */
  int ProcessRequest(vtkInformation* request, vtkInformationVector** inInfo,
    vtkInformationVector* outInfo) VTK_OVERRIDE;

  // Add validation for active filter so that the vtkExtractGeometry
  // won't be used without ImplicifFuntion being set.
  int ProcessRequest(
    vtkInformation* request, vtkCollection* inInfo, vtkInformationVector* outInfo) VTK_OVERRIDE;

protected:
  vtkPVMetaClipDataSet();
  ~vtkPVMetaClipDataSet() override;

  // Check to see if this filter can do crinkle, return true if
  // we need to switch active filter, so that we can switch back after.
  bool SwitchFilterForCrinkle();

private:
  vtkPVMetaClipDataSet(const vtkPVMetaClipDataSet&) = delete;
  void operator=(const vtkPVMetaClipDataSet&) = delete;

  class vtkInternals;
  vtkInternals* Internal;
};

#endif
