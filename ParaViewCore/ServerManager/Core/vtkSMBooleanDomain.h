/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBooleanDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMBooleanDomain - a domain with two values: true or false
// .SECTION Description
// vtkSMBooleanDomain does not really restrict the values of the property
// that contains it. All integer values are valid. Rather, it is used to
// specified that the property represents a true/false state. This domains
// works with only vtkSMIntVectorProperty.
// .SECTION See Also
// vtkSMDomain vtkSMIntVectorProperty

#ifndef __vtkSMBooleanDomain_h
#define __vtkSMBooleanDomain_h

#include "vtkSMDomain.h"

class VTK_EXPORT vtkSMBooleanDomain : public vtkSMDomain
{
public:
  static vtkSMBooleanDomain* New();
  vtkTypeMacro(vtkSMBooleanDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns true if the propery is a vtkSMIntVectorProperty.
  // Return 0 otherwise.
  virtual int IsInDomain(vtkSMProperty* property);

  // Description:
  // Set the value of an element of a property from the animation editor.
  virtual void SetAnimationValue(vtkSMProperty *property, int idx,
                                 double value);

protected:
  vtkSMBooleanDomain();
  ~vtkSMBooleanDomain();

private:
  vtkSMBooleanDomain(const vtkSMBooleanDomain&); // Not implemented
  void operator=(const vtkSMBooleanDomain&); // Not implemented
};

#endif
