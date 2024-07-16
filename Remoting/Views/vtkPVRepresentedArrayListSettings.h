// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVRepresentedArrayListSettings
 * @brief   singleton used to filter out undesired data attribute from the representation.
 *
 * Currently represent the "Represented Attributes" section of ParaView settings.
 *
 * vtkPVRepresentedArrayListSettings is a singleton used to keep track
 * of a list of regular expressions that filter out arrays in a
 * RepresentedArrayList domain, and contains other settings controlling which
 * array should be visible in which situation.
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

#include <string>
#include <vector>

class vtkStringArray;
class vtkDataArraySelection;
namespace vtksys
{
class RegularExpression;
}

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

  ///@{
  /**
   * ChartsDefaultXAxis is the list of array names that will be selected by default as
   * the X axis when displaying some data in charts.
   */
  virtual void SetNumberOfChartsDefaultXAxis(int n);
  virtual int GetNumberOfChartsDefaultXAxis() const;
  virtual void SetChartsDefaultXAxis(int i, const char* expression);
  virtual const char* GetChartsDefaultXAxis(int i) const;
  virtual const std::vector<std::string>& GetAllChartsDefaultXAxis() const;
  ///@}

  ///@{
  /**
   * ChartsHiddenAttributes is a list of regex that controls whether or not array should
   * be visible by defaults in a chart. If the name of the array matches one of the regex
   * then it will be hidden by defaults, otherwise it will be visible.
   *
   * See GetSeriesVisibilityDefault to evaluate the default visibility of a given array name.
   */
  virtual void SetNumberOfChartsHiddenAttributes(int n);
  virtual int GetNumberOfChartsHiddenAttributes() const;
  virtual void SetChartsHiddenAttributes(int i, const char* expression);
  virtual const std::vector<vtksys::RegularExpression>& GetAllChartsHiddenAttributes() const;
  virtual bool GetSeriesVisibilityDefault(const char* name) const;
  ///@}

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
