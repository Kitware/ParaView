/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBoundsDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMBoundsDomain -
// .SECTION Description
// .SECTION See Also
// vtkSMDoubleRangeDomain

#ifndef __vtkSMBoundsDomain_h
#define __vtkSMBoundsDomain_h

#include "vtkSMDoubleRangeDomain.h"

class vtkSMProxyProperty;

class VTK_EXPORT vtkSMBoundsDomain : public vtkSMDoubleRangeDomain
{
public:
  static vtkSMBoundsDomain* New();
  vtkTypeRevisionMacro(vtkSMBoundsDomain, vtkSMDoubleRangeDomain);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Update self checking the "unchecked" values of all required
  // properties. Overwritten by sub-classes.
  virtual void Update(vtkSMProperty*);

protected:
  vtkSMBoundsDomain();
  ~vtkSMBoundsDomain();

  void Update(vtkSMProxyProperty *pp);
  
private:
  vtkSMBoundsDomain(const vtkSMBoundsDomain&); // Not implemented
  void operator=(const vtkSMBoundsDomain&); // Not implemented
};

#endif
