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
// .NAME vtkSMFieldDataDomain -
// .SECTION Description
// .SECTION See Also
// vtkSMDomain 

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
  virtual void Update(vtkSMProperty* prop);

protected:
  vtkSMFieldDataDomain();
  ~vtkSMFieldDataDomain();

  // Description:
  // Utility functions called by Update()
  int CheckForArray(vtkSMSourceProxy* sp,
                    vtkPVDataSetAttributesInformation* info, 
                    vtkSMInputArrayDomain* iad);
  void Update(vtkSMSourceProxy* sp, vtkSMInputArrayDomain* iad);
  void Update(vtkSMProxyProperty* pp, vtkSMSourceProxy* sp);

private:
  vtkSMFieldDataDomain(const vtkSMFieldDataDomain&); // Not implemented
  void operator=(const vtkSMFieldDataDomain&); // Not implemented
};

#endif
