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
// .NAME vtkSMBagChartSeriesListDomain - list of strings corresponding to the names
// of the arrays in the required input dataset that can be used as a series in a
// chart.
// .SECTION Description
// vtkSMBagChartSeriesListDomain is designed to be used for the X-axis array name
// property on chart representations. This is similar to vtkSMArrayListDomain,
// however, it simplifies the logic that is uses to determine the available
// arrays and at the same time, uses custom logic to pick the default array for
// based on priorities for arrays.
//
// Supported Required-Property functions:
// \li \c Input : (required) refers to a property that provides the data-producer.
// \li \c FieldDataSelection : (required) refers to a field-selection property
// used to select the data-association i.e. cell-data, point-data etc.
//
// Supported XML attributes
// \li hide_partial_arrays : when set to 1, partial arrays will not be shown in
// the domain (default).
#ifndef __vtkSMBagChartSeriesListDomain_h
#define __vtkSMBagChartSeriesListDomain_h

#include "vtkSMChartSeriesListDomain.h"
#include "vtkPVServerManagerRenderingModule.h" // needed for exports

class vtkPVDataInformation;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMBagChartSeriesListDomain :
  public vtkSMChartSeriesListDomain
{
public:
  static vtkSMBagChartSeriesListDomain* New();
  vtkTypeMacro(vtkSMBagChartSeriesListDomain, vtkSMChartSeriesListDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the default values for the property.
  virtual int SetDefaultValues(vtkSMProperty*);

//BTX
protected:
  vtkSMBagChartSeriesListDomain();
  ~vtkSMBagChartSeriesListDomain();

  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);

  int ArrayType;

private:
  vtkSMBagChartSeriesListDomain(const vtkSMBagChartSeriesListDomain&); // Not implemented
  void operator=(const vtkSMBagChartSeriesListDomain&); // Not implemented
//ETX
};

#endif
