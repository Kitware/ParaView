// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMFunctionalBagChartSeriesSelectionDomain
 * @brief   extends vtkSMChartSeriesListDomain to
 * add logic to better handle default visibility suitable for bag and outliers.
 *
 * vtkSMFunctionalBagChartSeriesSelectionDomain extends vtkSMChartSeriesSelectionDomain to
 * handle default values visibility for bags and outliers.
 */

#ifndef vtkSMFunctionalBagChartSeriesSelectionDomain_h
#define vtkSMFunctionalBagChartSeriesSelectionDomain_h

#include "vtkBagPlotViewsAndFiltersBagPlotModule.h" // for export macro
#include "vtkSMChartSeriesSelectionDomain.h"

class VTKBAGPLOTVIEWSANDFILTERSBAGPLOT_EXPORT vtkSMFunctionalBagChartSeriesSelectionDomain
  : public vtkSMChartSeriesSelectionDomain
{
public:
  static vtkSMFunctionalBagChartSeriesSelectionDomain* New();
  vtkTypeMacro(vtkSMFunctionalBagChartSeriesSelectionDomain, vtkSMChartSeriesSelectionDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns the default visibility for a series given its name.
   */
  bool GetDefaultSeriesVisibility(const char*) override;

protected:
  vtkSMFunctionalBagChartSeriesSelectionDomain();
  ~vtkSMFunctionalBagChartSeriesSelectionDomain() override;

  /**
   * Get the default value that will be used for the series with the given name
   * by this domain.
   */
  std::vector<std::string> GetDefaultValue(const char* series) override;

private:
  vtkSMFunctionalBagChartSeriesSelectionDomain(
    const vtkSMFunctionalBagChartSeriesSelectionDomain&) = delete;
  void operator=(const vtkSMFunctionalBagChartSeriesSelectionDomain&) = delete;
};

#endif
