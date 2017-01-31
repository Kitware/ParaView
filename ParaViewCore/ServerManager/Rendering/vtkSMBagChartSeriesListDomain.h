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
 * @class   vtkSMBagChartSeriesListDomain
 * @brief   extends vtkSMChartSeriesListDomain to
 * setup defaults specific for bag plot representations.
 *
 * vtkSMBagChartSeriesListDomain extends vtkSMChartSeriesListDomain to
 * setup defaults specific for bag plot representations.
*/

#ifndef vtkSMBagChartSeriesListDomain_h
#define vtkSMBagChartSeriesListDomain_h

#include "vtkPVServerManagerRenderingModule.h" // needed for exports
#include "vtkSMChartSeriesListDomain.h"

class vtkPVDataInformation;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMBagChartSeriesListDomain
  : public vtkSMChartSeriesListDomain
{
public:
  static vtkSMBagChartSeriesListDomain* New();
  vtkTypeMacro(vtkSMBagChartSeriesListDomain, vtkSMChartSeriesListDomain);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Set the default values for the property.
   */
  virtual int SetDefaultValues(vtkSMProperty*, bool use_unchecked_values) VTK_OVERRIDE;

protected:
  vtkSMBagChartSeriesListDomain();
  ~vtkSMBagChartSeriesListDomain();

  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element) VTK_OVERRIDE;

  int ArrayType;

private:
  vtkSMBagChartSeriesListDomain(const vtkSMBagChartSeriesListDomain&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMBagChartSeriesListDomain&) VTK_DELETE_FUNCTION;
};

#endif
