/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStringListDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMStringListDomain -
// .SECTION Description
// .SECTION See Also
// vtkSMDomain 

#ifndef __vtkSMStringListDomain_h
#define __vtkSMStringListDomain_h

#include "vtkSMDomain.h"

//BTX
struct vtkSMStringListDomainInternals;
//ETX

class VTK_EXPORT vtkSMStringListDomain : public vtkSMDomain
{
public:
  static vtkSMStringListDomain* New();
  vtkTypeRevisionMacro(vtkSMStringListDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns true if the value of the property is in the domain.
  // The propery has to be a vtkSMStringVectorProperty. If all 
  // vector values are in the domain, it returns 1. It returns
  // 0 otherwise.
  virtual int IsInDomain(vtkSMProperty* property);

  // Description:
  // Returns true if the string is in the domain.
  int IsInDomain(const char* string, unsigned int& idx);

  // Description:
  unsigned int GetNumberOfStrings();

  // Description:
  const char* GetString(unsigned int idx);

  // Description:
  unsigned int AddString(const char* string);

  // Description:
  void RemoveString(const char* string);

  // Description:
  void RemoveAllStrings();

protected:
  vtkSMStringListDomain();
  ~vtkSMStringListDomain();

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);

  virtual void SaveState(const char* name, ofstream* file, vtkIndent indent);

  vtkSMStringListDomainInternals* SLInternals;

private:
  vtkSMStringListDomain(const vtkSMStringListDomain&); // Not implemented
  void operator=(const vtkSMStringListDomain&); // Not implemented
};

#endif
