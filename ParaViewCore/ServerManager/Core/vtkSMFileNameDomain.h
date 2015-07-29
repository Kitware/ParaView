/*=========================================================================

  Program:   ParaView
  Module:    vtkSMFileNameDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMFileNameDomain - a domain with two values: true or false
// .SECTION Description
// vtkSMFileNameDomain does not really restrict the values of the property
// that contains it. All integer values are valid. Rather, it is used to
// specified that the property represents a true/false state. This domains
// works with only vtkSMIntVectorProperty.
// .SECTION See Also
// vtkSMDomain vtkSMIntVectorProperty

#ifndef __vtkSMFileNameDomain_h
#define __vtkSMFileNameDomain_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMDomain.h"

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMFileNameDomain : public vtkSMDomain
{
public:
  static vtkSMFileNameDomain* New();
  vtkTypeMacro(vtkSMFileNameDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns true if the value of the property is in the domain.
  // The propery has to be a vtkSMStringVectorProperty. If in 
  // the domain, it returns 1. It returns 0 otherwise.
  virtual int IsInDomain(vtkSMProperty*);

  // Update self checking the "unchecked" values of all required
  // properties. Overwritten by sub-classes.
  virtual void Update(vtkSMProperty*);

  vtkGetStringMacro(FileName);

  virtual void SetAnimationValue(vtkSMProperty*, int, double);

protected:
  vtkSMFileNameDomain();
  ~vtkSMFileNameDomain();

private:
  vtkSMFileNameDomain(const vtkSMFileNameDomain&); // Not implemented
  void operator=(const vtkSMFileNameDomain&); // Not implemented
  char * FileName;
};

#endif
