/*=========================================================================

  Program:   ParaView
  Module:    vtkSMChartUseIndexForAxisDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMChartUseIndexForAxisDomain - extends vtkSMBooleanDomain to pick a
// good default for properties such as "UseIndexForXAxis" on chart
// representations.
// .SECTION Description
// vtkSMChartUseIndexForAxisDomain extends vtkSMBooleanDomain to add logic to
// pick an appropriate default e.g. UseIndexForXAxis for bar and line charts
// needs to be set to 0 by default, if the XArrayName is one of the known arrays
// such as "bin_extents", "arc_length", and set to 1 otherwise. This class
// encapsulates that logic.
//
// Supported Required-Property functions:
// \li ArraySelection : (required) refers to the property that dictates the
// array selection.
#ifndef __vtkSMChartUseIndexForAxisDomain_h
#define __vtkSMChartUseIndexForAxisDomain_h

#include "vtkSMBooleanDomain.h"
#include "vtkPVServerManagerRenderingModule.h" // needed for exports

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMChartUseIndexForAxisDomain : public vtkSMBooleanDomain
{
public:
  static vtkSMChartUseIndexForAxisDomain* New();
  vtkTypeMacro(vtkSMChartUseIndexForAxisDomain, vtkSMBooleanDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the default values for the property.
  // Overridden to handle the default value for "UseIndexForXAxis" property, if
  // any. UseIndexForXAxis needs to be OFF by default, if the required property
  // e.g. XArrayName property has one of the known types of arrays, otherwise it
  // must be ON.
  virtual int SetDefaultValues(vtkSMProperty*);

  // Description:
  // Overridden to fire DomainModified when the required property changes. This ensures
  // that SetDefaultValues() is called during proxy post-initialization after the required
  // property has been reset to default.
  virtual void Update(vtkSMProperty* requestingProperty);

//BTX
protected:
  vtkSMChartUseIndexForAxisDomain();
  ~vtkSMChartUseIndexForAxisDomain();

private:
  vtkSMChartUseIndexForAxisDomain(const vtkSMChartUseIndexForAxisDomain&); // Not implemented
  void operator=(const vtkSMChartUseIndexForAxisDomain&); // Not implemented
//ETX
};

#endif
