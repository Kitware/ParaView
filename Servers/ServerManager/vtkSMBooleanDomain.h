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
// .NAME vtkSMBooleanDomain -
// .SECTION Description
// .SECTION See Also
// vtkSMDomain 

#ifndef __vtkSMBooleanDomain_h
#define __vtkSMBooleanDomain_h

#include "vtkSMDomain.h"

class VTK_EXPORT vtkSMBooleanDomain : public vtkSMDomain
{
public:
  static vtkSMBooleanDomain* New();
  vtkTypeRevisionMacro(vtkSMBooleanDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns true if the propery is a vtkSMIntVectorProperty.
  // Return 0 otherwise.
  virtual int IsInDomain(vtkSMProperty* property);


protected:
  vtkSMBooleanDomain();
  ~vtkSMBooleanDomain();

  virtual void SaveState(const char*, ofstream*, vtkIndent) {};

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkPVXMLElement* element);

private:
  vtkSMBooleanDomain(const vtkSMBooleanDomain&); // Not implemented
  void operator=(const vtkSMBooleanDomain&); // Not implemented
};

#endif
