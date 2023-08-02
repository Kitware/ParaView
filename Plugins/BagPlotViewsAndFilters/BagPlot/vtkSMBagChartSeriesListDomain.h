// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMBagChartSeriesListDomain
 * @brief   extends vtkSMChartSeriesListDomain to
 * setup defaults specific for bag plot representations.
 *
 * vtkSMBagChartSeriesListDomain extends vtkSMChartSeriesListDomain to
 * setup defaults specific for bag plot representations.
 */

#ifndef vtkSMBagChartSeriesListDomain_h
#define vtkSMBagChartSeriesListDomain_h

#include "vtkBagPlotViewsAndFiltersBagPlotModule.h" // for export macro
#include "vtkSMChartSeriesListDomain.h"

class vtkPVDataInformation;

class VTKBAGPLOTVIEWSANDFILTERSBAGPLOT_EXPORT vtkSMBagChartSeriesListDomain
  : public vtkSMChartSeriesListDomain
{
public:
  static vtkSMBagChartSeriesListDomain* New();
  vtkTypeMacro(vtkSMBagChartSeriesListDomain, vtkSMChartSeriesListDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the default values for the property.
   */
  int SetDefaultValues(vtkSMProperty*, bool use_unchecked_values) override;

protected:
  vtkSMBagChartSeriesListDomain();
  ~vtkSMBagChartSeriesListDomain() override;

  int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element) override;

  int ArrayType;

private:
  vtkSMBagChartSeriesListDomain(const vtkSMBagChartSeriesListDomain&) = delete;
  void operator=(const vtkSMBagChartSeriesListDomain&) = delete;
};

#endif
