/*=========================================================================

  Program:   ParaView
  Module:    vtkSMArrayListDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMArrayListDomain -
// .SECTION Description
// .SECTION See Also
// vtkSMDomain 

#ifndef __vtkSMArrayListDomain_h
#define __vtkSMArrayListDomain_h

#include "vtkSMStringListDomain.h"

class vtkPVDataSetAttributesInformation;
class vtkSMInputArrayDomain;
class vtkSMProxyProperty;
class vtkSMSourceProxy;

class VTK_EXPORT vtkSMArrayListDomain : public vtkSMStringListDomain
{
public:
  static vtkSMArrayListDomain* New();
  vtkTypeRevisionMacro(vtkSMArrayListDomain, vtkSMStringListDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Updates the string list based on the available arrays. Requires
  // a property of class vtkSMProxyProperty which points to a
  // vtkSMSourceProxy and contains a vtkSMInputArrayDomain. Only
  // the first proxy and domain are used.
  virtual void Update(vtkSMProperty* prop);

  // Description:
  vtkGetMacro(DefaultElement, unsigned int);

protected:
  vtkSMArrayListDomain();
  ~vtkSMArrayListDomain();

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);

  // Description:
  // Utility functions called by Update()
  void AddArrays(vtkSMSourceProxy* sp,
                 vtkPVDataSetAttributesInformation* info, 
                 vtkSMInputArrayDomain* iad);
  void Update(vtkSMSourceProxy* sp, vtkSMInputArrayDomain* iad);
  void Update(vtkSMProxyProperty* pp, vtkSMSourceProxy* sp);
  void Update(vtkSMProxyProperty* pp);

  vtkSetMacro(AttributeType, int);
  vtkGetMacro(AttributeType, int);

  vtkSetMacro(DefaultElement, unsigned int);

  int AttributeType;
  unsigned int DefaultElement;

private:
  vtkSMArrayListDomain(const vtkSMArrayListDomain&); // Not implemented
  void operator=(const vtkSMArrayListDomain&); // Not implemented
};

#endif
