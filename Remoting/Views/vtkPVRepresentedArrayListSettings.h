/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRepresentedArrayListSettings.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVRepresentedArrayListSettings
 * @brief   singleton used to filter out undesired array names from color array list.
 *
 * Currently represent the "Miscellaneous" section of ParaView settings.
 *
 * vtkPVRepresentedArrayListSettings is a singleton used to keep track
 * of a list of regular expressions that filter out arrays in a
 * RepresentedArrayList domain.
 *
 * It also keeps track of a list of excluded readers used to filter
 * the File .. Open dialog, as well as rules on when to compute the magnitude
 * for data arrays.
 *
 * All calls to
 * vtkPVRepresentedArrayListSettings::New() returns a reference to the
 * singleton instance.
 */

#ifndef vtkPVRepresentedArrayListSettings_h
#define vtkPVRepresentedArrayListSettings_h

#include "vtkObject.h"
#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSmartPointer.h"        // needed for vtkSmartPointer

class vtkStringArray;
class vtkDataArraySelection;

class VTKREMOTINGVIEWS_EXPORT vtkPVRepresentedArrayListSettings : public vtkObject
{
public:
  static vtkPVRepresentedArrayListSettings* New();
  vtkTypeMacro(vtkPVRepresentedArrayListSettings, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Access the singleton.
   */
  static vtkPVRepresentedArrayListSettings* GetInstance();

  ///@{
  /**
   * Set/get the number of filter expressions.
   */
  virtual void SetNumberOfFilterExpressions(int n);
  virtual int GetNumberOfFilterExpressions();
  ///@}

  ///@{
  /**
   * Set/get the filter expression at index i. If the index is
   * outside the valid range, this call is a noop.
   */
  virtual void SetFilterExpression(int i, const char* expression);
  virtual const char* GetFilterExpression(int i);
  ///@}

  ///@{
  /**
   * Set/get the number of excluded name filters.
   */
  virtual void SetNumberOfExcludedNameFilters(int n);
  virtual int GetNumberOfExcludedNameFilters();
  ///@}

  ///@{
  /**
   * Set/get the excluded name filter at index i. If the index is
   * outside the valid range, this call is a noop.
   */
  virtual void SetExcludedNameFilter(int i, const char* expression);
  virtual const char* GetExcludedNameFilter(int i);
  ///@}

  /// Provide the list of all name filters
  vtkStringArray* GetAllNameFilters();

  ///@{
  /**
   * Setters to control whether ParaView should allow the user to display
   * the gradient of a given multi-component field.
   *
   * If ComputeArrayMagnitude is true then ParaView will always compute the magnitude.
   * Otherwise it will not.
   *
   * ArrayMagnitudeExceptions is the list of exceptions and will behave differently
   * depending on the value of ComputeArrayMagnitude. If ComputeArrayMagnitude is
   * false then the list describes the number of components for which ParaView will
   * still compute the magnitude. Else the list describe the number of components
   * for which ParaView will not compute the magnitude.
   *
   * @see ShouldUseMagnitudeMode
   */
  vtkSetMacro(ComputeArrayMagnitude, bool);
  virtual void SetArrayMagnitudeException(int idx, int ncomp);
  virtual void SetNumberOfArrayMagnitudeExceptions(int nexceptions);
  ///@}

  /**
   * Check if the array mode for this number of components should be the magnitude mode
   * or not. This will internally use the informations given by ComputeArrayMagnitude
   * and ArrayMagnitudeExceptions.
   *
   * @see SetComputeArrayMagnitude
   */
  virtual bool ShouldUseMagnitudeMode(int ncomp) const;

protected:
  vtkPVRepresentedArrayListSettings();
  ~vtkPVRepresentedArrayListSettings() override;

private:
  vtkPVRepresentedArrayListSettings(const vtkPVRepresentedArrayListSettings&) = delete;
  void operator=(const vtkPVRepresentedArrayListSettings&) = delete;

  static vtkSmartPointer<vtkPVRepresentedArrayListSettings> Instance;

  bool ComputeArrayMagnitude = true;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
