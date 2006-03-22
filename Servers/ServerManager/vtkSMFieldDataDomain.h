/*=========================================================================

  Program:   ParaView
  Module:    vtkSMFieldDataDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMFieldDataDomain - enumeration with point and cell data entries
// .SECTION Description
// vtkSMFieldDataDomain is a sub-class vtkSMEnumerationDomain that looks at 
// the input in Update() and populates the entry list based on whether
// there are valid arrays in point or cell data. At most it consists of two
// entries: ("Point Data", vtkDataSet::POINT_DATA_FIELD) and 
// ("Cell Data",  vtkDataSet::CELL_DATA_FIELD).
// It requires Input (vtkSMProxyProperty) property.
// .SECTION See Also
// vtkSMEnumerationDomain vtkSMProxyProperty

#ifndef __vtkSMFieldDataDomain_h
#define __vtkSMFieldDataDomain_h

#include "vtkSMEnumerationDomain.h"

class vtkPVDataSetAttributesInformation;
class vtkSMInputArrayDomain;
class vtkSMProxyProperty;
class vtkSMSourceProxy;

class VTK_EXPORT vtkSMFieldDataDomain : public vtkSMEnumerationDomain
{
public:
  static vtkSMFieldDataDomain* New();
  vtkTypeRevisionMacro(vtkSMFieldDataDomain, vtkSMEnumerationDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Check the input and appropriate fields (point data or cell data)
  // to the enumeration. This uses the Input property with a
  // vtkSMInputArrayDomain.
  virtual void Update(vtkSMProperty* prop);

protected:
  vtkSMFieldDataDomain();
  ~vtkSMFieldDataDomain();

private:

  // Description:
  // Utility functions called by Update()
  int CheckForArray(vtkSMSourceProxy* sp,
                    vtkPVDataSetAttributesInformation* info, 
                    vtkSMInputArrayDomain* iad);
  void Update(vtkSMSourceProxy* sp, vtkSMInputArrayDomain* iad);
  void Update(vtkSMProxyProperty* pp, vtkSMSourceProxy* sp);

  vtkSMFieldDataDomain(const vtkSMFieldDataDomain&); // Not implemented
  void operator=(const vtkSMFieldDataDomain&); // Not implemented
};

#endif
