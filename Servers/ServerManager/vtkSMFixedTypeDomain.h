/*=========================================================================

  Program:   ParaView
  Module:    vtkSMFixedTypeDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMFixedTypeDomain -
// .SECTION Description
// .SECTION See Also
// vtkSMDomain 

#ifndef __vtkSMFixedTypeDomain_h
#define __vtkSMFixedTypeDomain_h

#include "vtkSMDomain.h"

class vtkSMSourceProxy;

class VTK_EXPORT vtkSMFixedTypeDomain : public vtkSMDomain
{
public:
  static vtkSMFixedTypeDomain* New();
  vtkTypeRevisionMacro(vtkSMFixedTypeDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns true if the value of the propery is in the domain.
  // The propery has to be a vtkSMProxyProperty which points
  // to a vtkSMSourceProxy. If the new (unchecked) source proxy
  // has the same number of parts and data types as the old
  // one, it returns 1. Returns 0 otherwise.
  virtual int IsInDomain(vtkSMProperty* property);

  // Description:
  virtual int IsInDomain(vtkSMSourceProxy* oldProxy,
                         vtkSMSourceProxy* newProxy);

protected:
  vtkSMFixedTypeDomain();
  ~vtkSMFixedTypeDomain();

  virtual void SaveState(const char* name, ofstream* file, vtkIndent indent);

private:
  vtkSMFixedTypeDomain(const vtkSMFixedTypeDomain&); // Not implemented
  void operator=(const vtkSMFixedTypeDomain&); // Not implemented
};

#endif
