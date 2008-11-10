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
// there are valid arrays in point/cell/vertex/edge/row data. 
// At most it consists of two
// entries: ("Point Data", vtkDataObject::FIELD_ASSOCIATION_POINTS) and 
// ("Cell Data",  vtkDataObject::FIELD_ASSOCIATION_CELLS) in case of vtkDataSet
// subclasses 
// OR
// ("Vertex Data", vtkDataObject::FIELD_ASSOCIATION_VERTICES) and
// ("Edge Data", vtkDataObject::FIELD_ASSOCIATION_EDGES) in case of vtkGraph and
// subclasses 
// OR
// ("Row Data", vtkDataObject::FIELD_ASSOCIATION_ROWS) in case of vtkTable and
// subclasses.
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

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* elem);

  // When true, "Field Data" option is added to the domain.
  bool EnableFieldDataSelection;
private:

  // Description:
  // Utility functions called by Update()
  int CheckForArray(vtkSMSourceProxy* sp, int outputport,
                    vtkPVDataSetAttributesInformation* info, 
                    vtkSMInputArrayDomain* iad);
  void Update(vtkSMSourceProxy* sp, vtkSMInputArrayDomain* iad, int outputport);
  void Update(vtkSMProxyProperty* pp, vtkSMSourceProxy* sp, int outputport);

  vtkSMFieldDataDomain(const vtkSMFieldDataDomain&); // Not implemented
  void operator=(const vtkSMFieldDataDomain&); // Not implemented
};

#endif
