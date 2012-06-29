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
  vtkTypeMacro(vtkSMArraySelectionDomain, vtkSMStringListRangeDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Populate the values of the domain from the given information
  // property.
  virtual void Update(vtkSMProperty*);
  
  // Description:
  // A vtkSMProperty is often defined with a default value in the
  // XML itself. However, many times, the default value must be determined
  // at run time. To facilitate this, domains can override this method
  // to compute and set the default value for the property.
  // Note that unlike the compile-time default values, the
  // application must explicitly call this method to initialize the
  // property.
  // Returns 1 if the domain updated the property.
  virtual int SetDefaultValues(vtkSMProperty*);

protected:
  vtkSMArraySelectionDomain();
  ~vtkSMArraySelectionDomain();

private:
  vtkSMArraySelectionDomain(const vtkSMArraySelectionDomain&); // Not implemented
  void operator=(const vtkSMArraySelectionDomain&); // Not implemented
};

#endif
