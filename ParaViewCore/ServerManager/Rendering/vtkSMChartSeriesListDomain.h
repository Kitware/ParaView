/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMChartSeriesListDomain
 * @brief   list of strings corresponding to the names
 * of the arrays in the required input dataset that can be used as a series in a
 * chart.
 *
 * vtkSMChartSeriesListDomain is designed to be used for the X-axis array name
 * property on chart representations. This is similar to vtkSMArrayListDomain,
 * however, it simplifies the logic that is uses to determine the available
 * arrays and at the same time, uses custom logic to pick the default array for
 * based on priorities for arrays.
 *
 * Supported Required-Property functions:
 * \li \c Input : (required) refers to a property that provides the data-producer.
 * \li \c FieldDataSelection : (required) refers to a field-selection property
 * used to select the data-association i.e. cell-data, point-data etc.
 *
 * Supported XML attributes
 * \li hide_partial_arrays : when set to 1, partial arrays will not be shown in
 * the domain (default).
*/

#ifndef vtkSMChartSeriesListDomain_h
#define vtkSMChartSeriesListDomain_h

#include "vtkPVServerManagerRenderingModule.h" // needed for exports
#include "vtkSMStringListDomain.h"

class vtkPVArrayInformation;
class vtkPVDataInformation;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMChartSeriesListDomain : public vtkSMStringListDomain
{
public:
  static vtkSMChartSeriesListDomain* New();
  vtkTypeMacro(vtkSMChartSeriesListDomain, vtkSMStringListDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Update self checking the "unchecked" values of all required
   * properties. Overwritten by sub-classes.
   */
  void Update(vtkSMProperty*) override;

  /**
   * Set the default values for the property.
   */
  int SetDefaultValues(vtkSMProperty*, bool use_unchecked_values) override;

  /**
   * Returns the list of series that are know to this domain are are given a
   * priority when setting default values. This array is NULL terminated i.e.
   * the last entry in this array will be NULL.
   */
  static const char** GetKnownSeriesNames();

protected:
  vtkSMChartSeriesListDomain();
  ~vtkSMChartSeriesListDomain() override;

  /**
   * Returns the datainformation from the current input, if possible.
   */
  vtkPVDataInformation* GetInputInformation();

  int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element) override;

  virtual void PopulateArrayComponents(vtkPVArrayInformation*, std::vector<vtkStdString>&);

  bool HidePartialArrays;

private:
  vtkSMChartSeriesListDomain(const vtkSMChartSeriesListDomain&) = delete;
  void operator=(const vtkSMChartSeriesListDomain&) = delete;
};

#endif
