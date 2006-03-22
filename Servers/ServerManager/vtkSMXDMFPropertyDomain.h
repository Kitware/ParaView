/*=========================================================================

  Program:   ParaView
  Module:    vtkSMXDMFPropertyDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMXDMFPropertyDomain - adds all string from information property
// vtkSMArraySelectionDomain overwrites vtkSMStringListRangeDomain's Update
// in which it adds all strings from a required information property to
// the domain. The information property must have been populated by 
// an vtkSMXDMFInformationHelper. It also sets the IntDomainMode 
// to RANGE.
// .SECTION See Also
// vtkSMStringListRangeDomain vtkSMStringVectorProperty
// vtkSMXDMFInformationHelper

#ifndef __vtkSMXDMFPropertyDomain_h
#define __vtkSMXDMFPropertyDomain_h

#include "vtkSMStringListRangeDomain.h"

class VTK_EXPORT vtkSMXDMFPropertyDomain : public vtkSMStringListRangeDomain
{
public:
  static vtkSMXDMFPropertyDomain* New();
  vtkTypeRevisionMacro(vtkSMXDMFPropertyDomain, vtkSMStringListRangeDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Populate the values of the domain from the given information
  // property.
  virtual void Update(vtkSMProperty*);

protected:
  vtkSMXDMFPropertyDomain();
  ~vtkSMXDMFPropertyDomain();

private:
  vtkSMXDMFPropertyDomain(const vtkSMXDMFPropertyDomain&); // Not implemented
  void operator=(const vtkSMXDMFPropertyDomain&); // Not implemented
};

#endif
