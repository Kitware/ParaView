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
 * @class   vtkSMFunctionalBagChartSeriesSelectionDomain
 * @brief   extends vtkSMChartSeriesListDomain to
 * add logic to better handle default visibility suitable for bag and outliers.
 *
 * vtkSMFunctionalBagChartSeriesSelectionDomain extends vtkSMChartSeriesSelectionDomain to
 * handle default values visibility for bags and outliers.
*/

#ifndef vtkSMFunctionalBagChartSeriesSelectionDomain_h
#define vtkSMFunctionalBagChartSeriesSelectionDomain_h

#include "vtkPVServerManagerRenderingModule.h" // needed for exports
#include "vtkSMChartSeriesSelectionDomain.h"

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMFunctionalBagChartSeriesSelectionDomain
  : public vtkSMChartSeriesSelectionDomain
{
public:
  static vtkSMFunctionalBagChartSeriesSelectionDomain* New();
  vtkTypeMacro(vtkSMFunctionalBagChartSeriesSelectionDomain, vtkSMChartSeriesSelectionDomain);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Returns the default visibility for a series given its name.
   */
  virtual bool GetDefaultSeriesVisibility(const char*) VTK_OVERRIDE;

protected:
  vtkSMFunctionalBagChartSeriesSelectionDomain();
  ~vtkSMFunctionalBagChartSeriesSelectionDomain();

  /**
   * Get the default value that will be used for the series with the given name
   * by this domain.
   */
  virtual std::vector<vtkStdString> GetDefaultValue(const char* series) VTK_OVERRIDE;

private:
  vtkSMFunctionalBagChartSeriesSelectionDomain(
    const vtkSMFunctionalBagChartSeriesSelectionDomain&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMFunctionalBagChartSeriesSelectionDomain&) VTK_DELETE_FUNCTION;
};

#endif
