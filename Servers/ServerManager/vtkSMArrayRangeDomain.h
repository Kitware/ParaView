/*=========================================================================

  Program:   ParaView
  Module:    vtkSMArrayRangeDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMArrayRangeDomain -
// .SECTION Description
// .SECTION See Also
// vtkSMDomain 

#ifndef __vtkSMArrayRangeDomain_h
#define __vtkSMArrayRangeDomain_h

#include "vtkSMDoubleRangeDomain.h"

class vtkSMProxyProperty;
class vtkSMSourceProxy;
class vtkSMInputArrayDomain;
class vtkPVDataSetAttributesInformation;

class VTK_EXPORT vtkSMArrayRangeDomain : public vtkSMDoubleRangeDomain
{
public:
  static vtkSMArrayRangeDomain* New();
  vtkTypeRevisionMacro(vtkSMArrayRangeDomain, vtkSMDoubleRangeDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Updates the string list based on the available arrays. Requires
  // a property of class vtkSMProxyProperty which points to a
  // vtkSMSourceProxy and contains a vtkSMInputArrayDomain. Only
  // the first proxy and domain are used.
  virtual void Update(vtkSMProperty* prop);

protected:
  vtkSMArrayRangeDomain();
  ~vtkSMArrayRangeDomain();

  void Update(const char* arrayName,
              vtkSMProxyProperty* ip,
              vtkSMSourceProxy* sp);
  void Update(const char* arrayName,
              vtkSMSourceProxy* sp,
              vtkSMInputArrayDomain* iad);
  void SetArrayRange(vtkPVDataSetAttributesInformation* info,
                     const char* arrayName);

private:
  vtkSMArrayRangeDomain(const vtkSMArrayRangeDomain&); // Not implemented
  void operator=(const vtkSMArrayRangeDomain&); // Not implemented
};

#endif
