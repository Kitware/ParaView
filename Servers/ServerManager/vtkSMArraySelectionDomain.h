/*=========================================================================

  Program:   ParaView
  Module:    vtkSMArraySelectionDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMArraySelectionDomain - adds all strings from information property
// .SECTION Description
// vtkSMArraySelectionDomain overwrites vtkSMStringListRangeDomain's Update
// in which it adds all strings from a required information property to
// the domain. The information property must have been populated by 
// an vtkSMArraySelectionInformationHelper. It also sets the IntDomainMode 
// to BOOLEAN.
// .SECTION See Also
// vtkSMStringListRangeDomain vtkSMStringVectorProperty
// vtkSMArraySelectionInformationHelper

#ifndef __vtkSMArraySelectionDomain_h
#define __vtkSMArraySelectionDomain_h

#include "vtkSMStringListRangeDomain.h"

class VTK_EXPORT vtkSMArraySelectionDomain : public vtkSMStringListRangeDomain
{
public:
  static vtkSMArraySelectionDomain* New();
  vtkTypeRevisionMacro(vtkSMArraySelectionDomain, vtkSMStringListRangeDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Populate the values of the domain from the given information
  // property.
  virtual void Update(vtkSMProperty*);

protected:
  vtkSMArraySelectionDomain();
  ~vtkSMArraySelectionDomain();

private:
  vtkSMArraySelectionDomain(const vtkSMArraySelectionDomain&); // Not implemented
  void operator=(const vtkSMArraySelectionDomain&); // Not implemented
};

#endif
