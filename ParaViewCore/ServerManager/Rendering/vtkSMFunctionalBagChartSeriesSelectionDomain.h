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
// .NAME vtkSMFunctionalBagChartSeriesSelectionDomain - extends vtkSMChartSeriesListDomain to
// add logic to better handle default visibility suitable for bag and outliers.
// .SECTION Description
// vtkSMFunctionalBagChartSeriesSelectionDomain extends vtkSMChartSeriesSelectionDomain to
// handle default values visibility for bags and outliers.

#ifndef __vtkSMFunctionalBagChartSeriesSelectionDomain_h
#define __vtkSMFunctionalBagChartSeriesSelectionDomain_h

#include "vtkSMChartSeriesSelectionDomain.h"
#include "vtkPVServerManagerRenderingModule.h" // needed for exports


class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMFunctionalBagChartSeriesSelectionDomain :
  public vtkSMChartSeriesSelectionDomain
{
public:
  static vtkSMFunctionalBagChartSeriesSelectionDomain* New();
  vtkTypeMacro(vtkSMFunctionalBagChartSeriesSelectionDomain, vtkSMChartSeriesSelectionDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the default visibility for a series given its name.
  virtual bool GetDefaultSeriesVisibility(const char*);

//BTX
protected:
  vtkSMFunctionalBagChartSeriesSelectionDomain();
  ~vtkSMFunctionalBagChartSeriesSelectionDomain();

  // Description:
  // Get the default value that will be used for the series with the given name
  // by this domain.
  virtual std::vector<vtkStdString> GetDefaultValue(const char* series);

private:
  vtkSMFunctionalBagChartSeriesSelectionDomain(const vtkSMFunctionalBagChartSeriesSelectionDomain&); // Not implemented
  void operator=(const vtkSMFunctionalBagChartSeriesSelectionDomain&); // Not implemented

//ETX
};

#endif
